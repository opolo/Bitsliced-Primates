#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"

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

void sbox_schwabe(__m256i (*old)[2]) {
	//YMM 0 = LSB
	//YMM 4 = MSB

	for (int i = 0; i < 2; i++) {
		//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).

		//Helper variables
		__m256i z[13];
		__m256i q[13];
		__m256i t[13];
		__m256i y[5];

		z[0] = XOR(old[4][i], old[0][i]);
		z[1] = XOR(old[3][i], old[2][i]);
		z[2] = XOR(old[2][i], old[1][i]);

		q[0] = XOR(old[4][i], old[1][i]);
		t[0] = OR(q[0], old[3][i]);
		q[2] = XOR(old[3][i], old[1][i]);

		q[3] = NEG(XOR(old[4][i], old[2][i]));
		t[1] = OR(q[2], q[3]);
		q[4] = XOR(old[3][i], z[0]);

		q[5] = XOR(old[4][i], z[2]);
		t[2] = AND(q[4], q[5]);
		q[6] = NEG(XOR(old[0][i], q[5]));


		q[7] = XOR(old[0][i], z[1]);
		t[3] = OR(q[6], q[7]);
		q[8] = XOR(q[4], z[2]);

		z[9] = XOR(t[0], t[3]);
		q[9] = XOR(old[2][i], z[9]);
		t[4] = AND(q[8], q[9]);

		q[10] = NEG(XOR(old[1][i], z[0]));
		t[5] = AND(q[10], z[0]);
		q[12] = NEG(XOR4(z[1], z[9], t[2], t[4]));

		t[6] = AND(q[12], z[2]);
		z[3] = XOR(t[5], t[6]);
		z[4] = XOR(t[3], z[3]);


		z[5] = XOR(t[2], z[4]);
		z[6] = XOR(t[1], t[6]);
		z[7] = XOR(t[4], z[5]);

		z[8] = XOR(t[1], z[7]);
		z[10] = XOR(t[0], z[7]);
		z[11] = XOR(t[4], z[4]);
		z[12] = XOR(z[6], z[11]);

		y[4] = NEG(XOR(q[2], z[5]));
		y[3] = XOR(z[0], z[8]);
		y[2] = XOR(q[7], z[12]);
		y[1] = XOR(q[6], z[11]);
		y[0] = XOR(old[2][i], z[10]);

		old[0][i] = y[0];
		old[1][i] = y[1];
		old[2][i] = y[2];
		old[3][i] = y[3];
		old[4][i] = y[4];

	}
}
void sbox_schwabe_inv(__m256i (*old)[2]) {
	//YMM 0 = LSB
	//YMM 4 = MSB

	//reverse bits as my algo is wrong right now...
	__m256i old_rev[5][2];
	for (int i = 0; i < 5; i++) {
		old_rev[4-i][0] = old[i][0];
		old_rev[4-i][1] = old[i][1];
	}

	//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).
	for (int i = 0; i < 2; i++) {


		//Helper variables
		__m256i q[20];
		__m256i t[13];
		__m256i y[5];

		q[0] = XOR3(old_rev[0][i], old_rev[1][i], old_rev[2][i]);
		q[1] = NEG(XOR(old_rev[2][i], old_rev[4][i]));
		t[0] = AND(q[0], q[1]);

		q[2] = old_rev[0][i];
		q[3] = old_rev[1][i];
		t[1] = AND(q[2], q[3]);

		q[4] = XOR3(old_rev[2][i], old_rev[3][i], t[0]);
		q[5] = NEG(old_rev[1][i]);
		t[2] = AND(q[4], q[5]);

		q[6] = XOR3(old_rev[1][i], t[1], t[2]);
		q[7] = XOR(old_rev[2][i], old_rev[4][i]);
		t[3] = AND(q[6], q[7]);

		q[8] = XOR4(old_rev[2][i], t[0], t[2], t[3]);
		q[9] = XOR4(old_rev[0][i], old_rev[3][i], old_rev[4][i], XOR3(t[1], t[2], t[3]));
		t[4] = AND(q[8], q[9]);

		q[10] = XOR4(old_rev[0][i], old_rev[2][i], old_rev[3][i], XOR3(t[1], t[2], t[3]));
		q[11] = XOR4(old_rev[1][i], old_rev[3][i], t[0], t[2]);
		t[5] = AND(q[10], q[11]);

		q[12] = XOR(old_rev[0][i], old_rev[4][i]);
		q[13] = XOR4(t[0], t[3], t[4], t[5]);
		t[6] = AND(q[12], q[13]);

		q[14] = NEG(XOR4(old_rev[0][i], old_rev[1][i], old_rev[2][i], XOR4(old_rev[4][i], t[0], t[1], XOR4(t[3], t[4], t[5], t[6]))));
		q[15] = XOR4(old_rev[0][i], old_rev[3][i], t[0], XOR4(t[1], t[2], t[4], t[6]));
		t[7] = AND(q[14], q[15]);

		q[16] = NEG(XOR4(old_rev[2][i], old_rev[3][i], t[2], t[5]));
		q[17] = NEG(XOR4(old_rev[0][i], old_rev[1][i], old_rev[4][i], XOR4(t[0], t[1], t[2], XOR3(t[3], t[6], t[7]))));
		t[8] = AND(q[16], q[17]);

		q[18] = XOR4(old_rev[4][i], t[2], t[5], XOR(t[6], t[8]));
		q[19] = NEG(XOR4(old_rev[0][i], old_rev[1][i], old_rev[4][i], XOR3(t[4], t[7], t[8])));
		t[9] = AND(q[18], q[19]);

		old[4][i] = XOR4(old_rev[0][i], old_rev[1][i], t[0], XOR3(t[6], t[7], t[9]));
		old[3][i] = XOR(t[0], t[3], t[6]);
		old[2][i] = XOR4(t[3], t[5], t[6], t[7]);
		old[1][i] = XOR3(t[1], t[2], t[4]);;
		old[0][i] = XOR4(old_rev[1][i], t[0], t[4], t[8]);

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
		constant_addition_schwabe(state, round);

		//Sub Bytes
		sbox_schwabe(state);

		//ShiftRows
		schwabe_shiftrows(state);

		//Mix Columns
		mix_col_schwabe(state);

	}
}

void schwabe_bitsliced_primate_inv(__m256i (*state)[2]) {

	__m256i temp[5][2];

	for (int round = 0; round < PrimateRounds; round++) {
		
		//Mix Columns inv
		mix_col_schwabe_inv(state);

		//ShiftRowsInv
		schwabe_shiftrows_inv(state);

		//Sub bytes inv
		sbox_schwabe_inv(state);

		//Constant addition
		constant_addition_schwabe(state, (PrimateRounds-1)-round);
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