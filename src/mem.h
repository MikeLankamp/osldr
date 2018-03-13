#ifndef MEM_H
#define MEM_H

#include <multiboot.h>

/*
 * Allocates upper memory (>= 1 MB)
 *
 * @Start   The address at which to allocate the memory.
 *          Use 0 for any available address.
 * @Size    The amount of memory to allocate.
 * @Align   If non-zero, the address will be aligned to this. The alignment is
 *          only applied if @Start is zero. If no aligned address can be found,
 *          the function returns NULL.
 *
 *
 * Note that the user of this function is to remember the size for the call
 * to PhysFree().
 */
VOID* PhysAlloc( ULONGLONG Start, ULONGLONG Size, ULONG Align );

/*
 * Frees upper memory (>= 1 MB)
 *
 * @Address The address of the memory to free. This must be at least 1 MB
 * @Size    The amount of memory to free. This should be equal to the size
 *          passed to PhysAlloc().
 *
 * This function makes no effort to check the validity of freeing memory
 * regions.
 */
VOID PhysFree( VOID* Address, ULONGLONG Size );

/*
 * Initializes the physical memory manager.
 *
 * It will get the memory information from the MemUpper and MemoryMap members
 * of @mbi. Make sure that these are filled in correctly and the flags are set
 * to indicate they are valid.
 */
BOOL  PhysInit( MULTIBOOT_INFO* mbi );

VOID GetConventionalMemoryMap( MULTIBOOT_INFO* mbi );
VOID GetSystemMemoryMap( MULTIBOOT_INFO* mbi );

#endif
