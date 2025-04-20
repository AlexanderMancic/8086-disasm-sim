bits 16

; taken from listing 41

add si, 2
add bp, 2
add cx, 8
add byte [bx], 34
add word [bp + si + 1000], 29

sub si, 2
sub bp, 2
sub cx, 8
sub byte [bx], 34
sub word [bx + di], 29

cmp si, 2
cmp bp, 2
cmp cx, 8
cmp byte [bx], 34
cmp word [4834], 29

add ah, byte 4
