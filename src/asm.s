.text

/*
 * Call an interrupt
 */
.code32
.global int86
.p2align 3
int86:
    pusha

    call LeaveProtectedMode
    .code16
    
    movw %sp, %bp

    movb 36(%bp), %al
    mov %al, 1 + _int

    /* Read values from inregs */
    movw 40(%bp), %bp
    movl 0x0(%bp), %eax
    movl 0x4(%bp), %ebx
    movl 0x8(%bp), %ecx
    movl 0xc(%bp), %edx
    movl 0x10(%bp), %esi
    movl 0x14(%bp), %edi
    pushw 0x1e(%bp)
    pushw 0x20(%bp)
    popw %es
    popw %ds

_int:
    int $0

    pushf
    pushw %ds
    pushw %es

    xorw %bp, %bp
    movw %bp, %ds
    movw %bp, %es
    movw %sp, %bp

    /* Save to outregs */
    movw 50(%bp), %bp
    popw 0x20(%bp)
    popw 0x1e(%bp)
    movl %eax, 0x0(%bp)
    movl %ebx, 0x4(%bp)
    movl %ecx, 0x8(%bp)
    movl %edx, 0xc(%bp)
    movl %esi, 0x10(%bp)
    movl %edi, 0x14(%bp)

    xorl %eax, %eax
    popw %ax
    movl %eax, 0x18(%bp)    /* outregs.x.flags */
    andw $1, %ax
    mov %ax, 0x1c(%bp)     /* outregs.x.cflag */

    call EnterProtectedMode
    .code32

    popa

    ret

/*
 * This function attempts to enable the A20 gate
 */
.code32
.p2align 3
.global EnableA20Gate
EnableA20Gate:
    call LeaveProtectedMode
    .code16
    cli

    /* Try BIOS first */
    movw $0x2401, %ax
    int $0x15
    cli
    call TestA20
    movb $4, %al
    jne 8f

    /* Try Keyboard Controller (quick way) */
1:  inb $0x64, %al
    testb $2, %al
    jnz 1b
    movb $0xD1, %al
    outb %al, $0x64

2:  inb $0x64, %al
    testb $2, %al
    jnz 2b
    movb $0xDF, %al
    outb %al, $0x60

    call TestA20
    movb $5, %al
    jne 8f

    /* Try Keyboard Controller (old way) */

3:  inb $0x64, %al
    testb $2, %al
    jnz 3b
    movb $0xD0, %al
    outb %al, $0x64

    
4:  inb $0x64, %al
    testb $1, %al
    jz 4b
    inb $0x60, %al
    orb $0x02, %al
    pushw %ax

5:  inb $0x64, %al
    testb $2, %al
    jnz 5b
    movb $0xD1, %al
    outb %al, $0x64

6:  inb $0x64, %al
    testb $2, %al
    jnz 6b
    popw %ax
    outb %al, $0x60

    call TestA20
    movb $2, %al
    jne 8f
    
    /*
     * Try Fast Gate A20 (System Port A, 0x92) last .
     * We try this last because it is actually insecure. Some systems
     * link other stuff to it.
     */
    inb $0x92, %al
    orb $2, %al
    outb %al, $0x92

    /* Wait until A20 is really enabled */
    xorw %cx, %cx
7:
    call TestA20
    loope 7b
    movb $3, %al

8:  /*
     * A20 enable attempt finished. ZF is set if A20 is NOT enabled.
     */
    /* We return a boolean indicating if A20 was enabled */
    lahf
    xorl %edx, %edx
    movb %al, %dh
    movb %ah, %dl
    shrb $6, %dl 
    notb %dl
    andb $1, %dl
    call EnterProtectedMode
    .code32
9:
    movl %edx, %eax
    ret

/*
 * Returns ZF set iff A20 is disabled 
 */
.code16
.p2align 3
TestA20:
    pushw %ds
    pushw %es
    pushw %cx

    xorw %ax, %ax
    movw %ax, %ds

    decw %ax
    movw %ax, %es

    /* We use address 0000:0600 because its a low unused address */
    pushw %ds:(0x600)
    movw $0x1000, %cx
1:
    incw %ax
    movw %ax, %ds:(0x600)
    cmpw %ax, %es:(0x610)
    loope 1b
    popw %ds:(0x600)

    popw %cx
    popw %es
    popw %ds
    ret

/*
 * Calls a memory location as a bootsector. This means that 
 * this function does not return.
 */
.code32
.p2align 3
.global CallAsBootsector
CallAsBootsector:
    popl %edx   /* Remove EIP from stack */
    call LeaveProtectedMode
    .code16
    cli
    cld
    popl %edx   /* Boot Drive */
    popl %eax   /* Jump address */
    movl $0x7c00, %esp
    pushw $0
    pushw %ax
    lret

/*
 * Calls a memory location as a multiboot compliant entry point.
 */
.code32
.p2align 3
.global CallAsMultiboot
CallAsMultiboot:
    popl %ebx   /* Remove EIP from stack */
    cli
    cld
    movl 4(%esp), %ebx
    movl $0x1BADB002, %eax
    ret

/*
 * Note that all real-mode segments (especially CS and SS) must be
 * zero to make for good conversion to linear addresses and back.
 */
.code32
.p2align 3
.global EnterProtectedMode
EnterProtectedMode:
    .code16

    cli

    /* Convert return address to 32-bit */
    xorl %eax, %eax
    popw %ax
    pushl %eax

    /* Clear high word of ESP */
    movw %sp, %ax
    movl %eax, %esp

    lgdtl _GDTR

    /* Enable Protected Mode */
    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0
    ljmp $0x8, $_clrcs

_clrcs:
    .code32

    /* Set protected mode segments */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs

    ret

.code32
.p2align 3
.global LeaveProtectedMode
LeaveProtectedMode:
    cli

    /* Jump to a segment with proper settings (limit 0xffff, 16-bit) */
    ljmp $0x18, $_tmpseg

_tmpseg:
    .code16

    /* Convert return address to 16-bit  */
    popl %eax
    pushw %ax

    /* Set data segments to segments with proper settings
    movw $0x20, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs
    
    /* Disable Protected Mode */
    movl %cr0, %eax
    andb $0xfe, %al
    movl %eax, %cr0

    /* Reset CS */
    ljmp $0, $_clrcs2

_clrcs2:
    /* Now fill segments again, this time with real-mode values */
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs

    sti
    ret

.code32
.p2align 3
.global WaitForInterrupt
WaitForInterrupt:
    call LeaveProtectedMode
    .code16
    sti
    hlt
    call EnterProtectedMode
    .code32
    ret

/*
 * The Global Descriptor Table
 */

.p2align 3
_GDTR:
    .word 5 * 8 - 1
    .long _GDT

.p2align 3
_GDT:
    .long 0x00000000
    .long 0x00000000

    /* 32-bit 4 GB Protected Mode segments */
    .long 0x0000ffff
    .long 0x00cf9a00

    .long 0x0000ffff
    .long 0x00cf9200

    /* 16-bit 64 kB Real Mode segments */
    .long 0x0000ffff
    .long 0x00009a00

    .long 0x0000ffff
    .long 0x00009200

