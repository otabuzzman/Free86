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

    call setup_ivt

    sti

    cmp byte [test_cli], 1
    je test5

    ; test 1: hardware interrupt on INTR
    hlt         ; wait for INTR
    ; test 2: hardware interrupt on NMI
    hlt         ; wait for INTR
    ; test 3: divide by 0 exception (#DE)
    mov ax, 1
    xor bx, bx
    div bx      ; cause #DE
    ; test 4: nested INTR blocked (default)
    hlt         ; wait for INTR
test5:
    ; test 5: nested INTR allowed (sti)
    hlt         ; wait for INTR



    hlt ; debug



    ; HLT after reset on triple fault
    mov byte [bootstrap], 0xf4

intr_handler:      ; in test 4
    inc byte [test_cli]
    hlt ; wait for nested INTR
    iret

intr_handler_sti:  ; in test 5
    sti
    hlt ; wait for nested INTR
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

; INTR ISR
intr_handler_main:
    call write_test_number
    cmp byte [test_num], 4
    je intr_handler           ; test 4
    cmp byte [test_num], 5
    je intr_handler_sti       ; test 5
    cmp byte [test_num], 7
    je intr_handler_nmi       ; test 7
    cmp byte [test_num], 8
    je intr_handler_exception ; test 8
    ; fallthrough
    iret                      ; test 1

; NMI ISR
nmi_handler_main:
    call write_test_number
    cmp byte [test_num], 6
    je nmi_handler            ; test 6
    cmp byte [test_num], 9
    je nmi_handler_exception  ; test 9
    ; fallthrough
    iret                      ; test 2

; #DE ISR
div0_handler:
    call write_test_number
    cmp byte [test_num], 10
    je double_fault_test
    ; fallthrough
    add dword [esp], 2        ; adjust EIP
    iret                      ; test 3

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
test_cli db 0

times 0xfff0-($-$$) nop

bootstrap:
    jmp   C_SEG_REAL:start ; implicitly loads CS with C_SEG_REAL

times 0x10000-($-$$) nop
