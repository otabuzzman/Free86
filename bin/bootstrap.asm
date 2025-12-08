;----------------------------------------------------------------------
; bootstrap.asm - setup 386 emulator for linuxstart.bin execution
;
; compile:
;     nasm -f bin -o bootstrap.bin -l bootstrap.lst bootstrap.asm
;
; linuxstart.bin expects a specific CPU state. The original code
; sets this state by writing values directly to the corresponding
; registers.
;
; This state can also be set by a program (like this one) that starts
; directly from the CPU reset state, in real mode, at address 0xffff0.
;----------------------------------------------------------------------

C_SEG_REAL equ 0xf000
C_SEG_PROT equ 0x08

C_SEG_LINUX equ 0x10
D_SEG_LINUX equ 0x18

linuxstart equ 0x00010000

cpu 386

bits 16
start:
    cli ; common practice

    ; load GDTR
    o32 lgdt [cs:GDTR]

    ; protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp C_SEG_PROT:flush ; implicitly loads PM CS with selector 0x08
                         ; with its base 0xf0000 corresponding to
                         ; real mode CS and thus asures that PM
                         ; continues execution in same address space.

bits 32
flush:
    ; prepare state
    mov ax, D_SEG_LINUX
    mov ds, ax
    mov ss, ax

    mov eax, 16 * 1024 * 1024 ; mem_size
    mov ecx, 0x0000f800       ; cmdline_addr
    mov ebx, 2 * 1024 * 1024  ; initrd_size

    jmp C_SEG_LINUX:linuxstart ; implicitly reloads PM CS with
                               ; selector 0x10 with base 0xffff0000,
                               ; the value after reset and expected
                               ; by linuxstart.bin.
    ; end of prepare state

;----------------------------------------------------------------------
; Global Descriptor Table (GDT)
;----------------------------------------------------------------------
align 8
GDT_alpha:
    ; 1st descriptor table entry (dte), selector NULL
    dq 0

    ; 2nd dte, CS selector 0x08, base 0x000f0000, 32 bit, executable
    dw 0xFFFF
    dw 0x0000
    db 0x0F
    db 0x9A
    db 0x4F
    db 0x00

    ; 3rd dte, CS selector 0x10, base 0xffff0000, 32 bit, executable
    dw 0xFFFF
    dw 0x0000
    db 0xFF
    db 0x9A
    db 0x4F
    db 0xFF

    ; 4th dte, DS selector 0x18, base 0, 32 bit, writable
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0x4F
    db 0x00
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
    jmp   C_SEG_REAL:start ; implicitly loads CS with C_SEG_REAL
