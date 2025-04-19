bits 16

; taken from listing 39
; Dest address calculation
mov [bx + di], cx
mov [bp + si], cl
mov [bp], ch
