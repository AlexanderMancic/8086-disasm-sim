bits 16

	mov ax, 5
	cmp ax, 5
	je jl_label
	sub ax, 1

jl_label:
	cmp ax, 6
	jl jle_less_label
	sub ax, 1

jle_less_label:
	cmp ax, 6
	jle jle_eq_label
	sub ax, 1

jle_eq_label:
	cmp ax, 5
	jle jb_label
	sub ax, 1

jb_label:
	mov bl, 255
	add bl, 1
	jb jbe_below_label
	sub ax, 1

jbe_below_label:
	jbe jbe_eq_label
	sub ax, 1

jbe_eq_label:
	cmp ax, 5
	jbe jp_label
	sub ax, 1

jp_label:
	add ax, 2
	sub ax, 2
	jp jo_label
	sub ax, 1

jo_label:
	mov bl, 127
	add bl, 1
	jo js_label
	sub ax, 1

js_label:
	mov dl, 0
	sub dl, 1
	js jne_label
	sub ax, 1

jne_label:
	cmp ax, 6
	jne jge_gt_label
	sub ax, 1

jge_gt_label:
	cmp ax, 3
	jge jge_eq_label
	sub ax, 1

jge_eq_label:
	cmp ax, 5
	jge jg_label
	sub ax, 1

jg_label:
	cmp ax, 3
	jge jae_above_label
	sub ax, 1

jae_above_label:
	cmp ax, 3
	jae jae_eq_label
	sub ax, 1

jae_eq_label:
	cmp ax, 5
	jae ja_label
	sub ax, 1

ja_label:
	cmp ax, 3
	ja jnp_label
	sub ax, 1

jnp_label:
	add ax, 2
	jnp jno_label
	sub ax, 1

jno_label:
	add ax, 1
	sub ax, 1
	jno jns_label
	sub ax, 1

jns_label:
	add ax, 1
	jns loop_label
	sub ax, 1

loop_label:
	mov cx, 3

loop_start:
	add ax, 1
	loop loop_start

	mov cx, 4
	mov si, 6

loopz_zf_start:
	sub si, 6
	loopz loopz_zf_start

loopz_cx_start:
	add si, si
	loopz loopz_cx_start

	mov cx, 10
	mov si, 8
	mov bp, 106

loopnz_zf_start:
	sub si, 2
	loopnz loopnz_zf_start

loopnz_cx_start:
	sub bp, 1
	loopnz loopnz_cx_start

	jcxz end_label
	sub ax, 1

end_label:
	add cx, cx

; ax == 10
; cx == 0
; bx == 128
; dx == 255
; si == 0
; bp == 100
