$(shell mkdir -p bin obj)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -Iinclude -std=c23

TARGET = bin/program
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

.PHONY: default clean run tests test_single_reg_mov

test: clean $(TARGET) test_single_reg_mov test_many_reg_mov test_imm_to_reg_mov

test_single_reg_mov:
	nasm -f bin asm/listing_0037_single_register_mov.asm -o bin/input
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(TARGET) bin/input asm/output.asm
	nasm -f bin asm/output.asm -o bin/output
	cmp bin/input bin/output
	echo "PASS: single register mov"

test_many_reg_mov:
	nasm -f bin asm/listing_0038_many_register_mov.asm -o bin/input
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(TARGET) bin/input asm/output.asm
	nasm -f bin asm/output.asm -o bin/output
	cmp bin/input bin/output
	echo "PASS: many register mov"

test_imm_to_reg_mov:
	nasm -f bin asm/imm_to_reg_movs.asm -o bin/input
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(TARGET) bin/input asm/output.asm
	nasm -f bin asm/output.asm -o bin/output
	cmp bin/input bin/output
	echo "PASS: imm to register mov"

clean:
	rm -f obj/*.o bin/*

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

