;-----------------------------------------------------------------
; bootstrap.asm - setup 386 emulator for linuxstart.bin execution
;
;             compile: nasm -f bin -o bootstrap.bin bootstrap.asm
;-----------------------------------------------------------------

CS_REAL    equ 0xf000

DS         equ 0x10
CS         equ 0x08
LINUXSTART equ 0x00010000

cpu 386
section .text

bits 16

; ----------------------------------------------------------------
; GDT at physical address 0x00000800
; ----------------------------------------------------------------
org 0x00000800
gdtA:
    ; descriptor 0: null
    dq 0

    ; descriptor 1: code segment (selector 0x08)
    ; base=0x00000000, limit=0xFFFFF, access=0x9A (code, exec/read, present),
    ; granularity=0xCF (4K, 32 bit default)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

    ; descriptor 2: data segment (selector 0x10)
    ; base=0x00000000, limit=0xFFFFF, access=0x92 (data, writable, present)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdtO:

; ----------------------------------------------------------------
; GDTR at physical address 0x00000900
; ----------------------------------------------------------------
org 0x00000900
gdtr:
    dw gdtO - gdtA - 1 ; limit
    dd gdtA               ; base

org 0x00000A00
bootstrap:
    cli

    ; load GDTR
    lgdt [gdtr] ; reads 6 bytes from 0x900

    ; switch to protected mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ; manually assembled far jump in PM which is not available in 16 bit mode.
    db 0x66 ; operand-size override prefix
    db 0xEA ; JMPF opcode
    dd setup386 ; 32 bit offset
    dw CS       ; CS 0x08

org 0x00000B00
bits 32
setup386:
    ; prepare state for linuxstart.bin

    ; CS contains selector 0x08 from far jump
    ; load data selectors with selector 0x10
    mov ax, DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax

    mov eax, 16 * 1024 * 1024 ; mem_size
    mov ebx, 0x0000f800       ; cmdline_addr
    mov ecx, 2 * 1024 * 1024  ; initrd_size

    jmp CS:LINUXSTART

; ----------------------------------------------------------------
;   fill-in NOPs until 0xfff0 where execution starts after reset
; ----------------------------------------------------------------
times 0xfff0-($-$$) nop

bits 16

resetIP: ; 0x0000fff0
    jmp   CS_REAL:bootstrap
