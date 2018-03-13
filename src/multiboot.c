#include <bios.h>
#include <errno.h>
#include <limits.h>
#include <mem.h>
#include <multiboot.h>
#include <stdlib.h>
#include <string.h>

static BOOL GetApmInfo( APM_TABLE *apm )
{
    REGS regs;

    regs.h.ah = 0x53;   /* APM */
    regs.h.al = 0x00;   /* Installation check */
    regs.x.bx = 0;      /* Power device ID of APM BIOS */

    int86( 0x15, &regs, &regs );
    if (regs.x.cflag)
    {
        return FALSE;
    }

    apm->Version = regs.x.ax;
    apm->Flags   = regs.x.cx;

    regs.h.ah = 0x53;   /* APM */
    regs.h.al = 0x03;   /* Protected Mode 32-bit interface connect */
    regs.x.bx = 0;      /* Power device ID of APM BIOS */

    int86( 0x15, &regs, &regs );
    if (regs.x.cflag)
    {
        return FALSE;
    }

    apm->CSeg      = regs.x.ax;
    apm->Offset    = regs.d.ebx;
    apm->CSeg16    = regs.x.cx;
    apm->DSeg      = regs.x.dx;
    apm->CSegLen   = LOWORD( regs.d.esi );
    apm->CSeg16Len = HIWORD( regs.d.esi );
    apm->DSegLen   = regs.x.di;

    return TRUE;
}

static VOID SetVbeMode( VBE_INFO* vbe, ULONG Type, ULONG Width, ULONG Height, ULONG Depth )
{
    VBE_MODE_INFO vmi;
    UINT          i;
    ULONG         DesiredMemory = Width * Height * Depth;
    ULONG         BestMemory    = ULONG_MAX;
    ULONG         BestMode      = 0;
    REGS          regs;

    /* First check all OEM modes */
    for (i = 0; i < 0x80; i++)
    {
        regs.x.ax = 0x4F01;      /* Return VBE Mode Information */
        regs.x.cx = i;           /* Mode number */
        regs.x.es = SEG( &vmi );
        regs.x.di = OFS( &vmi );
        int86( 0x10, &regs, &regs );
        if ((regs.x.ax == 0x004F) && (vmi.ModeAttributes & 1) &&  /* Call succeeded and mode supported? */
            ((~vmi.ModeAttributes & 0x10) == Type) &&             /* Text or graphics match? */
            ((vmi.ModeAttributes & 0x80) || (Type != 0)))         /* Linear buffer supported for graphics? */
        {
            ULONG Memory = vmi.XResolution * vmi.YResolution * vmi.BitsPerPixel;
                  Memory = (Memory > DesiredMemory) ? (Memory - DesiredMemory) : (DesiredMemory - Memory);
            if (Memory < BestMemory)
            {
                BestMemory = Memory;
                BestMode   = i;
            }
        }
    }

    /* Then check all listed VBE modes */
    USHORT* VideoMode = PTR( HIWORD(vbe->VbeControlInfo->VideoModePtr), LOWORD(vbe->VbeControlInfo->VideoModePtr) );
    while (*VideoMode != 0xFFFF)
    {
        regs.x.ax = 0x4F01;      /* Return VBE Mode Information */
        regs.x.cx = *VideoMode;  /* Mode number */
        regs.x.es = SEG( &vmi );
        regs.x.di = OFS( &vmi );
        int86( 0x10, &regs, &regs );
        if ((regs.x.ax == 0x004F) && (vmi.ModeAttributes & 1) &&  /* Call succeeded and mode supported? */
            ((~vmi.ModeAttributes & 0x10) == Type) &&             /* Text or graphics match? */
            ((vmi.ModeAttributes & 0x80) || (Type != 0)))         /* Linear buffer supported for graphics? */
        {
            ULONG Memory = vmi.XResolution * vmi.YResolution * vmi.BitsPerPixel;
                  Memory = (Memory > DesiredMemory) ? (Memory - DesiredMemory) : (DesiredMemory - Memory);
            if (Memory < BestMemory)
            {
                BestMemory = Memory;
                BestMode   = *VideoMode;
            }
        }
        VideoMode++;
    }

    /* BestMode is the best approximation, set it */
    regs.x.ax = 0x4F02;            /* Set VBE Mode */
    regs.x.cx = BestMode | 0x4000; /* Mode number, with linear frame buffer */
    int86( 0x10, &regs, &regs );

    /* If the function fails, the current mode will be used */
}

static BOOL GetVbeInfo( VBE_INFO *vbe )
{
    REGS regs;

    vbe->VbeControlInfo = malloc( sizeof(VBE_INFO_BLOCK) );
    if (vbe->VbeControlInfo == NULL)
    {
        return FALSE;
    }

    vbe->VbeControlInfo->VbeSignature[0] = 'V';
    vbe->VbeControlInfo->VbeSignature[1] = 'B';
    vbe->VbeControlInfo->VbeSignature[2] = 'E';
    vbe->VbeControlInfo->VbeSignature[3] = '2';

    regs.x.ax = 0x4F00;     /* Return VBE Controller Information */
    regs.x.es = SEG( vbe->VbeControlInfo );
    regs.x.di = OFS( vbe->VbeControlInfo );

    int86( 0x10, &regs, &regs );
    if (regs.x.ax != 0x004F)
    {
        free( vbe->VbeControlInfo );
        return FALSE;
    }

    return TRUE;
}

static BOOL GetVbeModeInfo( VBE_INFO *vbe )
{
    REGS regs;
    regs.x.ax = 0x4F03;     /* Return current VBE Mode */
    int86( 0x10, &regs, &regs );
    if (regs.x.ax != 0x004F)
    {
        free( vbe->VbeControlInfo );
        return FALSE;
    }

    vbe->VbeModeInfo = malloc( sizeof(VBE_MODE_INFO) );
    if (vbe->VbeModeInfo == NULL)
    {
        free( vbe->VbeControlInfo );
        return FALSE;
    }

    vbe->VbeMode = regs.x.bx;

    regs.x.ax = 0x4F01;     /* Return VBE Mode Information */
    regs.x.cx = vbe->VbeMode;  /* Mode number */
    regs.x.es = SEG( vbe->VbeModeInfo );
    regs.x.di = OFS( vbe->VbeModeInfo );

    int86( 0x10, &regs, &regs );
    if (regs.x.ax != 0x004F)
    {
        free( vbe->VbeModeInfo );
        free( vbe->VbeControlInfo );
        return FALSE;
    }

    regs.x.ax = 0x4F0A;     /* Return VBE Protected Mode Interface */
    regs.h.bl = 0x00;
    int86( 0x10, &regs, &regs );
    if (regs.x.ax != 0x004F)
    {
        /* We don't completely fail, just zero the values */
        regs.x.es = 0;
        regs.x.di = 0;
        regs.x.cx = 0;
    }

    vbe->VbeInterfaceSeg = regs.x.es;
    vbe->VbeInterfaceOff = regs.x.di;
    vbe->VbeInterfaceLen = regs.x.cx;

    return TRUE;
}

BOOL GetSystemInformation( MULTIBOOT_INFO* mbi, MULTIBOOT_HEADER* mbhdr )
{
    REGS regs;

    /*
     * Memory information was already filled in by main()
     */

    /* Get configuration table address */
    regs.h.ah = 0xC0;
    int86( 0x15, &regs, &regs );
    mbi->ConfigTable = NULL;
    if ((!regs.x.cflag) && (regs.h.ah == 0))
    {
        mbi->ConfigTable = (VOID*)(((ULONG)regs.x.es << 4) + (ULONG)regs.x.bx);
        mbi->Flags |= MIF_CONFIG;
    }

    /* Get Advanced Power Management information */
    mbi->ApmTable = malloc( sizeof(APM_TABLE) );
    if (mbi->ApmTable != NULL)
    {
        if (GetApmInfo( mbi->ApmTable ))
        {
            mbi->Flags |= MIF_APM;
        }
    }

    if (mbhdr->Flags & MIF_WANT_GRAPHICS)
    {
        /* Get VESA Bios Extensions information */
        if (GetVbeInfo( &mbi->VbeInfo ))
        {
            mbi->Flags |= MIF_GRAPHICS;
        }

        SetVbeMode( &mbi->VbeInfo, mbhdr->ModeType, mbhdr->Width, mbhdr->Height, mbhdr->Depth );

        GetVbeModeInfo( &mbi->VbeInfo );
    }

    return TRUE;
}

VOID LoadModules( MULTIBOOT_INFO* mbi, MODULE* Modules, ULONG nModules, BOOL PageAlign )
{
    ULONG i;

    for (i = 0; i < nModules; i++)
    {
        char* sp = strchr( Modules[i].String, ' ' );
        if (sp != NULL)
        {
            *sp++ = '\0';
        }

        FILE* file = OpenFile( Modules[i].String );
        if (file != NULL)
        {
            ULONGLONG Size = GetFileSize( file );
            if (Size <= 0xFFFFFFFF)
            {
                VOID* addr = PhysAlloc( 0, Size, (PageAlign) ? 4096 : 0 );
                if (addr != NULL)
                {
                    if (ReadFile( file, addr, Size) == Size)
                    {
                        Modules[i].ModStart = (ULONG)addr;
                        Modules[i].ModEnd   = (ULONG)addr + (ULONG)Size;

                        /* Module loaded, adjust string */
                        if (sp != NULL)
                        {
                            /* Module has an argument string */
                            while (*sp == ' ') sp++;
                            Modules[i].String = sp;
                        }
                        else
                        {
                            /* Module has no argument string */
                            free( Modules[i].String );
                            Modules[i].String = NULL;
                        }

                        CloseFile( file );
                        continue;
                    }
                    PhysFree( addr, Size );
                }
            }
            CloseFile( file );
        }

        /* Error: clear error and remove module */
        errno = EZERO;
        memmove( &Modules[i], &Modules[i+1], (nModules - i - 1) * sizeof(MODULE) );
        nModules--;
        i--;
    }

    mbi->ModuleAddress = (nModules > 0) ? Modules : NULL;
    mbi->ModuleCount   = nModules;
    mbi->Flags        |= MIF_MODULES;
}

