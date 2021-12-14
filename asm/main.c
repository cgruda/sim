#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define dbg printf
#define DBG_MODE 1

#define IMEM_LEN	1024
#define IMEME_ADDR_MSK	0x000003FF
#define     MAX_LINE_LENGTH     501


#define     MSL                 51     //MSL - Max Segment Length

#define INST_OP_OFT	24
#define INST_OP_MSK	0xFF000000
#define INST_RD_OFT	20
#define INST_RD_MSK	0x00F00000
#define INST_RS_OFT	16
#define INST_RS_MSK	0x000F0000
#define INST_RT_OFT	12
#define INST_RT_MSK	0x0000F000
#define INST_IM_OFT	0
#define INST_IM_MSK	0x00000FFF

enum op {
	OP_ADD,
	OP_SUB,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_MUL,
	OP_SLL,
	OP_SRA,
	OP_SRL,
	OP_BEQ,
	OP_BNE,
	OP_BLT,
	OP_BGT,
	OP_BLE,
	OP_BGE,
	OP_JAL,
	OP_LW,
	OP_SW,
	OP_RSV1,
	OP_RSV2,
	OP_HALT,
	OP_MAX,
};

const char *op_name[OP_MAX] = {
	[OP_ADD] = "add",
	[OP_SUB] = "sub",
	[OP_AND] = "and",
	[OP_OR] = "or",
	[OP_XOR] = "xor",
	[OP_MUL] = "mul",
	[OP_SLL] = "sll",
	[OP_SRA] = "sra",
	[OP_SRL] = "srl",
	[OP_BEQ] = "beq",
	[OP_BNE] = "bne",
	[OP_BLT] = "blt",
	[OP_BGT] = "bgt",
	[OP_BLE] = "ble",
	[OP_BGE] = "bge",
	[OP_JAL] = "jal",
	[OP_LW] = "lw",
	[OP_SW] = "sw",
	[OP_RSV1] = "rsv1",
	[OP_RSV2] = "rsv2",
	[OP_HALT] = "halt",
};

enum reg {
	REG_0,
	REG_1,
	REG_2,
	REG_3,
	REG_4,
	REG_5,
	REG_6,
	REG_7,
	REG_8,
	REG_9,
	REG_10,
	REG_11,
	REG_12,
	REG_13,
	REG_14,
	REG_15,
	REG_MAX
};

const char *reg_name[REG_MAX][5] = {
	[REG_0] = {"$0", "$r0", "$zero", 0},
	[REG_1] = {"$1", "$r1", "$imm", 0},
	[REG_2] = {"$2", "$r2", "$fp", 0},
	[REG_3] = {"$3", "$r3", "$v0", 0},
	[REG_4] = {"$4", "$r4", "$a0", 0},
	[REG_5] = {"$5", "$r5", "$a1", 0},
	[REG_6] = {"$6", "$r6", "$t0", 0},
	[REG_7] = {"$7", "$r7", "$t1", 0},
	[REG_8] = {"$8", "$r8", "$t2", 0},
	[REG_9] = {"$9", "$r9", "$t3", 0},
	[REG_10] = {"$10", "$r10", "$s0", 0},
	[REG_11] = {"$11", "$r11", "$s1", 0},
	[REG_12] = {"$12", "$r12", "$s2", 0},
	[REG_13] = {"$13", "$r13", "$gp", 0},
	[REG_14] = {"$14", "$r14", "$sp", 0},
	[REG_15] = {"$15", "$r15", "$ra", 0},
};

void remove_comment(char *line)
{
	char *comment_start_pointer = strchr(line, '#');
	if (comment_start_pointer)
		line[comment_start_pointer - line] = '\0';
}

uint16_t str_to_dec(char *str)
{
	uint16_t x = 0;

	if ((*(str + 1) == 'x') || (*(str + 1) == 'X')) {
		x = (uint16_t)strtol(str + 2, NULL, 16);                  // decode hex number
	} else {
		if (*str == '-')
			x = (~((uint16_t)strtol((str + 1), NULL, 10))) + 1;   // decode negative dec number (2's complement)
		else
			x = (uint16_t)strtol(str, NULL, 10);                     // decode positive dec number
	}

	return x;
}

void write_word_to_imem(uint32_t *imem, char *ads, char *val)
{
	uint32_t addr = 0, word = 0;

	addr = str_to_dec(ads) & IMEME_ADDR_MSK;
	word = str_to_dec(val);

	imem[addr] = word;
}

void strtolow(char *str)
{
	for (char *p = str; *p; p++)
		*p = tolower(*p);
}

uint16_t op_decode(char *op)
{
	strtolow(op);

	for (int i = 0; i < OP_MAX; ++i)
		if (!strcmp(op, op_name[i]))
			return i;

	printf("error! op \"%s\" is unrecognized. placing \"halt\" instead!\n", op);
	return OP_HALT;
}

uint16_t reg_decode(char *reg)
{
	strtolow(reg);

	for (int i = 0; i < REG_MAX; ++i)
		for (int j = 0; reg_name[i][j]; j++)
			if (!strcmp(reg, reg_name[i][j]))
				return i;

	printf("error! rd/rs/rt \"%s\" unrecognized. placing \"$zero\" instead!\n", reg);
	return 0;
}


void write_instruction_to_imem(uint32_t *imem, uint16_t *pc, char *p1, char *p2, char *p3, char *p4, char *p5, char *i_l_n[], uint16_t *i_a_b, uint16_t *i_h)
{
	uint32_t inst = 0;
	uint16_t op = 0, rd = 0, rs = 0, rt = 0, im = 0;

	if (!p2) {
		op = op_decode(p1);
		inst |= op << INST_OP_OFT;
		imem[*pc] = inst;
		++*pc;
		return;
	}

	op = op_decode(p1);
	rd = reg_decode(p2);
	rs = reg_decode(p3);
	rt = reg_decode(p4);

	if (isalpha(*p5)) {
		i_l_n[*i_h] = malloc(MSL * sizeof(char));
		if (!i_l_n[*i_h]) {
			printf("memmory allocation error!\n");
			exit(1);
		}
		strcpy(i_l_n[*i_h], p5); // save label name in i_l_n[i_c]
		i_a_b[*i_h] = *pc;       // save imm_address
		++*i_h;                  // increment imm counter
		im = 0;                  // temporary imm to be placed in imem
	} else {
		im = str_to_dec(p5);
	}

	inst |= (op << INST_OP_OFT) & INST_OP_MSK;
	inst |= (rd << INST_RD_OFT) & INST_RD_MSK;
	inst |= (rs << INST_RS_OFT) & INST_RS_MSK;
	inst |= (rt << INST_RT_OFT) & INST_RT_MSK;
	inst |= (im << INST_IM_OFT) & INST_IM_MSK;

	imem[*pc] = inst;
	++*pc;
}

/*
 * function:        remember_label_location_in_imem
 * description:     remembers the pc location of a label in the imem
 * input:           pointer to imem, pointer to pc, label name, string array, label address book, label counter
 * output:          none
 */
void remember_label_location_in_imem(uint32_t *imem, uint16_t *pc, char *p1, char *l_n[], uint16_t *l_a_b, uint16_t *l_c)
{
	l_n[*l_c] = malloc(MSL * sizeof(char));
	if (!l_n[*l_c]) {
		printf("memmory allocation error!\n");
		exit(1);
	}
	strcpy(l_n[*l_c], p1);                      // save label name
	if (l_n[*l_c][strlen(l_n[*l_c]) - 1] == ':')
		l_n[*l_c][strlen(l_n[*l_c]) - 1] = '\0';    // remove ':' from end of label name
	l_a_b[*l_c] = *pc;                          // save address of label
	++*l_c;
}

/*
 * function:        parse_decode_encode_write
 * description:     parses assembly line by line and places encoded instructions in imem
 * input:           pointer to assembly file, label vars,  imem pointer
 * output:          none
 */
void parse_decode_encode_write(FILE *fptr, uint32_t *imem, char *l_n[], char *i_l_n[], uint16_t *l_a_b, uint16_t *i_l_b, uint16_t *l_c, uint16_t *i_h)
{
	uint16_t pc = 0;

	while (!feof(fptr)) {
		char asm_line[MAX_LINE_LENGTH] = {0};
		char s1[MSL] = {0}, s2[MSL] = {0}, s3[MSL] = {0}, s4[MSL] = {0}, s5[MSL] = {0}, s6[MSL] = {0};

		if (!fgets(asm_line, MAX_LINE_LENGTH, fptr))
			continue;

		remove_comment(asm_line);

		int num_of_segments = sscanf(asm_line, "%s%*[:\t\n ]"
			"%[^,\t ]%*[,\t ]"
			"%[^,\t\n ]%*[,\t\n ]"
			"%[^,\t\n ]%*[,\t\n ]"
			"%[^,\t\n ]%*[,\t\n ]"
			"%s",
			s1, s2, s3, s4, s5, s6); // complex formatting but saves a ton of code

		if (DBG_MODE)
			dbg("%d:   %s %s %s %s %s %s\n", num_of_segments, s1, s2, s3, s4, s5, s6);

		// decode assembly syntax line and call encode as SIMP instruction in imem
		switch (num_of_segments) {
		case -1:
			break;      // deal with different file endings
		case 0:
			break;      // deal with blank lines
		case 1:
			if (!strcmp(s1, op_name[OP_HALT]))
				write_instruction_to_imem(imem, &pc, s1, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			else
				remember_label_location_in_imem(imem, &pc, s1, l_n, l_a_b, l_c);
			break;
		case 3:
			write_word_to_imem(imem, s2, s3);
			break;
		case 4:
		case 5:
			write_instruction_to_imem(imem, &pc, s1, s2, s3, s4, s5, i_l_n, i_l_b, i_h);
			break;
		case 6:
			remember_label_location_in_imem(imem, &pc, s1, l_n, l_a_b, l_c);
			write_instruction_to_imem(imem, &pc, s2, s3, s4, s5, s6, i_l_n, i_l_b, i_h);
			break;
		default:
			if (DBG_MODE)
				dbg("assembler error!!\n");
			break;
		}
	}

	if (fclose(fptr) == EOF) {
		printf("closing file error!\n");
		exit(1);
	}
}

void label_placement(uint32_t *imem, char *l_n[], char *i_l_n[], uint16_t *l_a_b, uint16_t *i_a_b, uint16_t *l_c, uint16_t *i_h)
{
	for (uint16_t i = 0; i < *l_c; ++i)
		for (uint16_t j = 0; j < *i_h; ++j)
			if (!strcmp(l_n[i], i_l_n[j]))
				imem[i_a_b[j]] |= (l_a_b[i] << INST_IM_OFT);
}

void print_mem(FILE *fptr, uint32_t *imem)
{
	for (uint16_t i = 0; i < IMEM_LEN; ++i)
		fprintf(fptr, "%08X\n", imem[i]);

	if (fclose(fptr) == EOF) {
		printf("closing file error!\n");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("error! wrong number of arguments!\n");
		exit(1);
	}

	FILE *asmfile_ptr = fopen(argv[1], "r");
	FILE *mem_ptr = fopen(argv[2], "w");

	if (!asmfile_ptr || !mem_ptr) {
		printf("error opening file!\n");
		exit(1);
	}

	char *label_names[IMEM_LEN] = { NULL }, *imm_label_names[IMEM_LEN] = { NULL };
	uint16_t label_address_book[IMEM_LEN] = { 0 }, imm_address_book[IMEM_LEN] = { 0 };
	uint16_t label_counter = 0, imm_holders = 0;

	uint32_t simp_imem[IMEM_LEN] = { 0 };

	parse_decode_encode_write(asmfile_ptr, simp_imem, label_names, imm_label_names, label_address_book, imm_address_book, &label_counter, &imm_holders);
	label_placement(simp_imem, label_names, imm_label_names, label_address_book, imm_address_book, &label_counter, &imm_holders);
	print_mem(mem_ptr, simp_imem);

	for (uint16_t i = 0; i < label_counter; ++i)
		free(label_names[i]);
	for (uint16_t i = 0; i < imm_holders; ++i)
		free(imm_label_names[i]);
}