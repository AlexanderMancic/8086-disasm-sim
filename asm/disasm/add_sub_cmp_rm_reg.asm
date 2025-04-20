bits 16

; taken from listing 41

add bx, [bx+si]
add bx, [bp]
add bx, [bp + 0]
add cx, [bx + 2]
add bh, [bp + si + 4]
add di, [bp + di + 6]
add [bx+si], bx
add [bp], bx
add [bp + 0], bx
add [bx + 2], cx
add [bp + si + 4], bh
add [bp + di + 6], di
add ax, [bp]
add al, [bx + si]
add ax, bx
add al, ah

sub bx, [bx+si]
sub bx, [bp]
sub bx, [bp + 0]
sub cx, [bx + 2]
sub bh, [bp + si + 4]
sub di, [bp + di + 6]
sub [bx+si], bx
sub [bp], bx
sub [bp + 0], bx
sub [bx + 2], cx
sub [bp + si + 4], bh
sub [bp + di + 6], di
sub ax, [bp]
sub al, [bx + si]
sub ax, bx
sub al, ah

cmp bx, [bx+si]
cmp bx, [bp]
cmp bx, [bp + 0]
cmp cx, [bx + 2]
cmp bh, [bp + si + 4]
cmp di, [bp + di + 6]
cmp [bx+si], bx
cmp [bp], bx
cmp [bp + 0], bx
cmp [bx + 2], cx
cmp [bp + si + 4], bh
cmp [bp + di + 6], di
cmp ax, [bp]
cmp al, [bx + si]
cmp ax, bx
cmp al, ah
