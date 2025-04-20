CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -Iinclude -std=c23 -g
CFLAGS += -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wconversion

TARGET := ./bin/program
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c, obj/%.o, $(SRC))

INPUT_BIN := ./bin/input
OUTPUT_ASM := ./asm/output.asm
OUTPUT_BIN := ./bin/output

DISASM_TEST_LIST := \
	test_single_reg_mov test_many_reg_mov test_imm_to_reg_mov \
	test_mem_to_reg_mov_no_disp_no_direct test_mem_to_reg_mov_8bit_disp \
	test_mem_to_reg_mov_16bit_disp test_address_as_destination \
	test_listing_39 test_direct_address_mov test_imm_to_rm_mov \
	test_mem_to_accumulator_mov test_accumulator_to_mem_mov \
	test_listing_40 test_add_sub_cmp_rm_reg test_add_sub_cmp_imm_rm \
	test_non_wide_accumulator_to_mem_mov_and_vice_verca \
	test_add_sub_cmp_imm_accumulator test_jnz test_all_conditional_jumps \
	test_all_loops test_listing_41

SIM_TEST_LIST := \
	test_sim_imm_to_reg_w_movs test_sim_reg_to_reg_w_mov

define run_test
	@nasm -f bin $1 -o $(INPUT_BIN)
	@# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $(TARGET) $(INPUT_BIN) $(OUTPUT_ASM)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM)
	@nasm -f bin $(OUTPUT_ASM) -o $(OUTPUT_BIN)
	@cmp $(INPUT_BIN) $(OUTPUT_BIN)
	@echo "PASS DISASM: $@"
endef

.PHONY: \
	default clean run tests $(DISASM_TEST_LIST)

test: clean $(TARGET) $(DISASM_TEST_LIST) $(SIM_TEST_LIST)
	@echo All tests passed

test_single_reg_mov:
	$(call run_test,./asm/disasm/listing_0037_single_register_mov.asm)

test_many_reg_mov:
	$(call run_test,./asm/disasm/listing_0038_many_register_mov.asm)

test_imm_to_reg_mov:
	$(call run_test,./asm/disasm/imm_to_reg_movs.asm)

test_mem_to_reg_mov_no_disp_no_direct:
	$(call run_test,./asm/disasm/mem_to_reg_mov_no_disp_no_direct.asm)

test_mem_to_reg_mov_8bit_disp:
	$(call run_test,./asm/disasm/mem_to_reg_mov_8bit_disp.asm)

test_mem_to_reg_mov_16bit_disp:
	$(call run_test,./asm/disasm/mem_to_reg_mov_16bit_disp.asm)

test_address_as_destination:
	$(call run_test,./asm/disasm/address_as_destination.asm)

test_listing_39:
	$(call run_test,./asm/disasm/listing_0039_more_movs.asm)

test_direct_address_mov:
	$(call run_test,./asm/disasm/direct_address_mov.asm)

test_imm_to_rm_mov:
	$(call run_test,./asm/disasm/imm_to_rm_mov.asm)

test_mem_to_accumulator_mov:
	$(call run_test,./asm/disasm/mem_to_accumulator_mov.asm)

test_accumulator_to_mem_mov:
	$(call run_test,./asm/disasm/accumulator_to_mem_mov.asm)

test_listing_40:
	$(call run_test,./asm/disasm/listing_0040_challenge_movs.asm)

test_add_sub_cmp_rm_reg:
	$(call run_test,./asm/disasm/add_sub_cmp_rm_reg.asm)

test_add_sub_cmp_imm_rm:
	$(call run_test,./asm/disasm/add_sub_cmp_imm_rm.asm)

test_non_wide_accumulator_to_mem_mov_and_vice_verca:
	$(call run_test,./asm/disasm/non_wide_accumulator_to_mem_mov_and_vice_verca.asm)

test_add_sub_cmp_imm_accumulator:
	$(call run_test,./asm/disasm/add_sub_cmp_imm_accumulator.asm)

test_jnz:
	$(call run_test,./asm/disasm/jnz.asm)

test_all_conditional_jumps:
	$(call run_test,./asm/disasm/all_conditional_jumps.asm)

test_all_loops:
	$(call run_test,./asm/disasm/all_loops.asm)

test_listing_41:
	$(call run_test,./asm/disasm/listing_0041_add_sub_cmp_jnz.asm)

test_sim_imm_to_reg_w_movs:
	$(call run_test,./asm/sim/listing_0043_immediate_movs.asm)
	@grep -q ";.*ax: 1" $(OUTPUT_ASM)
	@grep -q ";.*bx: 2" $(OUTPUT_ASM)
	@grep -q ";.*cx: 3" $(OUTPUT_ASM)
	@grep -q ";.*dx: 4" $(OUTPUT_ASM)
	@grep -q ";.*sp: 5" $(OUTPUT_ASM)
	@grep -q ";.*bp: 6" $(OUTPUT_ASM)
	@grep -q ";.*si: 7" $(OUTPUT_ASM)
	@grep -q ";.*di: 8" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_reg_to_reg_w_mov:
	$(call run_test,./asm/sim/listing_0044_register_movs.asm)
	@grep -q ";.*ax: 4" $(OUTPUT_ASM)
	@grep -q ";.*bx: 3" $(OUTPUT_ASM)
	@grep -q ";.*cx: 2" $(OUTPUT_ASM)
	@grep -q ";.*dx: 1" $(OUTPUT_ASM)
	@grep -q ";.*sp: 1" $(OUTPUT_ASM)
	@grep -q ";.*bp: 2" $(OUTPUT_ASM)
	@grep -q ";.*si: 3" $(OUTPUT_ASM)
	@grep -q ";.*di: 4" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

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

