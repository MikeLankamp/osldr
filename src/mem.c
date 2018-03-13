#include <bios.h>
#include <errno.h>
#include <multiboot.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct _PHYSMEM PHYSMEM;
struct _PHYSMEM
{
    ULONGLONG Address;
    ULONGLONG Size;
    PHYSMEM** Prev;
    PHYSMEM*  Next;
};

typedef struct _SMAP_INFO
{
    ULONGLONG Start;
    ULONGLONG Length;
    ULONG     Type;
} SMAP_INFO;

typedef struct _MAP_ENTRY
{
    ULONG     size;
    SMAP_INFO smi;
} MAP_ENTRY;

static PHYSMEM* PhysMem;

static BOOL AddToPhysList( ULONGLONG Start, ULONGLONG Length )
{
    PHYSMEM* cur;

    if (Start + Length <= 0x100000)
    {
        /* Region lies completely below 1 MB, ignore it */
        return TRUE;
    }
    else if (Start < 0x100000)
    {
        /* Limit start to 1 MB */
        Length = Start + Length - 0x100000;
        Start  = 0x100000;
    }

    /* See if we can join the new region with an existing region */
    for (cur = PhysMem; cur != NULL; cur = cur->Next)
    {
        if ((Start >= cur->Address) &&
            (Start <= cur->Address + cur->Size))
        {
            /* Start of region is inside existing region */
            cur->Size = MAX( cur->Size, Start + Length - cur->Address );
            break;
        }

        if ((Start + Length >= cur->Address) &&
            (Start + Length <= cur->Address + cur->Size))
        {
            /* End of region is inside existing region */
            Start = MIN( Start, cur->Address );
            cur->Size    = cur->Address + cur->Size - Start;
            cur->Address = Start;
            break;
        }

        if ((Start < cur->Address) && (Start + Length > cur->Address + cur->Size ))
        {
            /* Region holds existing region */
            cur->Address = Start;
            cur->Size    = Length;
            break;
        }
    }

    if (cur == NULL)
    {
        /* We couldn't join, add it to the linked list */
        PHYSMEM* newmem = malloc( sizeof(PHYSMEM) );
        if (newmem == NULL)
        {
            return FALSE;
        }

        newmem->Address = Start;
        newmem->Size    = Length;
        newmem->Next    = PhysMem;
        newmem->Prev    = &PhysMem;
        if (newmem->Next != NULL)
        {
            newmem->Next->Prev = &newmem->Next;
        }
        PhysMem = newmem;
    }

    return TRUE;
}

VOID* PhysAlloc( ULONGLONG Start, ULONGLONG Size, ULONG Alignment )
{
    PHYSMEM* cur;

    if (Alignment == 0)
    {
        Alignment = 1;
    }

    /* Align and check size */
    Size = (Size + 3) & -4;
    if (Size == 0)
    {
        return NULL;
    }

    /* Find a suitable region */
    for (cur = PhysMem; cur != NULL; cur = cur->Next)
    {
        /* Determine which start address to use */
        ULONGLONG start = (Start != 0) ? Start : ((cur->Address + Alignment - 1) & -Alignment);

        if ((start >= cur->Address) && (start + Size <= cur->Address + cur->Size))
        {
            /* We found a region that's big enough and at the right spot */
            ULONGLONG before = start - cur->Address;
            ULONGLONG after  = (cur->Address + cur->Size) - (start + Size);

            /* Adjust regions to remove allocated space */
            if (before != 0)
            {
                cur->Size = before;

                if (after != 0)
                {
                    AddToPhysList( start + Size, after );
                }
            }
            else if (after != 0)
            {
                cur->Address = start + Size;
                cur->Size    = after;
            }
            else
            {
                /* Region is fully consumed, unlink it */
                *cur->Prev = cur->Next;
                if (cur->Next != NULL)
                {
                    cur->Next->Prev = cur->Prev;
                }
            }

            return (VOID*)(ULONG)start;
        }
    }

    errno = ENOMEM;
    return NULL;
}

VOID PhysFree( VOID* Address, ULONGLONG Size )
{
    AddToPhysList( (ULONG)Address, Size );
}

BOOL PhysInit( MULTIBOOT_INFO* mbi )
{
    PhysMem = NULL;

    if (mbi->Flags & MIF_MEMORY_MAP)
    {
        /* Add memory map information */
        MAP_ENTRY* map = (MAP_ENTRY*)mbi->MemoryMapAddress;

        while ((CHAR*)map < (CHAR*)mbi->MemoryMapAddress + mbi->MemoryMapLength)
        {
            /* Only use available regions */
            if (map->smi.Type == 1)
            {
                if (!AddToPhysList( map->smi.Start, map->smi.Length ))
                {
                    return FALSE;
                }
            }
            map = (MAP_ENTRY*)((CHAR*)map + map->size + sizeof(map->size));
        }
    }

    if (mbi->Flags & MIF_SIMPLE_MEMORY)
    {
        /* Add simple map information */
        if (!AddToPhysList( 0x100000, mbi->MemUpper ))
        {
            return FALSE;
        }
    }

    return TRUE;
}

VOID GetConventionalMemoryMap( MULTIBOOT_INFO* mbi )
{
    REGS regs;

    /* Get memory below 1 MB */
    int86( 0x12, &regs, &regs );
    mbi->MemLower = (ULONG)regs.x.ax;

    /* Get memory above 1 MB (< 16 MB) */
    regs.h.ah = 0x88;
    int86( 0x15, &regs, &regs );
    if (!regs.x.cflag)
    {
        /* Call succeeded, we have > 1 MB info */
        mbi->MemUpper = (ULONG)regs.x.ax;
        mbi->Flags   |= MIF_SIMPLE_MEMORY;
    }

    /*
     * Only check for > 16 MB info if we have 15 MB above 1 MB or
     * if the call didn't succeed
     */
    if ((mbi->MemUpper == 0x3C00) || (regs.x.cflag))
    {
        /* Get memory for > 64MB machines */
        regs.x.ax = 0xE801;
        int86( 0x15, &regs, &regs );
        if ((!regs.x.cflag) && (regs.x.ax == 0x3C00))
        {
            mbi->MemUpper = ((ULONG)regs.x.bx << 6) + 0x3C00;
            mbi->Flags   |= MIF_SIMPLE_MEMORY;
        }
    }
}

VOID GetSystemMemoryMap( MULTIBOOT_INFO* mbi )
{
    REGS regs;

    /* Initialize Multiboot Info fields */
    mbi->Flags |= MIF_MEMORY_MAP;
    mbi->MemoryMapLength  = 0;
    mbi->MemoryMapAddress = NULL;

    regs.d.ebx = 0;
    do
    {
        /* Translate stored address */
        MAP_ENTRY* map = (MAP_ENTRY*)mbi->MemoryMapAddress;

        /* Expand memory map for new entry */
        map = realloc( map, (mbi->MemoryMapLength + 1) * sizeof(MAP_ENTRY));
        if (map == NULL)
        {
            /* Unset flag and values */
            mbi->Flags &= ~MIF_MEMORY_MAP;
            free( mbi->MemoryMapAddress );
            mbi->MemoryMapLength  = 0;
            mbi->MemoryMapAddress = NULL;
            break;
        }

        /* Adjust values in Multiboot info structure */
        mbi->MemoryMapAddress = map;
        map += mbi->MemoryMapLength++;

        regs.d.eax = 0xE820;
        regs.d.edx = 0x534D4150;
        regs.d.ecx = map->size = 20;
        regs.x.es  = SEG( &map->smi );
        regs.x.di  = OFS( &map->smi );
        int86( 0x15, &regs, &regs );

        if ((regs.x.cflag) || (regs.d.eax != 0x534D4150))
        {
            /* Unset flag and values */
            mbi->Flags &= ~MIF_MEMORY_MAP;
            free( mbi->MemoryMapAddress );
            mbi->MemoryMapLength  = 0;
            mbi->MemoryMapAddress = NULL;
            break;
        }

    } while (regs.d.ebx != 0);

    mbi->MemoryMapLength *= sizeof(MAP_ENTRY);
}
