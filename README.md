# miniOS

Mini SO educativo escrito desde cero en C + ensamblador x86. Arranca en QEMU sin ningún bootloader externo (sin GRUB).

---

## Requisitos

```bash
# Ubuntu / WSL
sudo apt install nasm gcc-multilib binutils qemu-system-x86
```

| Herramienta | Para qué |
|---|---|
| `nasm` | Ensamblar el bootloader y el entry point del kernel |
| `gcc -m32` / `i686-elf-gcc` | Compilar el kernel C para i386 bare-metal |
| `ld` / `i686-elf-ld` | Linkar el kernel al mapa de memoria correcto |
| `objcopy` | Convertir ELF → binario plano |
| `qemu-system-i386` | Emular el PC |

---

## Compilar y ejecutar

```bash
make            # construye miniOS.img
make run        # lanza QEMU con el disco
make run-debug  # QEMU + servidor GDB en localhost:1234
make clean      # borra build/ y miniOS.img
```

---

## Estructura del proyecto

```
miniOS/
├── boot/
│   └── boot.asm          # Bootloader MBR (512 bytes)
├── kernel/
│   ├── arch/x86/
│   │   ├── start.asm     # Entry point 32-bit del kernel
│   │   ├── idt.asm       # ISR stubs (interrupts)
│   │   ├── idt.c         # IDT setup
│   │   ├── idt.h         # IDT interface
│   │   ├── pic.c         # PIC 8259A init + EOI
│   │   └── pic.h         # PIC interface
│   ├── drivers/
│   │   ├── vga.h         # Interfaz del driver VGA
│   │   ├── vga.c         # Driver texto 80×25
│   │   ├── keyboard.h    # Interfaz del driver de teclado
│   │   └── keyboard.c    # Keyboard IRQ handler
│   ├── include/
│   │   └── types.h       # uint8_t, uint32_t… sin libc
│   └── kernel.c          # kernel_main — punto de entrada en C
├── linker.ld             # Script de linkado: kernel en 0x10000
├── Makefile              # Build completo → imagen → QEMU
└── README.md
```

---

## Mapa de memoria al arrancar

```
0x00000000 – 0x000004FF   IVT + BDA (BIOS)
0x00007C00 – 0x00007DFF   Bootloader MBR  (este código)
0x00010000 – …            Kernel cargado aquí
0x0009FC00                Stack del kernel (crece hacia abajo)
0x000A0000 – 0x000BFFFF   Memoria de vídeo VGA
0x000B8000                Buffer de texto VGA (80×25)
```

---

## Flujo de arranque paso a paso

```
BIOS
 └─ carga sector 0 (MBR) en 0x7C00 y salta a él

boot/boot.asm  (modo real 16-bit)
 ├─ 1. Inicializa segmentos y stack en 0x7C00
 ├─ 2. INT 13h → lee 64 sectores del disco a 0x10000
 ├─ 3. INT 15h AX=2401 → habilita la línea A20
 ├─ 4. LGDT → carga la GDT (null | code 0x08 | data 0x10)
 ├─ 5. CR0.PE = 1 → entra en modo protegido 32-bit
 └─ 6. JMP FAR 0x08:0x10000

kernel/arch/x86/start.asm  (modo protegido 32-bit)
 ├─ Recarga todos los selectores de segmento con 0x10
 ├─ Pone ESP en 0x9FC00
 ├─ Zeroes el segmento BSS
 └─ CALL kernel_main

kernel/kernel.c
 └─ Imprime banner + checklist de fases por VGA y hace HLT
```

---

## Archivos clave explicados

### `boot/boot.asm`

El MBR ocupa exactamente 512 bytes y termina con la firma `0xAA55`.

| Sección | Qué hace |
|---|---|
| `_start` | Init de segmentos, stack, guarda el drive de arranque en `boot_drive` |
| Carga de disco | `INT 13h AH=02h` — lee en CHS (cilindro 0, cabezal 0, sector 2+) |
| A20 | `INT 15h AX=2401h` — habilita el bus de direcciones de 21 bits |
| GDT | Tres descriptores: null, código (0x08), datos (0x10), modelo plano 4 GB |
| `gdt_desc` | Registro que apunta `LGDT` — contiene tamaño y dirección de la GDT |
| `pm_entry` | Ya en 32-bit: recarga selectores y salta al kernel |

### `kernel/arch/x86/start.asm`

Primer código que ejecuta el kernel. Hace las tareas que el bootloader no puede hacer en C:
- `rep stosb` sobre `_bss_start`…`_bss_end` para garantizar que las variables globales empiezan a cero.
- Llama a `kernel_main`. Si retornase, entra en loop `hlt`.

### `linker.ld`

Dice al linker que el binario se ejecuta en `0x10000`.  
Pone `.text.entry` primero para que `_start` de `start.asm` esté exactamente al comienzo del binario — es el byte al que salta el bootloader.  
Define los símbolos `_bss_start` y `_bss_end` que usa `start.asm`.

### `kernel/drivers/vga.c`

Accede directamente al buffer de hardware en `0xB8000`.  
Cada celda son 2 bytes: `[carácter ASCII | atributo de color]`.  
Implementa scroll desplazando filas con memmove manual (sin libc).

### `kernel/include/types.h`

Reemplaza `<stdint.h>` porque compilamos con `-nostdinc` (sin cabeceras del sistema operativo host — no las queremos en un kernel bare-metal).

---

## Fases del proyecto

| # | Estado | Componente |
|---|---|---|
| 1 | ✅ | Bootloader MBR + modo protegido + VGA |
| 2 | ✅ | IDT + PIC 8259A + driver de teclado |
| 3 | ⬜ | Paginación x86 + `kmalloc` |
| 4 | ⬜ | Scheduler round-robin + PCB + context switch |
| 5 | ⬜ | Syscalls: `write`, `read`, `fork`, `exec`, `exit` |
| 6 | ⬜ | VFS + initrd (sistema de archivos en memoria) |
| 7 | ⬜ | Shell rudimentario |

---

## Phase 2: IDT + PIC + Keyboard

### IDT (Interrupt Descriptor Table)

- `kernel/arch/x86/idt.asm` — stubs para ISR0-ISR1 (excepciones) e IRQ0-IRQ1 (hardware)
- `kernel/arch/x86/idt.c` — carga GDT, registra handlers
- Cada ISR guarda todos los registros, llama al handler en C, y restaura con `iret`

### PIC (8259A Programmable Interrupt Controller)

- `kernel/arch/x86/pic.c` — inicializa PIC master + slave en modo cascada
- Remapea IRQ0-7 → ISR 32-39, IRQ8-15 → ISR 40-47
- Habilita solo IRQ1 (teclado) en esta fase
- `pic_eoi()` envía End-of-Interrupt al PIC

### Keyboard Driver

- `kernel/drivers/keyboard.c` — handler IRQ1
- Lee scancode del puerto 0x60
- Traduce a ASCII (qwerty, números, Enter, Backspace…)
- Imprime carácter en VGA

### Flujo de una pulsación

```
Teclado → IRQ1 → PIC → CPU → IDT[33] → isr_irq1
  → stack saved → irq_handler(33) → keyboard_handler()
  → inb(0x60) → scancode_to_ascii → vga_putchar()
  → pic_eoi(1) → iret → restora registros
```

---

## Depuración con GDB

```bash
# Terminal 1
make run-debug          # QEMU queda esperando en el primer byte

# Terminal 2
gdb
(gdb) target remote localhost:1234
(gdb) set architecture i386
(gdb) symbol-file build/kernel.elf
(gdb) break kernel_main
(gdb) continue
```

---

## Referencias

- [OSDev Wiki](https://wiki.osdev.org) — referencia principal de desarrollo de kernels
- *Operating Systems: Three Easy Pieces* — Arpaci-Dusseau (free online)
- Intel IA-32 Software Developer's Manual Vol. 3 — para GDT, IDT, paginación
