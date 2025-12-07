;-----------------------------------------------------------------
; bootstrap.asm - setup 386 emulator for linuxstart.bin execution
;
; compile:
;     nasm -f bin -o bootstrap.bin -l bootstrap.lst bootstrap.asm
;-----------------------------------------------------------------

C_SEG_REAL equ 0xf000
C_SEG_PROT equ 0x08

C_SEG_LINUX equ 0x18
D_SEG_LINUX equ 0x10

linuxstart equ 0x00010000

cpu 386

bits 16
start:
    cli

    ; load GDTR
    o32 lgdt [cs:GDTR]

    ; protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp C_SEG_PROT:flush

bits 32
flush:
    ; prepare state
    mov ax, D_SEG_LINUX
    mov ds, ax
    mov ss, ax

    mov eax, 16 * 1024 * 1024 ; mem_size
    mov ecx, 0x0000f800       ; cmdline_addr
    mov ebx, 2 * 1024 * 1024  ; initrd_size

    jmp C_SEG_LINUX:linuxstart

; ----------------------------------------------------------------
; GDT
; ----------------------------------------------------------------
align 8
GDT_alpha:
    ; selector NULL
    dq 0

    ; CS selector 0x08, base 0xf0000, 32 bit
    dw 0xFFFF
    dw 0x0000
    db 0x0F
    db 0x9A
    db 0x4F
    db 0x00

    ; DS selector 0x10, base 0, 32 bit
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0x4F
    db 0x00

    ; CS selector 0x18, base 0xFFFF0000, 32 bit
    dw 0xFFFF
    dw 0x0000
    db 0xFF
    db 0x9A
    db 0x4F
    db 0xFF
GDT_omega:

GDTR:
    dw GDT_omega - GDT_alpha - 1   ; limit
    dd C_SEG_REAL * 16 + GDT_alpha ; base

; ----------------------------------------------------------------
;   fill-in NOPs until 0xfff0 where execution starts after reset
; ----------------------------------------------------------------
times 0xfff0-($-$$) nop

bits 16
bootstrap:
    jmp   C_SEG_REAL:start
