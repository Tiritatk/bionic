bits 32
extern irq_handler

%macro IRQ 1
global irq%1
irq%1:
    cli
    push byte %1
    jmp irq_common
%endmacro

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

irq_common:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, [esp + 36]    ; recupera el número de IRQ (por encima de pusha + ds)
    push eax               ; lo pasa como argumento a irq_handler
    call irq_handler
    add esp, 4             ; limpia el argumento

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 4             ; limpia el número de IRQ original
    sti
    iret