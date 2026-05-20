; =============================================================================
; Kernel 32-bit entry point
;
; The bootloader jumps here (physical 0x10000).
; We zero BSS, set up a fresh kernel stack, then call kernel_main.
; =============================================================================

[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]
[EXTERN _bss_start]
[EXTERN _bss_end]

section .text.entry

_start:
    cld
    cli

    ; ── Zero BSS ─────────────────────────────────────────────────────────────
    mov  edi, _bss_start
    mov  ecx, _bss_end
    sub  ecx, edi
    xor  eax, eax
    rep  stosb

    ; ── Call kernel, never returns ────────────────────────────────────────────
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
