; =============================================================================
; miniOS Stage-1 Bootloader  (MBR, 512 bytes)
;
; Memory layout after boot:
;   0x00007C00  this code
;   0x00010000  kernel loaded here  (segment 0x1000 : offset 0x0000)
;   0x0009FC00  stack top
;
; Flow:
;   1. Initialise segments & stack
;   2. Load kernel sectors from disk with BIOS INT 13h
;   3. Enable A20 via BIOS INT 15h AX=2401h
;   4. Load a flat-model GDT
;   5. Set CR0.PE → protected mode
;   6. Far-jump into 32-bit code, reload segments, call kernel
; =============================================================================

[BITS 16]
[ORG  0x7C00]

%define KERNEL_SEG      0x1000      ; physical 0x10000
%define KERNEL_SECTORS  64          ; 64 × 512 = 32 KB max kernel

; -----------------------------------------------------------------------------
; Entry point
; -----------------------------------------------------------------------------
_start:
    cli
    xor  ax, ax
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    mov  sp, 0x7C00                 ; stack grows down from 0x7C00
    sti

    mov  [boot_drive], dl           ; BIOS passes boot drive in DL

    mov  si, msg_boot
    call print16

    ; ── Reset disk controller ────────────────────────────────────────────────
    mov  ah, 0x00
    mov  dl, [boot_drive]
    int  0x13
    jc   .disk_error

    ; ── Load kernel via CHS  (cyl=0, head=0, sector=2) ──────────────────────
    mov  ax, KERNEL_SEG
    mov  es, ax
    xor  bx, bx                     ; ES:BX → 0x10000

    mov  ah, 0x02                   ; INT 13h: read sectors
    mov  al, KERNEL_SECTORS
    mov  ch, 0                      ; cylinder 0
    mov  cl, 2                      ; sector 2  (sector 1 = MBR)
    mov  dh, 0                      ; head 0
    mov  dl, [boot_drive]
    int  0x13
    jc   .disk_error

    ; ── Enable A20 (BIOS method) ─────────────────────────────────────────────
    mov  ax, 0x2401
    int  0x15

    ; ── Switch to 32-bit protected mode ──────────────────────────────────────
    cli
    lgdt [gdt_desc]

    mov  eax, cr0
    or   eax, 0x1                   ; set PE bit
    mov  cr0, eax

    ; Far jump flushes the instruction pipeline and loads CS = selector 0x08
    jmp  0x08:pm_entry

.disk_error:
    mov  si, msg_err
    call print16
    cli
    hlt

; ── BIOS teletype ─────────────────────────────────────────────────────────────
print16:
    mov  ah, 0x0E
    mov  bh, 0
.loop:
    lodsb
    test al, al
    jz   .done
    int  0x10
    jmp  .loop
.done:
    ret

; =============================================================================
; 32-bit protected-mode entry  (still assembled in the same 512-byte MBR)
; =============================================================================
[BITS 32]
pm_entry:
    mov  ax, 0x10                   ; flat data selector (GDT entry 2)
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax
    mov  esp, 0x9FC00               ; stack top below EBDA

    ; Jump to kernel entry point at physical 0x10000
    jmp  0x08:0x10000

; =============================================================================
; Flat-model GDT  (null | 32-bit code | 32-bit data)
; =============================================================================
align 8
gdt_start:
    dq 0x0000000000000000           ; 0x00  null descriptor

    ; 0x08  code  base=0 limit=4GB  ring-0  32-bit  exec/read
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00

    ; 0x10  data  base=0 limit=4GB  ring-0  32-bit  read/write
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ── Data ─────────────────────────────────────────────────────────────────────
boot_drive: db 0
msg_boot:   db "miniOS: loading kernel...", 0x0D, 0x0A, 0
msg_err:    db "FATAL: disk read error", 0x0D, 0x0A, 0

; ── Boot signature ────────────────────────────────────────────────────────────
times 510 - ($ - $$) db 0
dw 0xAA55
