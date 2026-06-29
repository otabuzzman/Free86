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

    ; prepare to HLT after reset on triple fault
    mov byte [bootstrap], 0xf4

    call setup_ivt

    sti

    ; test 1: hardware interrupt on INTR
    hlt                         ; wait for INTR

    ; test 2: hardware interrupt on NMI
    hlt                         ; wait for NMI

    ; test 3: divide by 0 exception (#DE)
    mov ax, 1
    xor bx, bx
    div bx                      ; #DE

    ; test 4: nested INTR allowed (sti)
    hlt                         ; wait for INTR

intr_handler:
    sti
    hlt                         ; wait for nested INTR
    iret

    ; test 5: Im INTR-Handler CLI + weiteren INTR
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
    call write_test_number
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
    add dword [esp], 2       ; adjust EIP
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
    inc byte [test_num]
    mov al, [test_num]
    out DEBUG_PORT, al
    pop ax
    ret

test_num db 0

times 0xfff0-($-$$) nop

bootstrap:
    jmp   C_SEG_REAL:start ; implicitly loads CS with C_SEG_REAL

times 0x10000-($-$$) nop
