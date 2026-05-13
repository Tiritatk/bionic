bits 32
extern isr_handler

; Macro para ISRs sin error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push byte 0      ; error code ficticio
    push byte %1     ; número de interrupción
    jmp isr_common
%endmacro

; Macro para ISRs con error code (la CPU ya lo puso en el stack)
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push byte %1
    jmp isr_common
%endmacro

ISR_NOERR 0   ; Division by zero
ISR_NOERR 1   ; Debug
ISR_NOERR 2   ; NMI
ISR_NOERR 3   ; Breakpoint
ISR_NOERR 4   ; Overflow
ISR_NOERR 5   ; Bound range exceeded
ISR_NOERR 6   ; Invalid opcode
ISR_NOERR 7   ; Device not available
ISR_ERR   8   ; Double fault
ISR_NOERR 9   ; Coprocessor segment overrun
ISR_ERR   10  ; Invalid TSS
ISR_ERR   11  ; Segment not present
ISR_ERR   12  ; Stack fault
ISR_ERR   13  ; General protection fault
ISR_ERR   14  ; Page fault
ISR_NOERR 15  ; Reserved

; Stub común: guarda registros, llama a C, restaura
isr_common:
    pusha                  ; guarda eax,ecx,edx,ebx,esp,ebp,esi,edi
    mov ax, ds
    push eax               ; guarda el selector de datos
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call isr_handler       ; llama al manejador en C
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8             ; limpia int_no y err_code del stack
    sti
    iret