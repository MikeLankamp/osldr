#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

/*
 * Heap functions: malloc(), free(), realloc()
 */

typedef struct _HEAPBLOCK HEAPBLOCK;

struct _HEAPBLOCK
{
    ULONG       PrevSize;
    ULONG       Size;
    HEAPBLOCK** Prev;
    HEAPBLOCK*  Next;
};

#define BLOCKSIZE       sizeof(HEAPBLOCK)
#define MIN_BLOCKSIZE   4

#define BLOCK_FREE      1
#define BLOCK_ALIGNED   2
#define SIZEMASK        -4
#define SIZE(x)         ((x)->Size & SIZEMASK)
#define PREVSIZE(x)     ((x)->PrevSize & SIZEMASK)

#define ALIGNMENT       4
#define ALIGN(x)        (((x) + (ALIGNMENT-1)) & -ALIGNMENT)

#define NEXT(x)         ((HEAPBLOCK*)((UCHAR*)(x) + SIZE(x)     + BLOCKSIZE))
#define PREV(x)         ((HEAPBLOCK*)((UCHAR*)(x) - PREVSIZE(x) - BLOCKSIZE))

#define HASNEXT(x)       ((UCHAR*)(x) + sizeof(ULONG) <= (UCHAR*)HeapStart + HeapSize)
#define HASPREV(x)       ((UCHAR*)(x) + sizeof(ULONG) >  (UCHAR*)HeapStart)

/* Linked list of free blocks */
static HEAPBLOCK* HeapList;

/* Heap range */
static VOID* HeapStart;
static ULONG HeapSize;

static VOID Link( HEAPBLOCK* Block )
{
    Block->Prev = &HeapList;
    Block->Next = HeapList;
    if (HeapList != NULL)
    {
        HeapList->Prev = &Block->Next;
    }
    HeapList = Block;
}

static VOID Unlink( HEAPBLOCK *Block )
{
    *Block->Prev = Block->Next;
    if (Block->Next != NULL)
    {
        Block->Next->Prev = Block->Prev;
    }
}

/*
 * Heap Allocation Routines
 *
 * These functions can be safely used when allocating buffers for DMA
 * transfers; all (re-)allocations are guaranteed not to cross any 64k
 * boundries.
 */

VOID HeapInit( VOID* Address, ULONG Size )
{
    VOID* Aligned;
    ULONG Before;
    ULONG After;

    Address = (VOID*)(((ULONG)Address + 15) & -16);
    Aligned = (VOID*)(((ULONG)Address + 65535) & -65536);
    Before  = (ULONG)Aligned - (ULONG)Address;
    After   = (Size - Before) % 65536;
    Size    =  Size - Before - After;

    if (Before < BLOCKSIZE + MIN_BLOCKSIZE) Before = 0;
    if (After  < BLOCKSIZE + MIN_BLOCKSIZE) After  = 0;
    HeapList  = NULL;
    HeapStart = (CHAR*)Aligned - Before;
    HeapSize  = Before + Size + After;

    if (After > 0)
    {
        /* Create a seperate block after the last aligned block */
        HEAPBLOCK* Block = (HEAPBLOCK*)((CHAR*)Aligned + Size - offsetof(HEAPBLOCK, Size));
        Block->Size = (After - BLOCKSIZE) | BLOCK_ALIGNED | BLOCK_FREE;
        NEXT(Block)->PrevSize = Block->Size;
        Link( Block );
    }

    while (Size > 0)
    {
        HEAPBLOCK* Block;

        Size -= 65536;
        Block = (HEAPBLOCK*)((CHAR*)Aligned + Size - offsetof(HEAPBLOCK, Size));
        Block->Size = (65536 - BLOCKSIZE) | BLOCK_ALIGNED | BLOCK_FREE;
        NEXT(Block)->PrevSize = Block->Size;
        Link( Block );
    }

    if (Before > 0)
    {
        /* Create a seperate block before the first aligned block */
        HEAPBLOCK* Block = (HEAPBLOCK*)((CHAR*)Address - offsetof(HEAPBLOCK, Size));
        Block->Size = (Before - BLOCKSIZE) | BLOCK_FREE;
        NEXT(Block)->PrevSize = Block->Size;
        Link( Block );
    }
}

VOID* malloc( ULONG nBytes )
{
    HEAPBLOCK* Block;

    nBytes = ALIGN(nBytes);

    /* Walk through the linked list */
    for (Block = HeapList; Block != NULL; Block = Block->Next)
    {
        if (Block->Size >= nBytes)
        {
            /* We found a big enough block */
            ULONG Remainder = SIZE(Block) - nBytes;

            Unlink( Block );

            if (Remainder >= MIN_BLOCKSIZE + BLOCKSIZE)
            {
                /* It's worth splicing the block */
                HEAPBLOCK* NewBlock;

                /* Clear FREE, copy ALIGNED flag */
                Block->Size = nBytes | (Block->Size & BLOCK_ALIGNED);

                NewBlock = NEXT(Block);
                NewBlock->PrevSize = Block->Size;
                NewBlock->Size     = (Remainder - BLOCKSIZE) | BLOCK_FREE;
                NEXT(NewBlock)->PrevSize = NewBlock->Size;

                Link( NewBlock );
            }
            else
            {
                /* Just clear free flag */
                Block->Size &= ~BLOCK_FREE;
                NEXT(Block)->PrevSize &= ~BLOCK_FREE;
            }

            /* Return pointer to user area after header */
            return (Block + 1);
        }
    }

    /* No blocks found */
    return NULL;
}

VOID free( VOID* Address )
{
    HEAPBLOCK* Block = (HEAPBLOCK*)Address - 1;

    if ((HeapList != NULL) && (Address != NULL) && (~Block->Size & BLOCK_FREE))
    {
        /* The heap and block are valid */

        /* Mark as free block */
        Block->Size |= BLOCK_FREE;
        NEXT(Block)->PrevSize |= BLOCK_FREE;

        /* Try to merge with succeeding block */
        if (HASNEXT(Block))
        {
            HEAPBLOCK* Next = NEXT(Block);
            if ((Next->Size & BLOCK_FREE) && (~Next->Size & BLOCK_ALIGNED))
            {
                Unlink( Next );

                /* Increase block size */
                Block->Size += SIZE(Next) + BLOCKSIZE;
                NEXT(Block)->PrevSize = Block->Size;
            }
        }

        /* Try to merge with preceding block */
        if ((HASPREV(Block)) && (~Block->Size & BLOCK_ALIGNED))
        {
            HEAPBLOCK* Prev = PREV(Block);
            if (Prev->Size & BLOCK_FREE)
            {
                Unlink( Prev );

                /* Increase block size */
                Prev->Size += SIZE(Block) + BLOCKSIZE;
                NEXT(Prev)->PrevSize = Prev->Size;
                Block = Prev;
            }
        }
        Link( Block );
    }
}

VOID* realloc( VOID* Address, ULONG nBytes )
{
    if (nBytes == 0)
    {
        free( Address );
        return NULL;
    }
    else if (Address == NULL)
    {
        return malloc( nBytes );
    }

    if (HeapList != NULL)
    {
        HEAPBLOCK* Block = (HEAPBLOCK*)Address - 1;
        HEAPBLOCK* Next;

        /* Size of current block is sufficient */
        if (nBytes <= SIZE(Block))
        {
            return Address;
        }

        nBytes = ALIGN(nBytes);

        /* Try to expand with succeeding block */
        if (HASNEXT(Block))
        {
            Next = NEXT(Block);

            if ((Next->Size & BLOCK_FREE) && (~Next->Size & BLOCK_ALIGNED) && (SIZE(Block) + SIZE(Next) >= nBytes))
            {
                ULONG Remainder = SIZE(Block) + SIZE(Next) + BLOCKSIZE - nBytes;

                Unlink(Next);

                if (Remainder >= MIN_BLOCKSIZE + BLOCKSIZE)
                {
                    /* It's worth splicing the block */
                    HEAPBLOCK* NewBlock;

                    /* Clear FREE, copy ALIGNED flag */
                    Block->Size = nBytes | (Block->Size & BLOCK_ALIGNED);

                    NewBlock = NEXT(Block);
                    NewBlock->PrevSize = Block->Size;
                    NewBlock->Size     = (Remainder - BLOCKSIZE) | BLOCK_FREE;
                    NEXT(NewBlock)->PrevSize = NewBlock->Size;

                    Link( NewBlock );
                }
                else
                {
                    /* Increase size */
                    Block->Size += SIZE(Next) + BLOCKSIZE;
                    NEXT(Block)->PrevSize = Block->Size;
                }

                /* Return pointer to user area after header */
                return (Block + 1);
            }
        }

        /* Cannot increase block, allocate new, copy and free old */
        Next = malloc( nBytes );
        if (Next != NULL)
        {
            memcpy( Next, Block + 1, SIZE(Block) );
            free( Block + 1 );
            return Next;
        }
    }

    /* Invalid heap or out of memory */
    return NULL;
}

/*
 * Numeric conversion functions
 */
ULONG strtoul(CHAR *nptr, CHAR **endptr, INT base)
{
    ULONG result = 0;
    CHAR* s = nptr;

    /* To prevent checking for NULL every time */
    if (endptr == NULL) endptr = &nptr;

    /* Ignore starting whitespaces */
    while (isspace(*s)) s++;

    /* Only ignore +, this is unsigned conversion */
    if (*s == '+') s++;

    if (base == 0)
    {
        /* Auto base */
        if (s[0] != '0') base = 10;
        else if ((s[1] >= '0') && (s[1] <= '7')) base = 8;
        else if (tolower(s[1]) == 'x') base = 16;
    }

    if ((base < 2) || (base > 36))
    {
        /* Invalid base or auto base failed */
        *endptr = nptr;
        return 0;
    }

    /* Skip 0x or 0X for hexidecimal numbers */
    if ((base == 16) && (s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')))
    {
        s += 2;
    }

    for (; isalnum(*s); s++)
    {
        ULONG i = (isdigit(*s)) ? (*s - '0') : (10 + tolower(*s) - 'a');
        if (i >= base)
        {
            /* Invalid character */
            break;
        }

        i = result * base + i;
        if (i < result)
        {
            /* It has overflown */
            errno = ERANGE;
            return ULONG_MAX;
        }
        result = i;
    }

    *endptr = s;
    return result;
}

