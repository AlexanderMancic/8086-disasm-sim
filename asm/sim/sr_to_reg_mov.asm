bits 16

mov ax, 1111
mov bx, 2222
mov cx, 3333
mov dx, 4444

mov es, ax
mov cs, bx
mov ss, cx
mov ds, dx

mov sp, es
mov bp, cs
mov si, ss
mov di, ds
