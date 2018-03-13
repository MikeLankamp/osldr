#include <drive.h>
#include <errno.h>
#include <mem.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "loader.h"
#include "messages.h"
#include "filehdrs.h"

#define PAGE_SIZE 0x1000 /* 4 kB page */

/*
 * Searches for the multiboot header in the image and reads it.
 * Returns FALSE if the presence of the header could not be established.
 * Returns TRUE otherwise. Check image->mbhdr.Magic to see if the header
 * was indeed present.
 */
static BOOL ReadMultibootHeader( IMAGE* image )
{
    CHAR*             buf;
    MULTIBOOT_HEADER* hdr;
    ULONGLONG         size = GetFileSize( image->File );

    /* Multiboot header must be fully contained in first 8K, so we load that */
    size = MIN( size, 8192 );

    image->mbhdr.Magic = 0;

    if (!SetFilePointer( image->File, 0, FILE_BEGIN ))
    {
        errno = EFAULT;
        return FALSE;
    }

    buf = malloc( size );
    if (buf == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }

    if (ReadFile( image->File, buf, size ) != size)
    {
        free( buf );
        return FALSE;
    }

    hdr = (MULTIBOOT_HEADER*)buf;
    while ((CHAR*)hdr + sizeof(MULTIBOOT_HEADER) < buf + size)
    {
        if (hdr->Magic == MULTIBOOT_MAGIC)
        {
            /* Wrap-around used, be careful */
            if (hdr->Magic + hdr->Flags + hdr->Checksum == 0)
            {
                image->mbhdrOffset = (CHAR*)hdr - buf;
                image->mbhdr = *hdr;
                break;
            }
        }
        hdr = (MULTIBOOT_HEADER*)((CHAR*)hdr + 4);
    }
    free( buf );
    return TRUE;
}

static VOID LoadBootsector( IMAGE* image )
{
    DRIVE_INFO* pdi;
    ULONGLONG   size;
    UCHAR*      buf = (UCHAR*)0x7C00;

    if ((image->Drive == 0xFFFFFFFF) || (IsDevice( image->File )))
    {
        /* Only set if no value was set or we're loading raw sectors */
        image->Drive = image->File->Device->DeviceId >> 24;
    }

    pdi = GetDriveParameters( image->Drive );

    size = (IsDevice( image->File )) ? pdi->nBytesPerSector : GetFileSize( image->File );

    if (size == pdi->nBytesPerSector)
    {
        if (ReadFile( image->File, buf, size ) == size)
        {
            if (*(USHORT*)&buf[ size - 2 ] == 0xAA55)
            {
                /* This function shouldn't return */
                CallAsBootsector( image->Drive, 0x7C00 );
                errno = EFAULT;
            }
            else
            {
                errno = ECORRUPT;
            }
        }
    }
    else
    {
        errno = ECORRUPT;
    }
}

static BOOL LoadMultiboot( IMAGE* image, MULTIBOOT_INFO* mbi )
{
    ULONGLONG Base   = image->mbhdr.LoadAddr;
    ULONGLONG Size   = GetFileSize( image->File );
    ULONGLONG Offset = image->mbhdrOffset - (image->mbhdr.HeaderAddr - image->mbhdr.LoadAddr);
    CHAR*     mem;

    if ((image->mbhdr.Magic == 0) || (~image->mbhdr.Flags & MIF_HAS_ADDRESS))
    {
        /* No multiboot header or no load information */
        return TRUE;
    }

    /* Validate addresses */
    if ((image->mbhdr.LoadAddr > image->mbhdr.HeaderAddr) || (image->mbhdr.HeaderAddr - image->mbhdr.LoadAddr > image->mbhdrOffset))
    {
        errno = ECORRUPT;
        return FALSE;
    }

    if (image->mbhdr.LoadEndAddr == 0)
    {
        image->mbhdr.LoadEndAddr = image->mbhdr.LoadAddr + (Size - Offset);
    }
    else if (image->mbhdr.LoadEndAddr < image->mbhdr.LoadAddr)
    {
        /* Invalid load end address */
        errno = ECORRUPT;
        return FALSE;
    }

    if (image->mbhdr.BssEndAddr == 0)
    {
        image->mbhdr.BssEndAddr = image->mbhdr.LoadEndAddr;
    }
    else if (image->mbhdr.BssEndAddr < Base + Size)
    {
        /* Invalid BSS end address */
        errno = ECORRUPT;
        return FALSE;
    }

    mem = PhysAlloc( image->mbhdr.LoadAddr, image->mbhdr.BssEndAddr - image->mbhdr.LoadAddr, 0 );
    if (mem == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }

    if (!SetFilePointer( image->File, Offset, FILE_BEGIN ))
    {
        errno = ECORRUPT;
        PhysFree( mem, image->mbhdr.BssEndAddr - Base );
        return FALSE;
    }

    Size = MIN( image->mbhdr.LoadEndAddr - image->mbhdr.LoadAddr, Size);
    if (ReadFile( image->File, mem, Size ) != Size)
    {
        errno = EIO;
        PhysFree( mem, image->mbhdr.BssEndAddr - image->mbhdr.LoadAddr );
        return FALSE;
    }

    /* Clear BSS */
    memset( mem + Size, 0, image->mbhdr.BssEndAddr - (image->mbhdr.LoadAddr + Size) );

    /* Load modules */
    LoadModules( mbi, image->Modules, image->nModules, (image->mbhdr.Flags & MIF_WANT_PAGE_ALIGN) != 0);

    /* This function should not return */
    CallAsMultiboot( image->mbhdr.EntryAddr, mbi );

    errno = EFAULT;
    return FALSE;
}

static BOOL LoadELF( IMAGE* image, MULTIBOOT_INFO* mbi )
{
    ELF32_HDR   hdr;
    ELF32_PHDR* phdr;
    UINT        i;
    BOOL        loadable;

    if (!SetFilePointer( image->File, 0, FILE_BEGIN ))                          return TRUE;
    if (ReadFile( image->File, &hdr, sizeof(hdr)) != sizeof(hdr))               return TRUE;
    if ((hdr.e_ident[EI_MAG0] != ELFMAG0) || (hdr.e_ident[EI_MAG1] != ELFMAG1) ||
        (hdr.e_ident[EI_MAG2] != ELFMAG2) || (hdr.e_ident[EI_MAG3] != ELFMAG3)) return TRUE;
    /* The file is ELF */

    if (hdr.e_ident[EI_VERSION] != EV_CURRENT)
    {
        /* Unknown ELF version */
        return FALSE;
    }

    if (hdr.e_ident[EI_VERSION] != hdr.e_version)
    {
        /* File is corrupt */
        errno = ECORRUPT;
        return FALSE;
    }

    if ((hdr.e_ident[EI_CLASS] != ELFCLASS32) ||
        (hdr.e_ident[EI_DATA] != ELFDATA2LSB) ||
        (hdr.e_machine != EM_386) ||
        (hdr.e_type != ET_EXEC))
    {
        /* Cannot execute this file */
        return FALSE;
    }

    if ((hdr.e_ehsize < sizeof(ELF32_HDR)) ||
        (hdr.e_phoff < hdr.e_ehsize) || (hdr.e_phentsize < sizeof(ELF32_PHDR)))
    {
        /* Corrupt file */
        errno = ECORRUPT;
        return FALSE;
    }

    /* Read program header */
    phdr = malloc( hdr.e_phnum * hdr.e_phentsize );
    if (phdr == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }

    if ((!SetFilePointer( image->File, hdr.e_phoff, FILE_BEGIN )) ||
        (ReadFile( image->File, phdr, hdr.e_phnum * hdr.e_phentsize) != hdr.e_phnum * hdr.e_phentsize))
    {
        errno = ECORRUPT;
        free( phdr );
        return FALSE;
    }

    /* Check segments */
    loadable = FALSE;
    for (i = 0; i < hdr.e_phnum; i++)
    {
        ELF32_PHDR* cur = (ELF32_PHDR*)((CHAR*)phdr + i * hdr.e_phentsize);

        if ( (cur->p_align != PAGE_SIZE) ||
            ((cur->p_offset % PAGE_SIZE) != (cur->p_vaddr % PAGE_SIZE )) ||
             (cur->p_filesz > cur->p_memsz))
        {
            /* Wrong header values */
            errno = ECORRUPT;
            free( phdr );
            return FALSE;
        }

        if (cur->p_type == PT_LOAD)
        {
            loadable = TRUE;
        }
    }

    if (!loadable)
    {
        /* No loadable segments */
        errno = ECORRUPT;
        free( phdr );
        return FALSE;
    }

    /* Load loadable segments */
    for (i = 0; i < hdr.e_phnum; i++)
    {
        ELF32_PHDR* cur = (ELF32_PHDR*)((CHAR*)phdr + i * hdr.e_phentsize);

        if (cur->p_type == PT_LOAD)
        {
            /* Allocate memory and read data */
            cur->p_paddr = (ULONG)PhysAlloc( cur->p_vaddr, cur->p_memsz, 0 );
            if ((VOID*)phdr[i].p_paddr == NULL)
            {
                errno = ENOMEM;
                break;
            }

            if (!SetFilePointer( image->File, cur->p_offset, FILE_BEGIN ))
            {
                errno = ECORRUPT;
                PhysFree( (VOID*)cur->p_paddr, cur->p_memsz );
                break;
            }

            if (ReadFile( image->File, (VOID*)cur->p_paddr, cur->p_filesz) != cur->p_filesz)
            {
                PhysFree( (VOID*)cur->p_paddr, cur->p_memsz );
                break;
            }

            /* Zero remaining bytes */
            memset( (VOID*)(cur->p_paddr + cur->p_filesz), 0, cur->p_memsz - cur->p_filesz );
        }
    }

    if (i < hdr.e_phnum)
    {
        /* Something failed, free everything */
        INT j;
        for (j = 0; j < i; j++)
        {
            if (phdr[j].p_type == PT_LOAD)
            {
                ELF32_PHDR* cur = (ELF32_PHDR*)((CHAR*)phdr + i * hdr.e_phentsize);

                PhysFree( (VOID*)cur->p_paddr, cur->p_memsz );
            }
        }
        free( phdr );
        return FALSE;
    }

    /* Now load the multiboot modules */
    LoadModules( mbi, image->Modules, image->nModules, (image->mbhdr.Flags & MIF_WANT_PAGE_ALIGN) != 0);

    /* This function should not return */
    CallAsMultiboot( hdr.e_entry, mbi );

    errno = EFAULT;
    return FALSE;
}

static BOOL LoadCOFF( IMAGE* image, MULTIBOOT_INFO* mbi )
{
    COFF_FILE_HEADER        fhdr;
    COFF_OPTIONAL_HEADER    ohdr;
    COFF_OPTIONAL_NT_HEADER nthdr;
    COFF_SECTION_HEADER*    shdr;
    ULONG                   offset = 0;
    BOOL                    isPECOFF = FALSE;
    UINT                    i;

    /* Try to read it as PE/COFF and fall back to regular COFF if we can't */
    do
    {
        ULONG ofs;
        ULONG sig;

        if (!SetFilePointer( image->File, 0x3C, FILE_BEGIN ))              break;
        if (ReadFile( image->File, &ofs, sizeof(ULONG)) != sizeof(ULONG))  break;
        if (!SetFilePointer( image->File, ofs, FILE_BEGIN ))               break;
        if (ReadFile( image->File, &sig, sizeof(ULONG)) != sizeof(ULONG))  break;
        if (sig != IMAGE_NT_SIGNATURE)                                     break;
        offset = ofs + 4;
        isPECOFF = TRUE;
    } while (0);

    /* Read file header */
    if (!SetFilePointer( image->File, offset, FILE_BEGIN ))          return TRUE;
    if (ReadFile( image->File, &fhdr, sizeof(fhdr)) != sizeof(fhdr)) return TRUE;
    if (fhdr.Machine != IMAGE_FILE_MACHINE_I386)                     return TRUE;
    /* File is COFF */

    if (~fhdr.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
    {
        return FALSE;
    }

    if (((!isPECOFF) && (fhdr.SizeOfOptionalHeader < sizeof(COFF_OPTIONAL_HEADER))) ||
        (( isPECOFF) && (fhdr.SizeOfOptionalHeader < sizeof(COFF_OPTIONAL_HEADER) + offsetof(COFF_OPTIONAL_NT_HEADER, DataDirectory))))
    {
        errno = ECORRUPT;
        return FALSE;
    }

    offset = GetFilePointer( image->File ) + fhdr.SizeOfOptionalHeader;

    /* Read optional header */
    if (ReadFile( image->File, &ohdr, sizeof(ohdr)) != sizeof(ohdr))
    {
        errno = ECORRUPT;
        return FALSE;
    }

    if (ohdr.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
    {
        errno = ECORRUPT;
        return FALSE;
    }

    if (isPECOFF)
    {
        /* Read NT header */
        if (ReadFile( image->File, &nthdr, sizeof(nthdr)) != sizeof(nthdr))
        {
            errno = ECORRUPT;
            return FALSE;
        }
        ohdr.AddressOfEntryPoint += nthdr.ImageBase;
    }

    /* Read section headers */
    if (!SetFilePointer( image->File, offset, FILE_BEGIN ))
    {
        errno = ECORRUPT;
        return FALSE;
    }

    shdr = malloc( fhdr.NumberOfSections * sizeof(*shdr) );
    if (shdr == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }

    if (ReadFile( image->File, shdr, fhdr.NumberOfSections * sizeof(*shdr) ) != fhdr.NumberOfSections * sizeof(*shdr) )
    {
        errno = ECORRUPT;
        free( shdr );
        return FALSE;
    }

    /* Now load all the sections */
    for (i = 0; i < fhdr.NumberOfSections; i++)
    {
        if (isPECOFF)
        {
            /* With PE/COFF, the virtual addresses are relative to ImageBase */
            shdr[i].VirtualAddress += nthdr.ImageBase;
        }
        else
        {
            /*
             * With regular COFF, there is no virtual size. Instead, the raw
             * size IS the virtual size, except for sections marked as BSS,
             * in which case the raw size is zero.
             */
            shdr[i].VirtualSize = shdr[i].SizeOfRawData;
            if (shdr[i].Flags & CSF_BSS)
            {
                shdr[i].SizeOfRawData = 0;
            }
        }

        CHAR* addr = PhysAlloc( shdr[i].VirtualAddress, shdr[i].VirtualSize, 0 );
        if (addr == NULL)
        {
            errno = ENOMEM;
            break;
        }

        if (shdr[i].SizeOfRawData > 0)
        {
            if (!SetFilePointer( image->File, shdr[i].PointerToRawData, FILE_BEGIN ))
            {
                errno = ECORRUPT;
                PhysFree( addr, shdr[i].VirtualSize );
                break;
            }

            if (ReadFile( image->File, addr, shdr[i].SizeOfRawData ) != shdr[i].SizeOfRawData)
            {
                errno = ECORRUPT;
                PhysFree( addr, shdr[i].VirtualSize );
                break;
            }
        }

        if (shdr[i].VirtualSize > shdr[i].SizeOfRawData)
        {
            memset( addr + shdr[i].SizeOfRawData, 0, shdr[i].VirtualSize - shdr[i].SizeOfRawData );
        }
    }

    if (i < fhdr.NumberOfSections)
    {
        /* Something failed, free everything */
        UINT j;
        for (j = 0; j < i; j++)
        {
            PhysFree( (VOID*)shdr[j].VirtualAddress, shdr[j].VirtualSize );
        }
        return FALSE;
    }

    /* Now load the multiboot modules */
    LoadModules( mbi, image->Modules, image->nModules, (image->mbhdr.Flags & MIF_WANT_PAGE_ALIGN) != 0);

    /* This function should not return */
    CallAsMultiboot( ohdr.AddressOfEntryPoint, mbi );

    errno = EFAULT;
    return FALSE;
}

static BOOL LoadBinary( IMAGE* image, MULTIBOOT_INFO* mbi )
{
    ULONGLONG Size;
    VOID*     Mem;

    if (image->Address == 0)
    {
        return TRUE;
    }

    Size = GetFileSize( image->File );
    Mem  = PhysAlloc( image->Address, Size, 0 );
    if (Mem == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }

    if (!SetFilePointer( image->File, 0, FILE_BEGIN ))
    {
        errno = EFAULT;
        PhysFree( Mem, Size );
        return FALSE;
    }

    if (ReadFile( image->File, Mem, Size ) != Size)
    {
        PhysFree( Mem, Size );
        return FALSE;
    }

    LoadModules( mbi, image->Modules, image->nModules, (image->mbhdr.Flags & MIF_WANT_PAGE_ALIGN) != 0);

    /* This function should not return */
    CallAsMultiboot( (ULONG)Mem, mbi );
    return FALSE;
}

VOID LoadImage( IMAGE* image, MULTIBOOT_INFO* mbi )
{
    /* Seperate rest of command line from file */
    CHAR* sp = strchr(image->Command, ' ');
    if (sp != NULL) *sp = '\0';

    image->File = OpenFile( image->Command );
    if (image->File == NULL)
    {
        return;
    }

    /* Restore command line for multiboot info */
    if (sp != NULL)
    {
        *sp = ' ';
    }

    if ((IsDevice( image->File )) || (image->Type == IT_BOOTSECTOR))
    {
        /*
         * The image is bootsector or a device in which case we should load its
         * first sector (bootsector or MBR).
         */
        LoadBootsector( image );
        return;
    }

    /*
     * The image is a file that goes above 1 MB, load according to type.
     */

    if (!ReadMultibootHeader( image ))
    {
        /* We could not determine if there was a multiboot header */
        return;
    }

    if (image->mbhdr.Magic != 0)
    {
        /* The image contains a multiboot header */
        if ((image->mbhdr.Flags & MULTIBOOT_REQUIRED_MASK) & ~MULTIBOOT_SUPPORTED_FLAGS)
        {
            /* The header has requirements we don't support */
            PrintMessage( MSG_LOADER_UNSUPPORTED_REQS );
            return;
        }

        /* Set boot device, command line and bootloader name */
        mbi->BootDevice     = image->File->Device->DeviceId;
        mbi->CmdLine        = image->Command;
        mbi->BootLoaderName = "OSLDR/1.0";
        mbi->Flags         |= MIF_BOOT_DEVICE | MIF_CMDLINE | MIF_LOADER_NAME;

        /* Read system info like APM, config table, disk parameters and VBE */
        GetSystemInformation( mbi, &image->mbhdr );

        /* Compare requirements and capabilities */
        if (((image->mbhdr.Flags & MIF_WANT_GRAPHICS)      && (~mbi->Flags & MIF_GRAPHICS)) ||
            ((image->mbhdr.Flags & MIF_WANT_SIMPLE_MEMORY) && (~mbi->Flags & MIF_SIMPLE_MEMORY)))
        {
            /* The header has requirements we could not fullfil */
            PrintMessage( MSG_LOADER_WRONG_HARDWARE );
            return;
        }
    }

    if (!EnableA20Gate())
    {
        PrintMessage( MSG_LOADER_CANT_ENABLE_A20 );
        return;
    }

    switch (image->Type)
    {
        case IT_RELOCATABLE:
            /* Try to load as a relocatable executable */
            if (LoadELF( image, mbi ))
            if (LoadCOFF( image, mbi ))
            {
                errno = EFTYPE;
            }
            break;

        case IT_BINARY:
            /* Try to load as a binary */
            if (LoadBinary( image, mbi ))
            {
                errno = EFTYPE;
            }
            break;

        case IT_MULTIBOOT:
            /* The multiboot header specifies the load address information */
            if (LoadMultiboot( image, mbi ))
            {
                errno = EFTYPE;
            }
            break;

        default:
            /* Try everything in this specific order */
            if (LoadMultiboot( image, mbi ))
            if (LoadELF( image, mbi ))
            if (LoadCOFF( image, mbi ))
            if (LoadBinary( image, mbi ))
            {
                errno = EFTYPE;
            }
            break;
    }

    /* When we get here, we couldn't load the operating system */
    PrintError( MSG_LOADER_CANT_LOAD_OS );
}

