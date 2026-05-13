bits 32
global gdt_flush

gdt_flush:
    mov eax, [esp+4]   ; puntero a gdt_ptr
    lgdt [eax]         ; carga la GDT

    ; Recarga los registros de segmento con el selector de datos (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump para recargar CS con el selector de código (0x08)
    jmp 0x08:.flush
.flush:
    ret