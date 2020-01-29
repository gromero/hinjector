#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#define r3 	3
#define r4	4
#define r5	5
#define r6	6

#define DEV	"/dev/injector"

#define OPCODE_SHIFT	26u
#define RA_SHIFT	21u
#define RT_SHIFT	16u
#define SH04_SHIFT	11u
#define SH5_SHIFT	1u
#define ME_SHIFT	5u
#define RC_SHIFT	0u
#define LEV_SHIFT	5u
#define AA_SHIFT	1u
#define LK_SHIFT	0u
#define BO_SHIFT	21u
#define BH_SHIFT	11u

#define RA_MASK		0x1F
#define RT_MASK		0x1F
#define SI16_MASK	0xFFFF
#define SH04_MASK	0x1F
#define SH5_MASK	0x1
#define ME_MASK		0x3F
#define RC_MASK		0x1
#define LEV_MASK	0x7F
#define AA_MASK		0x1
#define LK_MASK		0x1
#define BO_MASK		0x1F
#define BH_MASK		0x1F

/* Branch conditions and hints */
#define BO_BRANCH_ALWAYS	0x14
#define BH_SUBROUTINE_RETURN	0

#define ADDI_OPCODE	(14u << OPCODE_SHIFT)
#define ORI_OPCODE	(24u << OPCODE_SHIFT)
#define ADDIS_OPCODE	(15u << OPCODE_SHIFT)
#define RLDICR_OPCODE	(30u << OPCODE_SHIFT)
#define SC_OPCODE	(17u << OPCODE_SHIFT)
#define BCLR_OPCODE	(19u << OPCODE_SHIFT)
#define ORIS_OPCODE	(25u << OPCODE_SHIFT)

static uint32_t codecache[1024*10];
static const uint32_t *first_instr = &codecache[0];
static uint32_t *instr_ptr = &codecache[0];


// load <reg>

// li *
// lis *
// addi *
// ori *
// oris *
// sldi *
// sc 1 *
// blr *

uint32_t sh04_field(int sh) {
	return (sh & SH04_MASK) << SH04_SHIFT;
}

uint32_t sh5_field(int sh) {
	return ((sh >> 5u) & SH5_MASK) << SH5_SHIFT;
}

uint32_t me_field(int me) {
	uint32_t me5, me04;

	me5 = (me >> 5u) & 1;
	me04 = me << 1u;
	me = (me5 | me04) & ME_MASK;
	me = me << ME_SHIFT;
	return me;
}

uint32_t rc_field(int rc) {
	return (rc & RC_MASK) << RC_SHIFT;
}

uint32_t bit_field(int b, int mask, int shift) {
	return (b & mask) << shift;
}

/*
 * XXX: sanitize unsigned / signed case
 */
uint32_t usimm(int si, int nbits) {
	return si & ((1 << nbits)-1);
}

uint32_t simm(int si, int nbits) {
	return si & ((1 << nbits)-1);
}

uint32_t rt(int r) {
	return (r & RT_MASK) << RA_SHIFT;
}

uint32_t ra(int r) {
	return (r & RA_MASK) << RT_SHIFT;
}

uint32_t rs(int r) {
	return rt(r);
}

uint32_t rldicr(int a, int s, int sh, int me) {
	return RLDICR_OPCODE | rs(s) | ra(a) | sh04_field(sh) | me_field(me) | bit_field(1, 0x7, 2) | sh5_field(sh) | rc_field(0);
}

uint32_t rldicr_(int a, int s, int sh, int me) {
	return RLDICR_OPCODE | rs(s) | ra(a) | sh04_field(sh) | me_field(me) | bit_field(1, 0x7, 2) | sh5_field(sh) | rc_field(1);
}

uint32_t sldi(int a, int s, int n) {
	return rldicr(a, s, n, 63-n);
}

uint32_t ori(int a, int s, uint16_t usimm16) {
	uint32_t instruction;

	instruction = ORI_OPCODE | rs(s) | ra(a) | usimm(usimm16, 16);
	return instruction;
}

uint32_t oris(int s, int a, uint16_t usimm16) {
	uint32_t instruction;

	instruction = ORIS_OPCODE | rs(s) | ra(a) | usimm(usimm16, 16);
	return instruction;
}

uint32_t addi(int t, int a, int16_t simm16) {
	uint32_t instruction;

	instruction = ADDI_OPCODE | rt(t) | ra(a) | simm(simm16, 16);
	return instruction;
}

uint32_t li(int d, int16_t si16) {
	return addi(d, 0, si16);
}

uint32_t addis(int t, int a, int16_t usimm16) {
	uint32_t instruction;

	instruction = ADDIS_OPCODE | rt(t) | ra(a) | simm(usimm16, 16);
	return instruction;
}

uint32_t lis(int t, int16_t simm16) {
	return addis(t, 0, simm16);
}

uint32_t sc(int lev) {
	return  SC_OPCODE | bit_field(lev, LEV_MASK, LEV_SHIFT) | bit_field(1, 0x1, 1);
}

uint32_t blr(void) {
	return BCLR_OPCODE | bit_field(BO_BRANCH_ALWAYS, BO_MASK, BO_SHIFT) |
	       /* No BI field */
	       bit_field(BH_SUBROUTINE_RETURN, BH_MASK, BH_SHIFT) |
	       bit_field(16, 0x3FF, 1) |
	       /* LK = 0 */ 0u;
}

/* Macro */
void load(int r, int64_t imm64) {
	short xa, xb, xc, xd;
	// x = xa xb xc xd
	xd = imm64 & 0xFFFF;
	xc = (imm64 >> 16) & 0xFFFF;
	xb = (imm64 >> 32) & 0xFFFF;
	xa = (imm64 >> 48) & 0xFFFF;

	*instr_ptr = lis(r, xa); instr_ptr++;
	*instr_ptr = ori(r, r, xb); instr_ptr++;
	*instr_ptr = sldi(r, r, 32); instr_ptr++;
	*instr_ptr = oris(r, r, xc); instr_ptr++;
	*instr_ptr = ori(r, r, xd); instr_ptr++;
}

/* Helpers */
// Print a instruction
void pi(uint32_t instr) {
	printf("%#.8x\n", instr);
}
// Print whole codecache
void pcodecache() {
	uint32_t *instr;

	for (instr = &codecache[0]; instr < instr_ptr; instr++)
		pi(*instr);
}

int main(int argc, char *argv[])
{
	int fd;
	uint32_t *i = instr_ptr;

	fd = open(DEV, O_RDWR);
 	if (fd < 0) {
		perror("open()");
		exit(1);
	}

	printf("Openning device " DEV "... \n");
	printf("Codecache is @%p\n", instr_ptr);

//	printf("%#.8x\n", addi(3, 0, 0xbe));
//	printf("%#.8x\n", li(3, 0xbe));
//	printf("%#.8x\n", ori(3, 4, 0xbe));
//	pi(blr());
//	pcodecache();
//	load(3, 0xdeadbeefc0debabe);

	*i = li(r3, 42 /* '*' */); i++;
//	*i = sldi(r6, r3, (24+32)); i++;
//	rldicr  r6,r3,56,7
	*i = rldicr(r6,r3,56,7); i++;
	*i = li(r3, 0x58); i++;
	*i = li(r4, 0); i++;
	*i = li(r5, 1); i++;
	*i = sc(1); i++;

	instr_ptr = i;

	pcodecache();
}
