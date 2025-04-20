bits 16

; taken from listing 45
mov ax, 0x2222
mov bx, 0x4444
mov cx, 0x6666
mov dx, 0x8888

mov ss, ax
mov ds, bx
mov es, cx

; cs mov
mov cs, dx
