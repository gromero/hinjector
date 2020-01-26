#define r3 3
#define r4 4
#define r5 5
#define r6 6


_start:
	li      3, 42
	sldi    r6,r3,(24+32)
	li      r3,0x58
	li      r4,0
	li      r5,1
	.long   0x44000022
	blr
