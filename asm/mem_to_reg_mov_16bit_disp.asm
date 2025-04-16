; taken from listing 39
; Source address calculation plus 8-bit displacement
mov ah, [bx + si + 4999]

; more test instructions
mov dx, [bp + 323]
mov al, [bx + si + 32767]
mov bx, [bp + di + 6933]
mov ah, [bx + di + 2345]
mov cx, [bp + si + 1299]
mov dl, [si + 3993]
mov bh, [bx + 9999]

; negative displacements
mov al, [bx + si - 129]
mov bx, [bp + di - 32768]
mov ah, [bx + di - 333]
mov cx, [bp + si - 19889]
