#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"

void andrey_sbox_test();

//SCHWABE APE CONSTANTS: (bit 0 = LSB)
static const __m256i ape_constants_bit0[12] =
{
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, OneBitsAtCol2, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 }
};

static const __m256i ape_constants_bit1[12] =
{
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, }
};

static const __m256i ape_constants_bit2[12] =
{
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, }
};

static const __m256i ape_constants_bit3[12] =
{
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, }
};

static const __m256i ape_constants_bit4[12] =
{
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, OneBitsAtCol2, 0, 0, },
	{ 0, 0, 0, 0, }
};


static const __m256i m256iAllOne = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, };


void schwabe_bitsliced_primate(__m256i (*state)[2]);
void schwabe_bitsliced_primate_inv(__m256i (*state)[2]);
void T2_schwabe(__m256i (*state)[2], __m256i (*new_state)[2]);
void T9_schwabe(__m256i (*state)[2], __m256i (*new_state)[2]);
void T15_schwabe(__m256i (*state)[2], __m256i (*new_state)[2]);
void T_2_9_15_schwabe(__m256i (*old_state)[2], __m256i (*t2_state)[2], __m256i (*t9_state)[2], __m256i (*t15_state)[2]);



void schwabe_primate_test() {
	__m256i state[5][2];

	//Vector to use START
	state[0][0] = _mm256_set1_epi8(0x00);
	state[0][1] = _mm256_set1_epi8(0x00);

	state[1][0] = _mm256_set1_epi8(0xFF);
	state[1][1] = _mm256_set1_epi8(0xFF);

	state[2][0] = _mm256_set1_epi8(0x00);
	state[2][1] = _mm256_set1_epi8(0x00);

	state[3][0] = _mm256_set1_epi8(0x00);
	state[3][1] = _mm256_set1_epi8(0x00);

	state[4][0] = _mm256_set1_epi8(0xFF);
	state[4][1] = _mm256_set1_epi8(0xFF);
	//Vector to use END

	andrey_sbox_test();

	schwabe_bitsliced_primate(state);
	schwabe_bitsliced_primate_inv(state);
}

//Round is zeroindex, so round 12 is integer 11.
void constant_addition_schwabe(__m256i (*state)[2], int round) {
	state[0][0] = XOR(state[0][0], ape_constants_bit0[round]);
	state[1][0] = XOR(state[1][0], ape_constants_bit1[round]);
	state[2][0] = XOR(state[2][0], ape_constants_bit2[round]);
	state[3][0] = XOR(state[3][0], ape_constants_bit3[round]);
	state[4][0] = XOR(state[4][0], ape_constants_bit4[round]);
}

#define testYel(a, b) XOR(a, b)
#define testRed(a, b) AND(NEG(b), a)

void andrey_sbox_test() {

	__m256i x[5];
	__m256i y[5];
	x[2] = _mm256_set1_epi8(0xFF); 
	x[1] = _mm256_setzero_si256();
	x[0] = _mm256_setzero_si256();
	x[3] = _mm256_setzero_si256();
	x[4] = _mm256_setzero_si256();

	__m256i t[40];

	t[1] =  testYel(x[0], x[3]);
	t[2] =  testYel(x[1], x[2]);
	t[9] =  testYel(x[2], x[3]);
	t[10] = testYel(x[1], x[3]);
	t[11] = testYel(x[0], x[2]);

	t[3] =  testYel(x[0], x[4]);
	t[4] =  testYel(x[4], t[1]);
	t[5] =  testYel(x[1], t[3]);
	t[6] =  testYel(x[2], t[1]);
	t[7] =  testYel(x[4], t[2]);
	t[12] = testRed(x[1], t[1]); //R
	t[13] = testRed(t[10], t[11]); //R
	t[18] = testRed(t[3], t[9]); //R
	
	t[8] =  testYel(x[2], t[4]);
	t[14] = testRed(t[6], t[5]); //R
	t[16] = testRed(t[2], t[4]); //R
	t[17] = testRed(t[3], t[4]); //R
	t[20] = testYel(t[9], t[18]);
	t[26] = testYel(t[11], t[13]);
	t[28] = testYel(t[2], t[12]);
	t[33] = testYel(t[10], t[3]);

	t[15] = testRed(t[7], t[8]); //R
	t[22] = testYel(t[4], t[16]);
	t[23] = testYel(t[8], t[14]);
	t[29] = testYel(x[1], t[26]);

	t[19] = testYel(t[15], t[17]);
	t[25] = testYel(t[3], t[22]);
	t[35] = testYel(t[22], t[26]);

	t[21] = testYel(t[19], t[20]);
	t[27] = testYel(t[1], t[25]);

	t[24] = testYel(t[21], t[23]);
	t[31] = testYel(t[27], t[28]);
	t[32] = testYel(t[19], t[27]);
	
	y[0] = testYel(t[24], t[33]);
	y[1] = testYel(t[24], t[35]);
	y[2] = testYel(t[29], t[32]);
	y[3] = testYel(t[21], t[25]);
	y[4] = testYel(t[24], t[31]);
}

void T_2_9_15_schwabe(__m256i (*old_state)[2], __m256i (*t2_state)[2], __m256i (*t9_state)[2], __m256i (*t15_state)[2]) {

	//**T2**
	T2_schwabe(old_state, t2_state);

	//**T9**
	T2_schwabe(t2_state, t9_state); //T4
	T2_schwabe(t9_state, t15_state); //T8

	for (int i = 0; i < 5; i++) { 
		//T9
		t9_state[i][0] = XOR(old_state[i][0], t15_state[i][0]);
		t9_state[i][1] = XOR(old_state[i][1], t15_state[i][1]);
	}

	//**15**
	__m256i temp[5][2];
	T2_schwabe(t15_state, temp); //T16

	//T15
	for (int i = 0; i < 5; i++) {
		t15_state[i][0] = XOR(old_state[i][0], temp[i][0]);
		t15_state[i][1] = XOR(old_state[i][1], temp[i][1]);
	}

}

void T9_schwabe(__m256i (*state)[2], __m256i (*new_state)[2]) {
	__m256i temp[5][2];
	T2_schwabe(state, new_state); //T2
	T2_schwabe(new_state, temp); //T4
	T2_schwabe(temp, new_state); //T8

	//T9
	for (int i = 0; i < 5; i++) {
		new_state[i][0] = XOR(state[i][0], new_state[i][0]);
		new_state[i][1] = XOR(state[i][1], new_state[i][1]);
	}
}

void T15_schwabe(__m256i (*state)[2], __m256i (*new_state)[2]) {
	__m256i temp[5][2];
	T2_schwabe(state, new_state); //T2
	T2_schwabe(new_state, temp); //T4
	T2_schwabe(temp, new_state); //T8
	T2_schwabe(new_state, temp); //T16

	//T15
	for (int i = 0; i < 5; i++) {
		new_state[i][0] = XOR(state[i][0], temp[i][0]);
		new_state[i][1] = XOR(state[i][1], temp[i][1]);
	}
}

void T2_schwabe(__m256i (*state)[2], __m256i (*new_state)[2]) {
	//Shift the bits in each primate element one to the left (i.e. shift bits between regs). and reduce with GF(2^5) mask x^5+x^2+1.
	__m256i GF_reduce_mask0 = state[4][0];
	__m256i GF_reduce_mask1 = state[4][1];

	new_state[0][0] = XOR(_mm256_setzero_si256(), GF_reduce_mask0);
	new_state[0][1] = XOR(_mm256_setzero_si256(), GF_reduce_mask1);

	new_state[1][0] = state[0][0];
	new_state[1][1] = state[0][1];

	new_state[2][0] = XOR(state[1][0], GF_reduce_mask0);
	new_state[2][1] = XOR(state[1][1], GF_reduce_mask1);

	new_state[3][0] = state[2][0];
	new_state[3][1] = state[2][1];

	new_state[4][0] = state[3][0];
	new_state[4][1] = state[3][1];
}

void primate(__m256i *state) {
}

void sbox_schwabe(__m256i (*x)[2]) {
	//YMM 0 = LSB
	//YMM 4 = MSB

	for (int i = 0; i < 2; i++) {
		//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).

		//Helper variables
		__m256i t[13];
		__m256i y[5];
		

		t[0] = AND(x[0][i], x[1][i]);
		t[1] = AND(x[0][i], x[2][i]);
		t[2] = AND(x[0][i], x[3][i]);
		t[3] = AND(x[0][i], x[4][i]);
		t[4] = AND(x[1][i], x[2][i]);
		t[5] = AND(x[1][i], x[3][i]);
		t[6] = AND(x[1][i], x[4][i]);
		t[7] = AND(x[2][i], x[3][i]);
		t[8] = AND(x[2][i], x[4][i]);
		t[9] = AND(x[3][i], x[4][i]);
		t[10] = XOR(t[1], t[4]);
		
		y[0] = NEG(XOR4(x[0][i], x[3][i],	t[1],	 t[6]));
		y[1] = XOR5(	x[4][i], t[0],		t[3],	 t[7],		t[8]);
		y[2] = XOR5(	x[3][i], x[4][i],	t[10],	 t[3],		t[9]);
		y[3] = XOR5(	x[1][i], x[4][i],	t[10],	 t[5],		t[7]);
		y[4] = XOR7(	x[1][i], x[2][i],	x[3][i], t[4],		t[2],	t[6],	t[8]);

		x[0][i] = y[0];
		x[1][i] = y[1];
		x[2][i] = y[2];
		x[3][i] = y[3];
		x[4][i] = y[4];

	}
}
void sbox_schwabe_inv(__m256i (*x)[2]) {
	//YMM 0 = LSB
	//YMM 4 = MSB


	//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).
	for (int i = 0; i < 2; i++) {


		//Helper variables
		__m256i t[30];
		__m256i y[5];


		t[0] = AND(x[0][i], x[1][i]); //x0x1
		t[1] = AND(x[0][i], x[2][i]); //x0x2
		t[2] = AND(x[0][i], x[3][i]); //x0x3
		t[3] = AND(x[0][i], x[4][i]); //x0x4
		t[4] = AND(x[1][i], x[2][i]); //x1x2
		t[5] = AND(x[1][i], x[3][i]); //x1x3
		t[6] = AND(x[1][i], x[4][i]); //x1x4
		t[7] = AND(x[2][i], x[3][i]); //x2x3
		t[8] = AND(x[2][i], x[4][i]); //x2x4
		t[9] = AND(x[3][i], x[4][i]); //x3x4

		t[10] = AND(t[0], x[2][i]); //x0x1x2
		t[11] = AND(t[0], x[3][i]); //x0x1x3
		t[12] = AND(t[0], x[4][i]); //x0x1x4
		t[13] = AND(t[1], x[3][i]); //x0x2x3
		t[14] = AND(t[1], x[4][i]); //x0x2x4
		t[15] = AND(t[2], x[4][i]); //x0x3x4
		t[16] = AND(t[4], x[3][i]); //x1x2x3
		t[17] = AND(t[4], x[4][i]); //x1x2x4
		t[18] = AND(t[5], x[4][i]); //x1x3x4
		t[19] = AND(t[7], x[4][i]); //x2x3x4
		
		t[20] = XOR(t[12], t[14]);
		t[21] = XOR(t[4], t[7]);
		t[22] = XOR(t[5], t[6]);
		t[23] = XOR3(t[0], t[10], t[17]);
		t[24] = XOR(t[9], t[11]);
		t[25] = XOR(t[16], t[15]);
		t[26] = XOR(x[2][i], t[13]);
		t[27] = XOR(x[1][i], t[18]);
		t[28] = XOR(x[4][i], t[3]);

		y[0] = NEG(XOR10(x[0][i], x[1][i], t[2],		t[21],	t[26], t[22],	t[20], t[17], t[24], t[19]));
		y[1] =	   XOR7(t[1],	 t[21],	  t[26],	t[28],	t[14], t[15],	t[27]);
		y[2] =	   XOR8(x[1][i], x[4][i], t[0],     t[22],	t[21], t[20],	t[8], t[25]);
		y[3] =	   XOR6(x[2][i], t[2],	  t[20],	t[23],	t[25], t[27]);
		y[4] =	   XOR8(x[3][i], t[4],	  t[28],	t[6],	t[20], t[8],	t[23], t[24]);

		x[0][i] = y[0];
		x[1][i] = y[1];
		x[2][i] = y[2];
		x[3][i] = y[3];
		x[4][i] = y[4];

	}
}

void schwabe_shiftrows(__m256i (*state)[2]){
	//0,1,2,3,4,5,7
	//TODO
	__m256i shuffleControlMaskFirstReg = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0,
		   												 9, 10, 11, 12, 13, 14, 15, 8,
		   												 18, 19, 20, 21, 22, 23, 16, 17,
		   												 27, 28, 29, 30, 31, 24, 25, 26);
	__m256i shuffleControlMaskSecondReg = _mm256_set_epi8(4, 5, 6, 7, 0, 1, 2, 3,
														  13, 14, 15, 8, 9, 10, 11, 12,
														  23, 16, 17, 18, 19, 20, 21, 22,
														  256, 256, 256, 256, 256, 256, 256, 256); //Setting it to 256 makes shuffle zero the bits
	for (int reg = 0; reg < 5; reg++) {

		state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
		state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
	}
}
void schwabe_shiftrows_inv(__m256i (*state)[2]) {
	//shifted from top down:
	//0,1,2,3,4,5,7
	//TODO
	__m256i shuffleControlMaskFirstReg = _mm256_set_epi8(0,	 0,	 0,	0,	0,	0,	0,	0,
														 15, 8,	 9,	10,	11,	12,	13,	14,
														 22, 23, 16, 17, 18, 19, 20, 21,
														 29, 30, 31, 24, 25, 26, 27, 28);
	__m256i shuffleControlMaskSecondReg = _mm256_set_epi8(4,  5,  6,	7,	0,  1,  2,  3,
														  11, 12, 13,	14,	15, 8,	9,	10,
														  17, 18, 19,	20, 21, 22, 23, 16,
														  256, 256, 256, 256, 256, 256, 256, 256); //Setting it to 256 makes shuffle zero the bits
	for (int reg = 0; reg < 5; reg++) {

		state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
		state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
	}
}

void mix_col_schwabe(__m256i (*old)[2]) {
	__m256i T2_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];

	__m256i new_state[5][2];
	T_2_9_15_schwabe(old, T2_regs, T9_regs, T15_regs);

	//Shufle first reg (handle lines 1,2,3,4 in matrix)
	for (int i = 0; i < 5; i++) {
		new_state[i][0] = _mm256_permute4x64_epi64(old[i][0], 0b01101100); // fill first 64 bits with second 64 bits (01 index), next 64 with third 64 bits (10 index) etc.
		new_state[i][0].m256i_u64[3] = old[i][0].m256i_u64[0]; //Test if this is faster when done..
	}


	//Calculate last row for second reg
	for (int i = 0; i < 5; i++) {
		__m256i i0_row, i1_row, i2_row, i3_row, i4_row, i5_row, i6_row = { 0, 0, 0, 0 };
		i0_row.m256i_i64[3] = old[i][0].m256i_u64[0];
		i1_row.m256i_i64[3] = T2_regs[i][0].m256i_u64[1];
		i2_row.m256i_i64[3] = T15_regs[i][0].m256i_u64[2];
		i3_row.m256i_i64[3] = T9_regs[i][0].m256i_u64[3];
		i4_row.m256i_i64[3] = T9_regs[i][1].m256i_u64[0];
		i5_row.m256i_i64[3] = T15_regs[i][1].m256i_u64[1];
		i6_row.m256i_i64[3] = T2_regs[i][1].m256i_u64[2];

		//XOR all the values together, and store them in reg 2 + shuffle reg 2
		new_state[i][1] = _mm256_permute4x64_epi64(old[i][1], 0b01101111);
		new_state[i][1] = XOR(new_state[i][1], XOR4(i0_row, i1_row, i2_row, XOR4(i3_row, i4_row, i5_row, i6_row)));
	}

	for (int i = 0; i < 5; i++) {
		old[i][0] = new_state[i][0];
		old[i][0] = new_state[i][1];
	}
}
void mix_col_schwabe_inv(__m256i (*old)[2]) {
	__m256i T2_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];

	__m256i new_state[5][2];
	T_2_9_15_schwabe(old, T2_regs, T9_regs, T15_regs);

	//Calculate and shuffle first reg
	for (int i = 0; i < 5; i++) {
		__m256i i0_row, i1_row, i2_row, i3_row, i4_row, i5_row, i6_row = { 0, 0, 0, 0 };
		i0_row.m256i_i64[0] = T2_regs[i][0].m256i_u64[0];
		i1_row.m256i_i64[0] = T15_regs[i][0].m256i_u64[1];
		i2_row.m256i_i64[0] = T9_regs[i][0].m256i_u64[2];
		i3_row.m256i_i64[0] = T9_regs[i][0].m256i_u64[3];
		i4_row.m256i_i64[0] = T15_regs[i][1].m256i_u64[0];
		i5_row.m256i_i64[0] = T2_regs[i][1].m256i_u64[1];
		i6_row.m256i_i64[0] = old[i][1].m256i_u64[2];

		//XOR all the values together, and store them in YMM
		new_state[i][0] = _mm256_permute4x64_epi64(old[i][1], 0b00000110);
		new_state[i][0].m256i_u64[0] = 0; //We cannot zero with permute, so we have to do it this way.
		new_state[i][0] = XOR(new_state[i][0], XOR4(i0_row, i1_row, i2_row, XOR4(i3_row, i4_row, i5_row, i6_row)));
	}

	//Shuffle second reg
	for (int i = 0; i < 5; i++) {
		new_state[i][1] = _mm256_permute4x64_epi64(old[i][1], 0b00000110);
		new_state[i][1].m256i_u64[0] = old[i][1].m256i_u64[3];
	}

	for (int i = 0; i < 5; i++) {
		old[i][0] = new_state[i][0];
		old[i][0] = new_state[i][1];
	}
}


void schwabe_bitsliced_primate(__m256i (*state)[2]) {

	__m256i temp[5][2];

	for (int round = 0; round < PrimateRounds; round++) {
		
		//Constant addition
		//constant_addition_schwabe(state, round);

		//Sub Bytes
		sbox_schwabe(state);

		//ShiftRows
		//schwabe_shiftrows(state);

		//Mix Columns
		//mix_col_schwabe(state);

	}
}

void schwabe_bitsliced_primate_inv(__m256i (*state)[2]) {

	__m256i temp[5][2];

	for (int round = 0; round < PrimateRounds; round++) {
		
		//Mix Columns inv
		//mix_col_schwabe_inv(state);

		//ShiftRowsInv
		//schwabe_shiftrows_inv(state);

		//Sub bytes inv
		sbox_schwabe_inv(state);

		//Constant addition
		//constant_addition_schwabe(state, (PrimateRounds-1)-round);
	}
}
































//SBOX end

		/*
		//Constant addition
		

		//Mix Columns
		//TODO

		
		
		//Sub Elements
		//TODO
	}
}


//void primate(__m256i *state) {
//	__m256i states[5][2];
//	for (int i = 0; i < 5; i++) {
//		states[i][0] = _mm256_setzero_si256();
//	}
//
//	for (int round = 0; round < PrimateRounds; round++) {
//		/*
//		//Constant addition
//		//XOR a roundconstant to the second element of the second row (47th element of the capacity <-- probably wrong qqq)
//		//Round constants for p1: 01, 02, 05, 0a, 15, 0b, 17, 0e, 1d, 1b, 16, 0c
//		//Array where the constants are transposed to the 47th bit:
//		const u64 roundConstantBitsYMM0[] = { 70368744177664,	0,				70368744177664, 0,
//		70368744177664,	70368744177664, 70368744177664, 0,
//		70368744177664,	70368744177664,	0,				0 };
//
//		const u64 roundConstantBitsYMM1[] = { 0,				70368744177664, 0,				70368744177664,
//		0,				70368744177664, 70368744177664, 70368744177664,
//		0,				70368744177664, 70368744177664, 0 };
//
//		const u64 roundConstantBitsYMM2[] = { 0,				0,				70368744177664, 0,
//		70368744177664,	0,				70368744177664, 70368744177664,
//		70368744177664,	0,				70368744177664, 70368744177664 };
//
//		const u64 roundConstantBitsYMM3[] = { 0,				0,				0,				70368744177664,
//		0,				70368744177664, 0,				70368744177664,
//		70368744177664,	70368744177664, 0,				70368744177664 };
//
//		const u64 roundConstantBitsYMM4[] = { 0,				0,				0,				0,
//		70368744177664,	0,				70368744177664, 0,
//		70368744177664,	70368744177664, 70368744177664, 0 };
//
//		__m256i rc_ymm0 = _mm256_set1_epi64x(roundConstantBitsYMM0[round]);
//		__m256i rc_ymm1 = _mm256_set1_epi64x(roundConstantBitsYMM1[round]);
//		__m256i rc_ymm2 = _mm256_set1_epi64x(roundConstantBitsYMM2[round]);
//		__m256i rc_ymm3 = _mm256_set1_epi64x(roundConstantBitsYMM3[round]);
//		__m256i rc_ymm4 = _mm256_set1_epi64x(roundConstantBitsYMM4[round]);
//		states[0] = _mm256_xor_si256(states[0], rc_ymm0);
//		states[1] = _mm256_xor_si256(states[1], rc_ymm1);
//		states[2] = _mm256_xor_si256(states[2], rc_ymm2);
//		states[3] = _mm256_xor_si256(states[3], rc_ymm3);
//		states[4] = _mm256_xor_si256(states[4], rc_ymm4);
//		*/
//
//		//Mix Columns
//		//TODO
//
//		//Shift Rows primate 120
//		//We do this by shifting each byte (which corresponds to all columns in a row), high and low, and then blend them together. After that we blend the bytes together and finally load them into the registers. 
//		//shifted from top down:
//		//0,1,2,3,4,5,7
//		//TODO
//		__m256i shuffleControlMaskLowLaneFirstReg = _mm256_set1_epi16(0); //TODO THESE
//		__m256i shuffleControlMaskHighLaneFirstReg = _mm256_set1_epi16(0);
//		__m256i shuffleControlMaskLowLaneSecondReg = _mm256_set1_epi16(0);
//		__m256i shuffleControlMaskHighLaneSecondReg = _mm256_set1_epi16(0);
//		for (int reg = 0; reg < 5; reg++) {
//			__m256i temp;
//
//			temp = _mm256_shuffle_epi8(states[reg][0], shuffleControlMaskLowLaneFirstReg); //Lower lane is shuffled correctly
//			states[reg][0] = _mm256_inserti128_si256(_mm256_shuffle_epi8(states[reg][0], shuffleControlMaskHighLaneFirstReg), _mm256_castsi256_si128(temp), 0); //Higher lane is shuffled correctly, and combined with lower lane.
//
//			temp = _mm256_shuffle_epi8(states[reg][1], shuffleControlMaskLowLaneSecondReg); //Lower lane is shuffled correctly
//			states[reg][1] = _mm256_inserti128_si256(_mm256_shuffle_epi8(states[reg][0], shuffleControlMaskHighLaneSecondReg), _mm256_castsi256_si128(temp), 0); //Higher lane is shuffled correctly, and combined with lower lane.
//		}
//
//		//Sub Elements
//		//TODO
//	}
//}

void p1_inv(__m256i *states) {}


/* OLD SHIFTREG

void schwabe_shiftrows(__m256i (*state)[2], __m256i (*new_state)[2]){
//SHIFT ROWS (primate 120) START
//shifted to the left (from top down):
//0,1,2,3,4,5,7
//TODO
__m256i shuffleControlMaskFirstReg = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0,
9, 10, 11, 12, 13, 14, 15, 8,
18, 19, 20, 21, 22, 23, 16, 17,
27, 28, 29, 30, 31, 24, 25, 26);
__m256i shuffleControlMaskSecondReg = _mm256_set_epi8(4, 5, 6, 7, 0, 1, 2, 3,
13, 14, 15, 8, 9, 10, 11, 12,
23, 16, 17, 18, 19, 20, 21, 22,
256, 256, 256, 256, 256, 256, 256, 256); //Setting it to 256 makes shuffle zero the bits
for (int reg = 0; reg < 5; reg++) {

new_state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
new_state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
}
}
void schwabe_shiftrows_inv(__m256i (*state)[2], __m256i (*new_state)[2]) {
//shifted from top down:
//0,1,2,3,4,5,7
//TODO
__m256i shuffleControlMaskLowLaneFirstReg = _mm256_set1_epi16(0); //TODO THESE
__m256i shuffleControlMaskHighLaneFirstReg = _mm256_set1_epi16(0);
__m256i shuffleControlMaskLowLaneSecondReg = _mm256_set1_epi16(0);
__m256i shuffleControlMaskHighLaneSecondReg = _mm256_set1_epi16(0);
for (int reg = 0; reg < 5; reg++) {
__m256i temp;

temp = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskLowLaneFirstReg); //Lower lane is shuffled correctly
new_state[reg][0] = _mm256_inserti128_si256(_mm256_shuffle_epi8(state[reg][0], shuffleControlMaskHighLaneFirstReg), _mm256_castsi256_si128(temp), 0); //Higher lane is shuffled correctly, and combined with lower lane.

temp = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskLowLaneSecondReg); //Lower lane is shuffled correctly
new_state[reg][1] = _mm256_inserti128_si256(_mm256_shuffle_epi8(state[reg][0], shuffleControlMaskHighLaneSecondReg), _mm256_castsi256_si128(temp), 0); //Higher lane is shuffled correctly, and combined with lower lane.
}
}

*/