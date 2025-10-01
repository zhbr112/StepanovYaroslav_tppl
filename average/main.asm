section .data
    filename db 'data.txt', 0
    N equ 7

    msg db 'Mean of differences: '
    len_msg equ $ - msg
    newline db 10

    err_open db 'Error: Cannot open data.txt!', 10
    len_err_open equ $ - err_open

section .bss
    file_descriptor resq 1
    file_buffer resb 200
    
    x resd N
    y resd N

    int_buffer resb 12

section .text
global _start

_start:
    mov rax, 2
    mov rdi, filename
    xor rsi, rsi
    xor rdx, rdx
    syscall
    
    mov [file_descriptor], rax

    mov rax, 0
    mov rdi, [file_descriptor]
    mov rsi, file_buffer
    mov rdx, 200
    syscall

    mov rax, 3
    mov rdi, [file_descriptor]
    syscall

    mov r12, file_buffer
    
    mov rdi, x
    call parse_array
    
    mov rdi, y
    call parse_array
    

    xor rbx, rbx
    xor rcx, rcx

loop_calc:
    mov eax, [x + rcx * 4]
    sub eax, [y + rcx * 4]
    add ebx, eax
    inc rcx
    cmp rcx, N
    jl loop_calc

    mov eax, ebx
    mov edi, N
    cdq
    idiv edi

    movsx rax, eax

    mov r12, rax

    mov rax, 1
    mov rdi, 1
    mov rsi, msg
    mov rdx, len_msg
    syscall

    mov rdi, r12
    call print_int

    mov rax, 1
    mov rdi, 1
    mov rsi, newline
    mov rdx, 1
    syscall

    jmp exit

exit:
    mov rax, 60
    xor rdi, rdi
    syscall

parse_array:
    mov rcx, 0

.parse_loop:
    xor rax, rax

.number_loop:
    movzx r8, byte [r12]
    inc r12

    cmp r8b, '0'
    jl .end_number
    cmp r8b, '9'
    jg .end_number
    
    sub r8b, '0'
    imul rax, 10
    add rax, r8
    jmp .number_loop

.end_number:
    mov [rdi + rcx * 4], eax
    inc rcx
    cmp rcx, N
    jl .parse_loop
    ret

print_int:
    mov rsi, int_buffer + 10
    mov byte [rsi + 1], 0
    mov rax, rdi
    mov rbx, 10
    xor r9, r9

    cmp rax, 0
    je .zero

    jns .positive
    
    mov r9, 1
    neg rax

.positive:
.digit_loop:
    xor rdx, rdx
    div rbx
    add rdx, '0'
    mov [rsi], dl
    dec rsi
    cmp rax, 0
    jne .digit_loop

    cmp r9, 1
    jne .print
    mov byte [rsi], '-'
    dec rsi
    jmp .print

.zero:
    mov byte [rsi], '0'
    dec rsi

.print:
    inc rsi
    mov rdx, int_buffer + 11
    sub rdx, rsi
    
    mov rax, 1
    mov rdi, 1
    syscall
    ret