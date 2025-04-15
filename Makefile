$(shell mkdir -p bin obj)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -Iinclude -std=c23

TARGET = bin/program
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

.PHONY: default clean run test

test: clean $(TARGET)
	nasm -f bin asm/listing_0037_single_register_mov.asm -o bin/input
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(TARGET) bin/input asm/output.asm
	nasm -f bin asm/output.asm -o bin/output
	cmp bin/input bin/output
	echo "PASS: single register mov"

clean:
	rm -f obj/*.o bin/*

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

