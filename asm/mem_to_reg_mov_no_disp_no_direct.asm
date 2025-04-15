; taken from listing 39
; Source address calculation
mov al, [bx + si]
mov bx, [bp + di]

; more instructions
mov ah, [bx + di]
mov cx, [bp + si]
mov dl, [si]
mov bh, [bx]
