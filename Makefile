$(shell mkdir -p bin obj)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -Iinclude

TARGET = bin/program
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

.PHONY: default clean run

default: $(TARGET) run

clean:
	rm -f obj/*.o bin/*

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run:
	./$(TARGET)
