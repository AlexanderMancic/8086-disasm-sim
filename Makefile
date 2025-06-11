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
	test_all_loops test_listing_41 test_rm_to_sr_mov test_sr_to_rm_mov

SIM_TEST_LIST := \
	test_sim_imm_to_reg_w_movs test_sim_reg_to_reg_w_mov test_sim_reg_to_sr_mov \
	test_sim_sr_to_reg_mov test_sim_listing_45 test_sim_listing_46 test_sim_listing_47 \
	test_sim_imm_to_accumulator_add_sub_cmp test_sim_listing_0048_ip_register \
	test_sim_listing_0049_conditional_jumps test_sim_listing_0050_challenge_jumps \
	test_sim_all_conditional_jumps_and_loops test_sim_listing_0051_memory_mov

define run_test
	@nasm -f bin $1 -o $(INPUT_BIN)
	@# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $(TARGET) $(INPUT_BIN) $(OUTPUT_ASM)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM)
	@nasm -f bin $(OUTPUT_ASM) -o $(OUTPUT_BIN)
	@cmp $(INPUT_BIN) $(OUTPUT_BIN)
	@echo "PASS DISASM: $@"
endef

.PHONY: \
	default clean run tests $(DISASM_TEST_LIST) newline $(SIM_TEST_LIST)

test: clean $(DISASM_TEST_LIST) newline $(SIM_TEST_LIST)
	@echo
	@echo All tests passed

test_single_reg_mov: $(TARGET)
	@echo
	$(call run_test,./asm/disasm/listing_0037_single_register_mov.asm)

test_many_reg_mov: $(TARGET)
	$(call run_test,./asm/disasm/listing_0038_many_register_mov.asm)

test_imm_to_reg_mov: $(TARGET)
	$(call run_test,./asm/disasm/imm_to_reg_movs.asm)

test_mem_to_reg_mov_no_disp_no_direct: $(TARGET)
	$(call run_test,./asm/disasm/mem_to_reg_mov_no_disp_no_direct.asm)

test_mem_to_reg_mov_8bit_disp: $(TARGET)
	$(call run_test,./asm/disasm/mem_to_reg_mov_8bit_disp.asm)

test_mem_to_reg_mov_16bit_disp: $(TARGET)
	$(call run_test,./asm/disasm/mem_to_reg_mov_16bit_disp.asm)

test_address_as_destination: $(TARGET)
	$(call run_test,./asm/disasm/address_as_destination.asm)

test_listing_39: $(TARGET)
	$(call run_test,./asm/disasm/listing_0039_more_movs.asm)

test_direct_address_mov: $(TARGET)
	$(call run_test,./asm/disasm/direct_address_mov.asm)

test_imm_to_rm_mov: $(TARGET)
	$(call run_test,./asm/disasm/imm_to_rm_mov.asm)

test_mem_to_accumulator_mov: $(TARGET)
	$(call run_test,./asm/disasm/mem_to_accumulator_mov.asm)

test_accumulator_to_mem_mov: $(TARGET)
	$(call run_test,./asm/disasm/accumulator_to_mem_mov.asm)

test_listing_40: $(TARGET)
	$(call run_test,./asm/disasm/listing_0040_challenge_movs.asm)

test_add_sub_cmp_rm_reg: $(TARGET)
	$(call run_test,./asm/disasm/add_sub_cmp_rm_reg.asm)

test_add_sub_cmp_imm_rm: $(TARGET)
	$(call run_test,./asm/disasm/add_sub_cmp_imm_rm.asm)

test_non_wide_accumulator_to_mem_mov_and_vice_verca: $(TARGET)
	$(call run_test,./asm/disasm/non_wide_accumulator_to_mem_mov_and_vice_verca.asm)

test_add_sub_cmp_imm_accumulator: $(TARGET)
	$(call run_test,./asm/disasm/add_sub_cmp_imm_accumulator.asm)

test_jnz: $(TARGET)
	$(call run_test,./asm/disasm/jnz.asm)

test_all_conditional_jumps: $(TARGET)
	$(call run_test,./asm/disasm/all_conditional_jumps.asm)

test_all_loops: $(TARGET)
	$(call run_test,./asm/disasm/all_loops.asm)

test_listing_41: $(TARGET)
	$(call run_test,./asm/disasm/listing_0041_add_sub_cmp_jnz.asm)

test_rm_to_sr_mov: $(TARGET)
	$(call run_test,./asm/disasm/rm_to_sr_mov.asm)

test_sr_to_rm_mov: $(TARGET)
	$(call run_test,./asm/disasm/sr_to_rm_mov.asm)

newline:
	@echo

test_sim_imm_to_reg_w_movs: $(TARGET)
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

test_sim_reg_to_reg_w_mov: $(TARGET)
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

test_sim_reg_to_sr_mov: $(TARGET)
	$(call run_test,./asm/sim/reg_to_sr_mov.asm)
	@grep -q ";.*ax: 8738" $(OUTPUT_ASM)
	@grep -q ";.*bx: 17476" $(OUTPUT_ASM)
	@grep -q ";.*cx: 26214" $(OUTPUT_ASM)
	@grep -q ";.*dx: 34952" $(OUTPUT_ASM)
	@grep -q ";.*sp: 0" $(OUTPUT_ASM)
	@grep -q ";.*bp: 0" $(OUTPUT_ASM)
	@grep -q ";.*si: 0" $(OUTPUT_ASM)
	@grep -q ";.*di: 0" $(OUTPUT_ASM)
	@grep -q ";.*ss: 8738" $(OUTPUT_ASM)
	@grep -q ";.*ds: 17476" $(OUTPUT_ASM)
	@grep -q ";.*es: 26214" $(OUTPUT_ASM)
	@grep -q ";.*cs: 34952" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_sr_to_reg_mov: $(TARGET)
	$(call run_test,./asm/sim/sr_to_reg_mov.asm)
	@grep -q ";.*ax: 1111" $(OUTPUT_ASM)
	@grep -q ";.*bx: 2222" $(OUTPUT_ASM)
	@grep -q ";.*cx: 3333" $(OUTPUT_ASM)
	@grep -q ";.*dx: 4444" $(OUTPUT_ASM)
	@grep -q ";.*sp: 1111" $(OUTPUT_ASM)
	@grep -q ";.*bp: 2222" $(OUTPUT_ASM)
	@grep -q ";.*si: 3333" $(OUTPUT_ASM)
	@grep -q ";.*di: 4444" $(OUTPUT_ASM)
	@grep -q ";.*es: 1111" $(OUTPUT_ASM)
	@grep -q ";.*cs: 2222" $(OUTPUT_ASM)
	@grep -q ";.*ss: 3333" $(OUTPUT_ASM)
	@grep -q ";.*ds: 4444" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_listing_45: $(TARGET)
	$(call run_test,./asm/sim/listing_0045_challenge_register_movs.asm)
	@grep -q ";.*ax: 17425" $(OUTPUT_ASM)
	@grep -q ";.*bx: 13124" $(OUTPUT_ASM)
	@grep -q ";.*cx: 26231" $(OUTPUT_ASM)
	@grep -q ";.*dx: 30600" $(OUTPUT_ASM)
	@grep -q ";.*sp: 17425" $(OUTPUT_ASM)
	@grep -q ";.*bp: 13124" $(OUTPUT_ASM)
	@grep -q ";.*si: 26231" $(OUTPUT_ASM)
	@grep -q ";.*di: 30600" $(OUTPUT_ASM)
	@grep -q ";.*es: 26231" $(OUTPUT_ASM)
	@grep -q ";.*cs: 0" $(OUTPUT_ASM)
	@grep -q ";.*ss: 17425" $(OUTPUT_ASM)
	@grep -q ";.*ds: 13124" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_listing_46: $(TARGET)
	$(call run_test,./asm/sim/listing_0046_add_sub_cmp.asm)
	@grep -q ";.*ax: 0" $(OUTPUT_ASM)
	@grep -q ";.*bx: 57602" $(OUTPUT_ASM)
	@grep -q ";.*cx: 3841" $(OUTPUT_ASM)
	@grep -q ";.*dx: 0" $(OUTPUT_ASM)
	@grep -q ";.*sp: 998" $(OUTPUT_ASM)
	@grep -q ";.*bp: 0" $(OUTPUT_ASM)
	@grep -q ";.*si: 0" $(OUTPUT_ASM)
	@grep -q ";.*di: 0" $(OUTPUT_ASM)
	@grep -q ";.*es: 0" $(OUTPUT_ASM)
	@grep -q ";.*cs: 0" $(OUTPUT_ASM)
	@grep -q ";.*ss: 0" $(OUTPUT_ASM)
	@grep -q ";.*ds: 0" $(OUTPUT_ASM)
	@grep -q "; Flags: PZ" $(OUTPUT_ASM)
	@grep -q "^sub .*bx, .*cx ; .* | Flags:.* -> S" $(OUTPUT_ASM)
	@grep -q "^cmp .*bp, .*sp ; .* | Flags:.* -> " $(OUTPUT_ASM)
	@grep -q "^sub .*bp, .*2026 ; .* | Flags:.* -> PZ" $(OUTPUT_ASM)

test_sim_listing_47: $(TARGET)
	$(call run_test,./asm/sim/listing_0047_challenge_flags.asm)
	@grep -q ";.*ax: 0" $(OUTPUT_ASM)
	@grep -q ";.*bx: 40101" $(OUTPUT_ASM)
	@grep -q ";.*cx: 0" $(OUTPUT_ASM)
	@grep -q ";.*dx: 10" $(OUTPUT_ASM)
	@grep -q ";.*sp: 99" $(OUTPUT_ASM)
	@grep -q ";.*bp: 98" $(OUTPUT_ASM)
	@grep -q ";.*si: 0" $(OUTPUT_ASM)
	@grep -q ";.*di: 0" $(OUTPUT_ASM)
	@grep -q ";.*es: 0" $(OUTPUT_ASM)
	@grep -q ";.*cs: 0" $(OUTPUT_ASM)
	@grep -q ";.*ss: 0" $(OUTPUT_ASM)
	@grep -q ";.*ds: 0" $(OUTPUT_ASM)
	@grep -q "; Flags: CPAS" $(OUTPUT_ASM)
	@grep -q "^add .*bx, .*30000 ; .* | Flags:.* -> P" $(OUTPUT_ASM)
	@grep -q "^add .*bx, .*10000 ; .* | Flags:.* -> SO" $(OUTPUT_ASM)
	@grep -q "^sub .*bx, .*5000 ; .* | Flags:.* -> PAS" $(OUTPUT_ASM)
	@grep -q "^sub .*bx, .*5000 ; .* | Flags:.* -> PO" $(OUTPUT_ASM)
	@grep -q "^add .*bx, .*cx ; .* | Flags:.* -> P" $(OUTPUT_ASM)
	@grep -q "^sub .*cx, .*dx ; .* | Flags:.* -> PA" $(OUTPUT_ASM)
	@grep -q "^add .*bx, .*40000 ; .* | Flags:.* -> PS" $(OUTPUT_ASM)
	@grep -q "^add .*cx, .*65446 ; .* | Flags:.* -> CPAZ" $(OUTPUT_ASM)
	@grep -q "^cmp .*bp, .*sp ; .* | Flags:.* -> CPAS" $(OUTPUT_ASM)

test_sim_imm_to_accumulator_add_sub_cmp: $(TARGET)
	$(call run_test,./asm/sim/imm_to_accumulator_add_sub_cmp.asm)
	@grep -q ";.*ax: 33222" $(OUTPUT_ASM)
	@grep -q ";.*bx: 0" $(OUTPUT_ASM)
	@grep -q ";.*cx: 0" $(OUTPUT_ASM)
	@grep -q ";.*dx: 0" $(OUTPUT_ASM)
	@grep -q ";.*sp: 0" $(OUTPUT_ASM)
	@grep -q ";.*bp: 0" $(OUTPUT_ASM)
	@grep -q ";.*si: 0" $(OUTPUT_ASM)
	@grep -q ";.*di: 0" $(OUTPUT_ASM)
	@grep -q ";.*es: 0" $(OUTPUT_ASM)
	@grep -q ";.*cs: 0" $(OUTPUT_ASM)
	@grep -q ";.*ss: 0" $(OUTPUT_ASM)
	@grep -q ";.*ds: 0" $(OUTPUT_ASM)
	@grep -q "; Flags: PAZ" $(OUTPUT_ASM)
	@grep -q "^add .*al, .*222 ; .* | Flags:.* -> PS" $(OUTPUT_ASM)
	@grep -q "^add .*ax, .*33000 ; .* | Flags:.* -> PAS" $(OUTPUT_ASM)
	@grep -q "^cmp .*ax, .*33222 ; .* | Flags:.* -> PAZ" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_listing_0048_ip_register: $(TARGET)
	$(call run_test,./asm/sim/listing_0048_ip_register.asm)
	@grep -q ";.*ax: 0" $(OUTPUT_ASM)
	@grep -q ";.*bx: 2000" $(OUTPUT_ASM)
	@grep -q ";.*cx: 64736" $(OUTPUT_ASM)
	@grep -q ";.*dx: 0" $(OUTPUT_ASM)
	@grep -q ";.*sp: 0" $(OUTPUT_ASM)
	@grep -q ";.*bp: 0" $(OUTPUT_ASM)
	@grep -q ";.*si: 0" $(OUTPUT_ASM)
	@grep -q ";.*di: 0" $(OUTPUT_ASM)
	@grep -q ";.*es: 0" $(OUTPUT_ASM)
	@grep -q ";.*cs: 0" $(OUTPUT_ASM)
	@grep -q ";.*ss: 0" $(OUTPUT_ASM)
	@grep -q ";.*ds: 0" $(OUTPUT_ASM)
	@grep -q "; IP: 14" $(OUTPUT_ASM)
	@grep -q "; Flags: CS" $(OUTPUT_ASM)
	@grep -q "^IP_0:" $(OUTPUT_ASM)
	@grep -q "^IP_3:" $(OUTPUT_ASM)
	@grep -q "^IP_5:" $(OUTPUT_ASM)
	@grep -q "^IP_9:" $(OUTPUT_ASM)
	@grep -q "^IP_12:" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_listing_0049_conditional_jumps: $(TARGET)
	@$(call run_test,./asm/sim/listing_0049_conditional_jumps.asm)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM) simulate
	@grep -q ";.*bx: 1030" $(OUTPUT_ASM)
	@grep -q "; IP: 14" $(OUTPUT_ASM)
	@grep -q "; Flags: PZ" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_listing_0050_challenge_jumps: $(TARGET)
	@$(call run_test,./asm/sim/listing_0050_challenge_jumps.asm)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM) simulate
	@grep -q ";.*ax: 13" $(OUTPUT_ASM)
	@grep -q ";.*bx: 65531" $(OUTPUT_ASM)
	@grep -q "; IP: 28" $(OUTPUT_ASM)
	@grep -q "; Flags: CAS" $(OUTPUT_ASM)
	@echo "PASS SIM: $@"

test_sim_all_conditional_jumps_and_loops: $(TARGET)
	@$(call run_test,./asm/sim/all_conditional_jumps_and_loops.asm)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM) simulate
	@grep -q ";.*ax: 10" $(OUTPUT_ASM)
	@grep -q ";.*cx: 0" $(OUTPUT_ASM)
	@grep -q ";.*bx: 128" $(OUTPUT_ASM)
	@grep -q ";.*dx: 255" $(OUTPUT_ASM)
	@grep -q ";.*si: 0" $(OUTPUT_ASM)
	@grep -q ";.*bp: 100" $(OUTPUT_ASM)

test_sim_listing_0051_memory_mov: $(TARGET)
	@$(call run_test,./asm/sim/listing_0051_memory_mov.asm)
	@$(TARGET) $(INPUT_BIN) $(OUTPUT_ASM) simulate
	@grep -q ";.*bx: 1" $(OUTPUT_ASM)
	@grep -q ";.*cx: 2" $(OUTPUT_ASM)
	@grep -q ";.*dx: 10" $(OUTPUT_ASM)
	@grep -q ";.*bp: 4" $(OUTPUT_ASM)
	@grep -q "; IP: 48" $(OUTPUT_ASM)

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

