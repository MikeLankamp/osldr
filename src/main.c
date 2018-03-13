#include <conio.h>
#include <mem.h>
#include <multiboot.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <video.h>

#include "loader.h"
#include "config.h"
#include "messages.h"

/*
 * Making this variable a global is _extremely_ easy for heap allocation.
 * Besides, its not so weird to have this as a global; its pretty central.
 */
static MULTIBOOT_INFO mbi;

/*
 * This is a symbol defined by the linker.
 * It is located at the end of the image when loaded in memory.
 * We use its address to know where to start the heap.
 */
extern int ImageEndAddress;

/*
 * Returns the image that the user wants to run or NULL for a reboot
 */
static IMAGE* RunBootMenu( CONFIG* config, BOOL useTimer )
{
    IMAGE*   img         = config->Images;
    ULONG    MaxLen      = 10;
    ULONG    Selected    = 0;
    ULONG    oldSel;
    ULONG    windowStart = 0;
    ULONG    windowSize  = 13;
    ULONG    textAlign;
    LONGLONG nPrevSecs;
    time_t   start;
    INT      i;

    if ((useTimer) && (config->Timeout == 0))
    {
        /* Don't show anything for a zero timeout */
        return config->Default;
    }

    /* Get maximum name length */
    for (i = config->nImages - 1; i >= 0; i--, img = img->Next )
    {
        INT len = strlen( img->Name );
        if (len > 50)
        {
            /* Limit name length */
            strcpy( &img->Name[47], "..." );
            len = 50;
        }

        if (len > MaxLen)           MaxLen   = len;
        if (img == config->Default) Selected = i;
    }

    /* Get the column of the ':' */
    textAlign = MAX( 2 + strlen( GetMessage( MSG_MENU_SECOND ) ),
                     4 + strlen( GetMessage( MSG_MENU_SECONDS ) ) );
    textAlign = MAX( textAlign, strlen(GetMessage( MSG_MENU_REBOOT )) );
    textAlign = 77 - textAlign;

    SetBkColor( COLOR_BLACK );
    SetTextColor( COLOR_LIGHTGRAY );

    ClearScreen();
    ShowCursor( FALSE );

    /* Draw the static part */
    puts("");
    PrintMessage( MSG_MENU_TITLE );
    printf("\n ");

    INT len = strlen( GetMessage( MSG_MENU_TITLE ) );
    while (len-- > 0)
    {
        printf("\xCD");
    }
    puts("\n");
    PrintMessage( MSG_MENU_INSTR1 );
    PrintMessage( MSG_MENU_INSTR2 );

    GotoXY( textAlign - 2, 23);
    printf("F9: ");
    PrintMessage( MSG_MENU_REBOOT );

    nPrevSecs = -1;
    oldSel    = Selected + 1;
    start     = time( NULL );
    while (1)
    {
        if (kbhit())
        {
            /* Handle keyboard input */
            INT c = getch();

            if (useTimer)
            {
                /* Disable and clear the timer */
                GotoXY( 0, 22 );
                for (i = 0; i < 80; i += 8)
                {
                    printf("        ");
                }
                useTimer = FALSE;
            }

            if (c == VK_UP)
            {
                if (Selected > 0)
                {
                    Selected--;
                    if (Selected < windowStart) windowStart--;
                }
            }
            else if (c == VK_DOWN)
            {
                if (Selected < config->nImages-1)
                {
                    Selected++;
                    if (Selected == windowStart + windowSize) windowStart++;
                }
            }
            else if (c == VK_NEXT)
            {
                if (Selected < config->nImages - 1 - windowSize) Selected += windowSize;
                else                                             Selected  = config->nImages - 1;
                while (Selected >= windowStart + windowSize) windowStart++;
            }
            else if (c == VK_PREV)
            {
                if (Selected >= windowSize) Selected -= windowSize;
                else                        Selected  = 0;
                while (Selected < windowStart) windowStart--;
            }
            else if (c == VK_HOME) { Selected = 0; windowStart = 0; }
            else if (c == VK_END)  { Selected = config->nImages - 1; windowStart = config->nImages - windowSize; }
            else if (c == VK_F9)   { return NULL; }
            else if ((c >= VK_1) && (c < VK_1 + config->nImages))
            {
                Selected = c - VK_1;
            }
            else if (c == VK_ENTER)
            {
                /* Boot selected */
                break;
            }
        }

        if (oldSel != Selected)
        {
            /* The selection has changed. Redraw list */
            img = config->Images;
            for (i = config->nImages - 1; i >= 0; i--, img = img->Next )
            {
                if ((i >= windowStart) && (i < windowStart + windowSize))
                {
                    GotoXY( 1, 8 + i - windowStart );
                    printf(" %2d:", i + 1 );

                    if (i == Selected)
                    {
                        SetBkColor( COLOR_LIGHTGRAY );
                        SetTextColor( COLOR_BLACK );
                    }
                    printf(" %-*s ", MaxLen, img->Name );
                    SetBkColor( COLOR_BLACK );
                    SetTextColor( COLOR_LIGHTGRAY );
                }
            }

            GotoXY( 6, 7 );
            printf( (windowStart > 0) ? "..." : "   " );

            GotoXY( 6, 8 + windowSize );
            printf( (windowStart + windowSize < config->nImages) ? "..." : "   ");

            oldSel = Selected;
        }

        if (useTimer)
        {
            ULONG nSecs = difftime( time( NULL ), start );
            if (nSecs != nPrevSecs )
            {
                /* Timer has changed. Redraw it */
                nPrevSecs = nSecs;

                GotoXY( textAlign - strlen(GetMessage(MSG_MENU_TIME_LEFT)), 22 );

                PrintMessage( MSG_MENU_TIME_LEFT );
                printf(": %d ", config->Timeout - nSecs );

                if (config->Timeout - nSecs == 1) PrintMessage( MSG_MENU_SECOND );
                else                              PrintMessage( MSG_MENU_SECONDS );

                if (nSecs == config->Timeout)
                {
                    /* Timer expired */
                    break;
                }
            }
        }

        WaitForInterrupt();
    }

    /* Timer has expired or user selected an OS */
    img = config->Images;
    for (i = config->nImages - 1; i >= 0; i--, img = img->Next )
    {
        if (i == Selected)
        {
            break;
        }
    }
    return img;
}

/*
 * Called from start.S, which sets up the initial environment.
 * BootDevice contains the boot disk and partition which OSLDR was loaded from.
 * Its format is according to Multiboot specification:
 * Bits 24 - 31: BIOS Drive number
 * Bits 16 - 23: First  partition number (> 4 is extended partition)
 * Bits  8 - 15: Second partition number (BSD partitions)
 * Bits  0 -  7: Third  partition number (unused: 0xFF)
 */
INT main( ULONG BootDevice )
{
    CONFIG Config;
    FILE*  file;
    IMAGE* image;
    INT    ch;

    /* First get the conventional memory */
    GetConventionalMemoryMap( &mbi );

    /*
     * First up, initialize heap immediately after the OSLDR image.
     * We don't want to waste anything of that precious 640 kB.
     * We use the lower memory count to be safe. If we cannot get the lower
     * memory count, we use 0x9FC00 and pray that nothing goes wrong.
     */
    HeapInit( &ImageEndAddress, ((mbi.Flags & MIF_SIMPLE_MEMORY) ? (mbi.MemLower * 1024) : 0x9FC00) - (ULONG)&ImageEndAddress );

    /* Now get the extended memory information (which requires a heap) */
    GetSystemMemoryMap( &mbi );

    /* If we don't have either memory info, error out */
    if ((mbi.Flags & (MIF_SIMPLE_MEMORY | MIF_MEMORY_MAP)) == 0)
    {
        PrintMessage( MSG_MAIN_NO_MEMORY_INFO );
        PrintMessage( MSG_MAIN_PRESS_TO_REBOOT );
        return 0;
    }

    /* Tell the I/O Manager what device we booted from */
    IoInitialize( BootDevice );

    /*
     * Next up: reading the boot.ini file from the booted drive
     */

    file = OpenFile( "/boot.ini" );
    if (file == NULL)
    {
        PrintMessage( MSG_MAIN_CANT_OPEN_CONF_FILE );
        PrintMessage( MSG_MAIN_PRESS_TO_REBOOT );
        return 0;
    }

    if (!ParseConfigFile( file, &Config ))
    {
        CloseFile( file );
        PrintMessage( MSG_MAIN_PRESS_TO_REBOOT );
        return 0;
    }

    CloseFile( file );
    if (Config.nImages == 0)
    {
        PrintMessage( MSG_MAIN_NO_ENTRIES_IN_FILE );
        PrintMessage( MSG_MAIN_PRESS_TO_REBOOT );
        return 0;
    }

    /*
     * Everything loaded ok, determine which image to load
     */

    /* Get the key if the user pressed one */
    ch    = (kbhit()) ? getch() : 0;
    image = Config.Default;

    /* F8 overrides zero timeouts and such */
    if ((Config.nImages > 1) || (ch == VK_F8))
    {
        /* Run boot menu */
        image = RunBootMenu( &Config, ch != VK_F8 );

        if (image == NULL)
        {
            /* User requested reboot */
            return -1;
        }

        ClearScreen();
        ShowCursor( TRUE );
    }

    /*
     * Load and run image
     */
    PhysInit( &mbi );

    LoadImage( image, &mbi );

    PrintMessage( MSG_MAIN_PRESS_TO_REBOOT );
    return 0;
}
