.code32
.text

.global memset
memset:
    movl %edi, %edx
    cld
    movl 4(%esp), %edi
    movb 8(%esp), %al
    movl 12(%esp), %ecx
    movb %al, %ah
    shrl $1, %ecx
    rep stosw
    adcl %ecx, %ecx
    rep stosb
    movl %edx, %edi
    movl 4(%esp), %eax
    ret

.global memchr
memchr:
    movl %edi, %edx
    cld
    movl 4(%esp), %edi
    movb 8(%esp), %al
    movl 12(%esp), %ecx
    repne scasb
    leal -1(%edi), %eax
    movl %edx, %edi
    je 1f
    xorl %eax, %eax
1:
    ret

.global memcpy
memcpy:
    mov %edi, %eax
    mov %esi, %edx
    cld
    movl 4(%esp), %edi
    movl 8(%esp), %esi
    movl 12(%esp), %ecx
    shrl $1, %ecx
    rep movsw
    adcl %ecx, %ecx
    rep movsb
    movl %edx, %esi
    movl %eax, %edi
    movl 4(%esp), %eax
    ret

.global memmove
memmove:
    mov %edi, %eax
    mov %esi, %edx
    cld
    movl 4(%esp), %edi
    movl 8(%esp), %esi
    movl 12(%esp), %ecx
    cmpl %esi, %edi
    jna 1f
    std
    leal -2(%edi,%ecx,1), %edi
    leal -2(%esi,%ecx,1), %esi
1:
    shrl $1, %ecx
    rep movsw
    adcl %ecx, %ecx
    rep movsb
    movl %edx, %esi
    movl %eax, %edi
    movl 4(%esp), %eax
    cld
    ret

.global strlen
strlen:
    movl %edi, %edx
    movl 4(%esp), %edi
    xorl %eax, %eax
    movl $-1, %ecx
    repne scasb
    notl %ecx
    decl %ecx
    movl %ecx, %eax
    movl %edx, %edi
    ret

.global strupr
strupr:
    movl 4(%esp), %edx
0:
    movb (%edx), %al
    cmpb $'a', %al
    jb 1f
    cmpb $'z', %al
    ja 1f
    subb $32, (%edx)
1:
    incl %edx
    orb %al, %al
    jnz 0b
    movl 4(%esp), %eax
    ret

.global strcpy
strcpy:
    pushl %ebx
    movl 0xc(%esp), %ebx
    movl 0x8(%esp), %edx
0:
    movb (%ebx), %al
    movb %al, (%edx)
    incl %ebx
    incl %edx
    orb %al, %al
    jnz 0b
    movl 0x8(%esp), %eax
    popl %ebx
    ret

.global stricmp
stricmp:
    pushl %ebx
    pushl %ecx
    movl 16(%esp), %ebx
    movl 12(%esp), %edx
    xorl %eax, %eax
0:
    movb (%edx), %al
    cmpb $'a', %al
    jb 1f
    cmpb $'z', %al
    ja 1f
    subb $32, %al
1:
    movb (%ebx), %cl
    cmpb $'a', %cl
    jb 2f
    cmpb $'z', %cl
    ja 2f
    subb $32, %cl
2:
    subb %cl, %al
    jnz 3f
    incl %ebx
    incl %edx

    orb %cl, %cl
    jnz 0b
3:
    popl %ecx
    popl %ebx
    ret

.global strncmp
strncmp:
    pushl %ebx
    movl 0x10(%esp), %ecx
    movl 0xc(%esp), %ebx
    movl 0x8(%esp), %edx
    xorl %eax, %eax
0:
    movb (%edx), %al
    subb (%ebx), %al
    incl %ebx
    incl %edx
    orb %al, %al
    jnz 1f
    cmp $0, -1(%edx)
    jz 1f
    decl %ecx
    jnz 0b
1:
    popl %ebx
    ret

.global strncpy
strncpy:
    pushl %ebx
    movl 0x10(%esp), %ecx
    movl 0xc(%esp), %ebx
    movl 0x8(%esp), %edx
0:
    movb (%ebx), %al
    movb %al, (%edx)
    incl %ebx
    incl %edx
    orb %al, %al
    jz 1f
    decl %ecx
    jnz 0b
1:
    movl 0x8(%esp), %eax
    popl %ebx
    ret

.global strchr
strchr:
    movl 0x4(%esp), %edx
    movb 0x8(%esp), %al
    decl %edx
0:
    incl %edx
    cmpb %al, (%edx)
    je 1f
    cmpb $0, (%edx)
    jne 0b
    xorl %edx, %edx
1:
    movl %edx, %eax
    ret

.global _wcscpy
_wcscpy:
    pushl %ebx
    movl 0xc(%esp), %ebx
    movl 0x8(%esp), %edx
0:
    movw (%ebx), %ax
    movw %ax, (%edx)
    addl $2, %ebx
    addl $2, %edx
    orw %ax, %ax
    jnz 0b
    movl 0x8(%esp), %eax
    popl %ebx
    ret

.global _wcsicmp
_wcsicmp:
    pushl %ebx
    movl 0x10(%esp), %ecx
    movl 0xc(%esp), %ebx
    movl 0x8(%esp), %edx
    xorl %eax, %eax
0:
    movw (%edx), %ax
    cmpw 'a', %ax
    jb 1f
    cmpw 'z', %ax
    ja 1f
    subw $32, %ax
1:
    pushl %ecx
    movw (%ebx), %cx
    cmpw 'a', %cx
    jb 2f
    cmpw 'z', %cx
    ja 2f
    subw $32, %cx
2:
    subw %cx, %ax
    jnz 3f
    popl %ecx
    addl $2, %ebx
    addl $2, %edx

    orw %ax, %ax
    jz 3f

    decl %ecx
    jnz 0b
3:
    popl %ebx
    ret

.global _wcsupr
_wcsupr:
    movl 0x4(%esp), %edx
0:
    movw (%edx), %ax
    cmpw 'a', %ax
    jb 1f
    cmpw 'z', %ax
    ja 1f
    subw $32, (%edx)
1:
    addl $2, %edx
    cmpw $0, %ax
    jne 0b
    movl 0x4(%esp), %eax
    ret

.global _wcschr
_wcschr:
    movl 0x4(%esp), %edx
    movl 0x8(%esp), %eax
    subl $2, %edx
0:
    addl $2, %edx
    cmpw %ax, (%edx)
    je 1f
    cmpw $0, (%edx)
    jne 0b
    xorl %edx, %edx
1:
    movl %edx, %eax
    ret

.global _wcslen
_wcslen:
    movl %edi, %edx
    movl 0x4(%esp), %edi
    xorl %eax, %eax
    movl $-1, %ecx
    repne scasw
    notl %ecx
    decl %ecx
    movl %ecx, %eax
    movl %edx, %edi
    ret
