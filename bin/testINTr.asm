; Free86/ testINTr - real mode interrupt tests
; - run various INTR/ NMI tests
; - output test number on port 0x2a
;
; - load image at segment 0xf000
;
; compile:
;   nasm -f bin testINTr.asm -l testINTr.lst -o testINTr.bin

C_SEG_REAL equ 0xf000
DEBUG_PORT equ 0x2a

cpu 386

bits 16
start:
    cli

    push cs
    pop ds

    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00

    call setup_ivt

    sti

    ; test 1: divide by 0 exception (#DE)
    mov ax, 1
    xor bx, bx
    div bx ; cause #DE

test_loop:
    ; test 2: HW interrupt on INTR
    ; test 3: HW interrupt on NMI
    ; test 4: HW interrupt on INTR
    ;         - nested INTR blocked
    ;         - RESET to continue
    ; test 5: divide by 0 exception (#DE)
    ; test 6: HW interrupt on INTR (sti in ISR)
    ; test 7: nested INTR allowed
    ; test 8: HW interrupt on NMI
    ;         - nested NMI blocked
    ;         - nested INTR blocked
    ;         - RESET to continue
    ; test 9: divide by 0 exception (#DE)
    ;         - nested INTR blocked
    ;         - NMI to continue
    ; test 10: HW interrupt on INTR
    ; test 11: nested #DE allowed
    ; test 12: HW interrupt on NMI
    ; test 13: nested #DE allowed
    inc byte [test_number]
    hlt ; wait for interrupt/ exception to run test
    cmp byte [test_number], 13
    jne test_loop

    ; test 14: double fault
    lidt [double_fault_idtr]
    hlt ; wait for INT 13 (any id > 8 will do)

    ; test 15: triple fault
    inc byte [test_number]
    call write_test_number
    lidt [triple_fault_idtr]
    mov eax, 0xdeadc0de
    hlt ; wait for INT 13 (any id > 7 will do)
    ; not reached

double_fault_idtr: ; any id > 8 causes GP yielding DF
    dw 0x0024
    dd 0x00000000
triple_fault_idtr: ; any id > 7 causes GP yielding DF failing causing triple fault
    dw 0x0020
    dd 0x00000000

intr_test4:
nmi_test8:
    call write_test_number
    hlt ; wait for nested INTR
    ; not reached due to RESET

intr_test6:
    call write_test_number
    inc byte [test_number]
    sti
    hlt ; wait for nested INTR
    iret

div0_test9:
    call write_test_number
    add dword [esp], 2 ; adjust EIP
    hlt ; wait for nested interrupts
    iret

intr_test10:
nmi_test12:
    call write_test_number
    mov ax, 1
    xor bx, bx
    div bx ; cause #DE
    iret

setup_ivt:
    xor ax, ax
    mov es, ax

    ; vector 0: divide error (#DE)
    mov word [es:0x00], div0_dispatcher
    mov word [es:0x02], cs

    ; vector 2: hardware NMI
    mov word [es:0x08], nmi_dispatcher
    mov word [es:0x0A], cs

    ; vector 8: double fault (#DF)
    mov word [es:0x20], double_fault_handler
    mov word [es:0x22], cs

    ; vector 32: hardware INTR
    mov word [es:0x80], intr_dispatcher
    mov word [es:0x82], cs

    ret

; #DE ISR (vector 0)
div0_dispatcher:
    inc byte [test_number]
    cmp byte [test_number], 9
    je div0_test9
    ; fallthrough
    call write_test_number
    add dword [esp], 2 ; adjust EIP
    iret ; tests 1, 5, 11, 13

; NMI ISR (vector 2)
nmi_dispatcher:
    cmp byte [test_number], 8
    je nmi_test8
    cmp byte [test_number], 12
    je nmi_test12
    ; fallthrough
    call write_test_number
    iret ; test 3, 9

; #DF ISR (vector 8)
double_fault_handler:
    inc byte [test_number]
    call write_test_number
    iret

; INTR ISR (vector 32)
intr_dispatcher:
    cmp byte [test_number], 4
    je intr_test4
    cmp byte [test_number], 6
    je intr_test6
    cmp byte [test_number], 10
    je intr_test10
    ; fallthrough
    call write_test_number
    iret ; test 2, 7, 9

write_test_number:
    push ax
    mov al, [test_number]
    out DEBUG_PORT, al
    pop ax
    ret

test_number db 0

times 0xfff0-($-$$) nop

bootstrap:
    jmp C_SEG_REAL:start ; implicitly loads CS with C_SEG_REAL

times 0x10000-($-$$) nop
