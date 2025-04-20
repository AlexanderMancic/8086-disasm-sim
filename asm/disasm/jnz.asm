bits 16

; taken from listing 41

test_label0:
jnz test_label1
jnz test_label0
test_label1:
jnz test_label0
jnz test_label1
