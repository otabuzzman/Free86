; Free86/ testINT
; - loop on wait for INT/ NMI
; - output vector number on port 2a
;
; - load 64k image at segment f000
; - interrupt vector at f000:fff0
;
; compile:
;   nasm -f bin testINT.asm -l testINT.lst -o testINT.bin

C_SEG_REAL  equ 0xf000
DEBUG_PORT  equ 0x2a

cpu 386

bits 16
start:
    cli
    cld

    ; set stack
    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00

    ; set data to point at code segment
    push cs
    pop ds

    ; set ES (stosw) to point at physical address 0
    xor ax, ax
    mov es, ax

    ; copy IVT to physical address 0
    xor di, di          ; destination ES:di (stosw)
    mov si, ivt         ; source DS:si (lodsw)
    mov cx, 256

.copy_ivt:
    lodsw
    stosw
    mov ax, cs
    stosw
    loop .copy_ivt

    lidt [idtr]

    sti

.loop:
    nop                 ; wait for next INT/ NMI
    jmp .loop

idtr:
    dw 0x03ff
    dd 0x00000000

; ------------------------------------------------------------
; offset table for 256 interrupt service routines (ISR)
; ------------------------------------------------------------

ivt:
%assign i 0
%rep 256
    dw isr%+i
%assign i i+1
%endrep

; ------------------------------------------------------------
; 256 ISRs (aka interrupt handler)
; ------------------------------------------------------------

%assign i 0
%rep 256
isr%+i:
    push ax
    push dx

    mov dx, DEBUG_PORT
    mov al, i
    out dx, al

    pop dx
    pop ax
    iret
%assign i i+1
%endrep

times 0xfff0-($-$$) nop

bootstrap:
    jmp   C_SEG_REAL:start ; implicitly loads CS with C_SEG_REAL

times 0x10000-($-$$) nop
