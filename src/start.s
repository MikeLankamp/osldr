.text

/*
 * This is first code called from the bootsector and should be linked first
 * We are still in 16 bit real mode.
 */
.code16
.global start
start:
    pushl %eax
    pushl %edx

    /* Now we can enter Protected mode and call main() */
    call EnterProtectedMode
    .code32

    call main
    addl $4, %esp
    pushl %eax

    call LeaveProtectedMode
    .code16

    /* Wait for a keypress if wanted */
    popl %eax
    orl %eax, %eax
    jnz 5f
    xorb %ah, %ah
    int $0x16
5:

    /* Reboot machine through KBC */
    cli
6:
    inb $0x64, %al
    testb $0x02, %al
    jnz 6b
    movb $0xFE, %al
    outb %al, $0x64
    jmp 6b
