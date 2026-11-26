/* SysV AMD64: rdi = bytes, rsi = remaining bytes; return eax = 0/1.
 * No load may precede its corresponding length check. */
.text
.globl utf8_sse2
.type utf8_sse2,@function
utf8_sse2:
.Lnext:
    cmp $16,%rsi
    jb .Lscalar
    movdqu (%rdi),%xmm0
    pmovmskb %xmm0,%eax
    test %eax,%eax
    jnz .Lprefix
    add $16,%rdi
    sub $16,%rsi
    jmp .Lnext
.Lprefix:
    /* All preceding bytes are ASCII; consume them without rechecking. */
    bsf %eax,%eax
    add %rax,%rdi
    sub %rax,%rsi
    jmp .Llead
.Lscalar:
    /* Stay scalar through adjacent multibyte characters; switch on ASCII. */
    test %rsi,%rsi
    jz .Lgood
.Llead:
    movzbl (%rdi),%eax
    cmp $0x80,%eax
    jb .Ladvance1
    cmp $0xc2,%eax
    jb .Lbad
    cmp $0xe0,%eax
    jb .Ltwo
    cmp $0xf0,%eax
    jb .Lthree
    cmp $0xf4,%eax
    ja .Lbad
    cmp $4,%rsi
    jb .Lbad
    movzbl 1(%rdi),%edx
    cmp $0xf0,%eax
    jne .Lnot_f0
    cmp $0x90,%edx
    jb .Lbad
.Lnot_f0:
    cmp $0xf4,%eax
    jne .Lfour_tail
    cmp $0x8f,%edx
    ja .Lbad
.Lfour_tail:
    sub $0x80,%edx
    cmp $0x3f,%edx
    ja .Lbad
    movzbl 2(%rdi),%edx
    sub $0x80,%edx
    cmp $0x3f,%edx
    ja .Lbad
    movzbl 3(%rdi),%edx
    sub $0x80,%edx
    cmp $0x3f,%edx
    ja .Lbad
    add $4,%rdi
    sub $4,%rsi
    jmp .Lscalar
.Lthree:
    cmp $3,%rsi
    jb .Lbad
    movzbl 1(%rdi),%edx
    cmp $0xe0,%eax
    jne .Lnot_e0
    cmp $0xa0,%edx
    jb .Lbad
.Lnot_e0:
    cmp $0xed,%eax
    jne .Lthree_tail
    cmp $0x9f,%edx
    ja .Lbad
.Lthree_tail:
    sub $0x80,%edx
    cmp $0x3f,%edx
    ja .Lbad
    movzbl 2(%rdi),%edx
    sub $0x80,%edx
    cmp $0x3f,%edx
    ja .Lbad
    add $3,%rdi
    sub $3,%rsi
    jmp .Lscalar
.Ltwo:
    cmp $2,%rsi
    jb .Lbad
    movzbl 1(%rdi),%edx
    sub $0x80,%edx
    cmp $0x3f,%edx
    ja .Lbad
    add $2,%rdi
    sub $2,%rsi
    jmp .Lscalar
.Ladvance1:
    inc %rdi
    dec %rsi
    jmp .Lnext
.Lgood:
    mov $1,%eax
    ret
.Lbad:
    xor %eax,%eax
    ret
.size utf8_sse2,.-utf8_sse2
.section .note.GNU-stack,"",@progbits
