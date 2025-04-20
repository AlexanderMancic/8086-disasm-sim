bits 16

; taken from listing 39
; Source address calculation plus 8-bit displacement
mov ah, [bx + si + 4]

; more test instructions
mov dx, [bp]
mov dx, [bp + 3]
mov al, [bx + si + 127]
mov bx, [bp + di + 69]
mov ah, [bx + di + 0]
mov cx, [bp + si + 12]
mov dl, [si + 33]
mov bh, [bx + 99]

; negative displacements
mov al, [bx + si - 128]
mov bx, [bp + di - 69]
mov ah, [bx + di - 0]
mov cx, [bp + si - 1]
