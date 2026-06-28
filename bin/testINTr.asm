; Free86/ testINTr - real mode interrupt tests
; - run various INTR/ NMI tests
; - output test number on port 0x2a
;
; - load image at segment 0xf000
;
; compile:
;   nasm -f bin testINTr.asm -l testINTr.lst -o testINTr.bin

C_SEG_REAL  equ 0xf000
DEBUG_PORT  equ 0x2a

cpu 386

bits 16
start:
    cli

    push cs
    pop ds

    xor ax, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov byte [bootstrap], 0xf4 ; prepare to HLT after reset on triple fault

    call setup_ivt

    ; Test 1: Hardware Interrupt an INTR (z.B. IRQ)
    call write_test_number      ; → 01 auf Port 0x2A
    sti
    hlt                         ; Warte auf INTR

    ; Test 2: Hardware Interrupt an NMI
test2:
    call write_test_number      ; → 02
    hlt                         ; Warte auf NMI

    ; Test 3: Exception (Divide by Zero)
test3:
    call write_test_number      ; → 03
    mov ax, 1
    xor bx, bx
    div bx                      ; #DE Exception (Vektor 0)

    ; ================================================
    ; Ab hier werden die Tests von den Handlern gesteuert
    ; ================================================

; ------------------------------------------------
; Test 4: Im INTR-Handler weiteren INTR auslösen
intr_handler:
    call write_test_number      ; Test 4 (wird bei verschachteltem INTR aufgerufen)
    ; User triggert **erneut** INTR → Test 4
    iret

; ------------------------------------------------
; Test 5: Im INTR-Handler CLI + weiteren INTR
intr_handler_cli:
    call write_test_number      ; Test 5
    cli
    ; User triggert jetzt INTR → sollte **nicht** ausgeführt werden (CLI)
    ; Danach STI + IRET im nächsten Handler
    iret

; ------------------------------------------------
; Test 6: Im NMI-Handler weiteren NMI
nmi_handler:
    call write_test_number      ; Test 6
    ; User triggert **erneut** NMI
    iret

; ------------------------------------------------
; Test 7: Im INTR-Handler NMI auslösen
intr_handler_nmi:
    call write_test_number      ; Test 7
    ; User triggert NMI
    iret

; ------------------------------------------------
; Test 8: Im INTR-Handler Exception
intr_handler_exception:
    call write_test_number      ; Test 8
    mov ax, 1
    xor bx, bx
    div bx                      ; Exception im Handler
    iret

; ------------------------------------------------
; Test 9: Im NMI-Handler Exception
nmi_handler_exception:
    call write_test_number      ; Test 9
    mov ax, 1
    xor bx, bx
    div bx
    iret

; ------------------------------------------------
; Test 10: Double Fault
double_fault_test:
    call write_test_number      ; Test 10
    ; Wir lösen eine Exception aus, deren Handler wieder eine Exception auslöst
    int 0                       ; #DE, dessen Handler wieder #DE macht
    ; Sollte Double Fault (#DF, Vektor 8) auslösen

; ------------------------------------------------
; Test 11: Triple Fault (sehr schwer zu kontrollieren)
triple_fault_test:
    call write_test_number      ; Test 11
    ; Triple Fault entsteht bei Double Fault im Double-Fault-Handler
    ; In vielen Emulatoren führt das zum Reset

; ================================================================
; Handler Setup (Real Mode IVT)
; ================================================================
setup_ivt:
    xor ax, ax
    mov es, ax

    ; Vektor 0: Divide Error (#DE)
    mov word [es:0x00], div0_handler
    mov word [es:0x02], cs

    ; Vektor 2: NMI
    mov word [es:0x08], nmi_handler_main
    mov word [es:0x0A], cs

    ; Hardware INTR (angenommen Vektor 0x20)
    mov word [es:0x80], intr_handler_main
    mov word [es:0x82], cs

    ; Double Fault (Vektor 8)
    mov word [es:0x20], double_fault_handler
    mov word [es:0x22], cs

    ret

; ================================================================
; Haupt-Handler (mit Test-Steuerung)
; ================================================================

; INTR Haupt-Handler
intr_handler_main:
    ; Test 1 wurde schon beim ersten Aufruf gemacht
    ; Für verschachtelte Tests springen wir gezielt
    cmp byte [test_num], 4
    je intr_handler          ; Test 4
    cmp byte [test_num], 5
    je intr_handler_cli      ; Test 5
    cmp byte [test_num], 7
    je intr_handler_nmi      ; Test 7
    cmp byte [test_num], 8
    je intr_handler_exception; Test 8

    ; Normaler Fall (Test 1)
    call write_test_number
    iret

; NMI Haupt-Handler
nmi_handler_main:
    cmp byte [test_num], 6
    je nmi_handler           ; Test 6
    cmp byte [test_num], 9
    je nmi_handler_exception ; Test 9

    call write_test_number   ; Test 2
    iret

; Divide by Zero Handler
div0_handler:
    cmp byte [test_num], 3
    je .normal_exception
    cmp byte [test_num], 10
    je double_fault_test

.normal_exception:
    call write_test_number
    iret

; Double Fault Handler
double_fault_handler:
    call write_test_number   ; Test 10 (Double Fault)
    ; Für Triple Fault: hier wieder Exception auslösen
    int 0
    iret                     ; Sollte nie ankommen

; ================================================================
; Hilfsroutine: Testnummer auf Port 0x2A schreiben
; ================================================================
write_test_number:
    push ax
    mov al, [test_num]
    out DEBUG_PORT, al
    inc byte [test_num]
    pop ax
    ret

test_num db 1

times 0xfff0-($-$$) nop

bootstrap:
    jmp   C_SEG_REAL:start ; implicitly loads CS with C_SEG_REAL

times 0x10000-($-$$) nop
