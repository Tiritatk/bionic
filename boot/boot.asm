MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
VIDEO    equ 1 << 2
FLAGS    equ MBALIGN | MEMINFO | VIDEO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

    ; Campos a.out kludge de Multiboot v1.
    ; Aunque no los usemos, deben estar presentes antes del modo de vídeo.
    dd 0    ; header_addr
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr

    ; Video mode request
    dd 0       ; mode_type: 0 = graphics
    dd 1920    ; width
    dd 1080    ; height
    dd 32      ; depth

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
bits 32
global _start
_start:
    mov esp, stack_top
    and esp, 0xFFFFFFF0
    push 0
    popf
    push ebx           ; puntero a la estructura multiboot (argumento 2)
    push eax           ; magic number de multiboot    (argumento 1)
    extern kernel_main
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang