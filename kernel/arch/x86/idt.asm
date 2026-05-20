; =============================================================================
; x86 Interrupt Descriptor Table (IDT) stubs
;
; Each interrupt handler must push an error code (or 0) and the interrupt
; number, then jump to a common handler. The common handler saves all
; registers and calls the C interrupt dispatcher.
; =============================================================================

[BITS 32]
[GLOBAL isr0]
[GLOBAL isr1]
[GLOBAL isr_irq0]
[GLOBAL isr_irq1]
[GLOBAL isr_irq_common]
[EXTERN isr_handler]
[EXTERN irq_handler]

; ── Exception ISRs (0-31) ────────────────────────────────────────────────────
; No error code: push 0
isr0:
    push dword 0
    push dword 0
    jmp isr_common

; Keyboard: exception with no error code
isr1:
    push dword 0
    push dword 1
    jmp isr_common

; Common ISR handler: save state and call C handler
isr_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10    ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8      ; skip error code and ISR number
    iret

; ── Hardware IRQ handlers (32-47) ─────────────────────────────────────────────
; IRQ0 (timer, remapped to ISR 32)
isr_irq0:
    push dword 0
    push dword 32
    jmp isr_irq_common

; IRQ1 (keyboard, remapped to ISR 33)
isr_irq1:
    push dword 0
    push dword 33
    jmp isr_irq_common

; Common IRQ handler
isr_irq_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call irq_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret
