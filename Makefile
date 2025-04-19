CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -Iinclude -std=c23 -g
CFLAGS += -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wconversion

TARGET := ./bin/program
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c, obj/%.o, $(SRC))

INPUT_BIN := ./bin/input
OUTPUT_ASM := ./asm/output.asm
OUTPUT_BIN := ./bin/output

TEST_LIST := \
	test_single_reg_mov test_many_reg_mov test_imm_to_reg_mov \
	test_mem_to_reg_mov_no_disp_no_direct test_mem_to_reg_mov_8bit_disp \
	test_mem_to_reg_mov_16bit_disp test_address_as_destination \
	test_listing_39 test_direct_address_mov test_imm_to_rm_mov \
	test_mem_to_accumulator_mov test_accumulator_to_mem_mov \
	test_listing_40 test_add_sub_cmp_rm_reg test_add_sub_cmp_imm_rm \
	test_non_wide_accumulator_to_mem_mov_and_vice_verca \
	test_add_sub_cmp_imm_accumulator test_jnz test_all_conditional_jumps \
	test_all_loops

define run_test
	@nasm -f bin $1 -o $(INPUT_BIN)
	@# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $(TARGET) $(INPUT_BIN) $(OUTPUT_ASM)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM)
	@nasm -f bin $(OUTPUT_ASM) -o $(OUTPUT_BIN)
	@cmp $(INPUT_BIN) $(OUTPUT_BIN)
	@echo "PASS: $@"
endef

.PHONY: \
	default clean run tests $(TEST_LIST)

test: clean $(TARGET) $(TEST_LIST)
	@echo All tests passed

test_single_reg_mov:
	$(call run_test,./asm/listing_0037_single_register_mov.asm)

test_many_reg_mov:
	$(call run_test,./asm/listing_0038_many_register_mov.asm)

test_imm_to_reg_mov:
	$(call run_test,./asm/imm_to_reg_movs.asm)

test_mem_to_reg_mov_no_disp_no_direct:
	$(call run_test,./asm/mem_to_reg_mov_no_disp_no_direct.asm)

test_mem_to_reg_mov_8bit_disp:
	$(call run_test,./asm/mem_to_reg_mov_8bit_disp.asm)

test_mem_to_reg_mov_16bit_disp:
	$(call run_test,./asm/mem_to_reg_mov_16bit_disp.asm)

test_address_as_destination:
	$(call run_test,./asm/address_as_destination.asm)

test_listing_39:
	$(call run_test,./asm/listing_0039_more_movs.asm)

test_direct_address_mov:
	$(call run_test,./asm/direct_address_mov.asm)

test_imm_to_rm_mov:
	$(call run_test,./asm/imm_to_rm_mov.asm)

test_mem_to_accumulator_mov:
	$(call run_test,./asm/mem_to_accumulator_mov.asm)

test_accumulator_to_mem_mov:
	$(call run_test,./asm/accumulator_to_mem_mov.asm)

test_listing_40:
	$(call run_test,./asm/listing_0040_challenge_movs.asm)

test_add_sub_cmp_rm_reg:
	$(call run_test,./asm/add_sub_cmp_rm_reg.asm)

test_add_sub_cmp_imm_rm:
	$(call run_test,./asm/add_sub_cmp_imm_rm.asm)

test_non_wide_accumulator_to_mem_mov_and_vice_verca:
	$(call run_test,./asm/non_wide_accumulator_to_mem_mov_and_vice_verca.asm)

test_add_sub_cmp_imm_accumulator:
	$(call run_test,./asm/add_sub_cmp_imm_accumulator.asm)

test_jnz:
	$(call run_test,./asm/jnz.asm)

test_all_conditional_jumps:
	$(call run_test,./asm/all_conditional_jumps.asm)

test_all_loops:
	$(call run_test,./asm/all_loops.asm)

clean:
	@echo "Cleaning project..."
	@rm -rf ./obj ./bin
	@rm -f ./asm/output.asm

bin obj:
	@mkdir -p $@

$(TARGET): $(OBJ) | bin
	@$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c | obj
	@$(CC) $(CFLAGS) -c $< -o $@

