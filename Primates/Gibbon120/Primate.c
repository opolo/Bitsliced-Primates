#include "Primate.h"
#include "Debug.h"

static const __m256i m256iAllOne = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

static YMM p1_constants_bit0[12];
static YMM p1_constants_bit1[12];
static YMM p1_constants_bit2[12];
static YMM p1_constants_bit3[12];
static YMM p1_constants_bit4[12];

static  YMM p2_constants_bit0[6];
static  YMM p2_constants_bit1[6];
static  YMM p2_constants_bit2[6];
static  YMM p2_constants_bit3[6];
static  YMM p2_constants_bit4[6];

static  YMM p3_constants_bit0[6];
static  YMM p3_constants_bit1[6];
static  YMM p3_constants_bit2[6];
static  YMM p3_constants_bit3[6];
static  YMM p3_constants_bit4[6];

void Initialize() {
	/*
	Round constants for p_1:
	01, 02, 05, 0a, 15, 0b, 17, 0e, 1d, 1b, 16, 0c

	Round constants for p_2:
	18, 11, 03, 07, 0f, 1f

	Round constants for p_3:
	1e, 1c, 19, 13, 06, 0d
	*/

	//Set the bits to 1111'1111 in the column two, second row byte, if the roundconstant has a onebit on this indice
	p1_constants_bit0[0]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[1]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[2]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[3]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[4]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[5]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[6]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[7]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[8]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[9]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[11] = _mm256_set_epi64x(0, 0, 0, 0);

	p1_constants_bit1[0]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[1]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[2]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[3]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[4]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[5]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[6]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[7]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[8]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[9]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[10] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[11] = _mm256_set_epi64x(0, 0, 0, 0);

	p1_constants_bit2[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[5] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[7] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[9] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[10] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p1_constants_bit3[0]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[1]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[2]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[3]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[4]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[5]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[6]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[7]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[8]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[9]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p1_constants_bit4[0]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[1]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[2]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[3]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[4]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[5]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[6]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[7]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[8]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[9]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[10] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[11] = _mm256_set_epi64x(0, 0, 0, 0);

	p2_constants_bit0[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit0[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit0[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit0[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit0[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit0[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p2_constants_bit1[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit1[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit1[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit1[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit1[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit1[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p2_constants_bit2[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit2[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit2[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit2[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit2[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit2[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p2_constants_bit3[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit3[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit3[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit3[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit3[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit3[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p2_constants_bit4[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit4[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p2_constants_bit4[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit4[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit4[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p2_constants_bit4[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p3_constants_bit0[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit0[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit0[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit0[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit0[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit0[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p3_constants_bit1[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit1[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit1[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit1[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit1[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit1[5] = _mm256_set_epi64x(0, 0, 0, 0);

	p3_constants_bit2[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit2[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit2[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit2[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit2[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit2[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p3_constants_bit3[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit3[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit3[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit3[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit3[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit3[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p3_constants_bit4[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit4[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit4[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit4[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p3_constants_bit4[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p3_constants_bit4[5] = _mm256_set_epi64x(0, 0, 0, 0);
}

void sbox(__m256i (*x)[2]);
void shiftrows(__m256i (*state)[2]);
void mixcolumns(__m256i (*old)[2]);
void T2(__m256i (*state)[2], __m256i (*new_state)[2]);

void p1(YMM(*state)[2]) {
	for (int round = 0; round < p1_rounds; round++) {
		
		//Sub Bytes
		//sbox(state);

		//Shift Rows
		//shiftrows(state);

		//Mix Columns
		mixcolumns(state);

		
		//Constant Addition
		state[0][0] = XOR(state[0][0], p1_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p1_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p1_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p1_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p1_constants_bit4[round]);
		
	}
}

void p2(YMM(*state)[2]) {
	for (int round = 0; round < p2_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		//shiftrows(state);

		//Mix Columns
		mixcolumns(state);
		
		//Constant Addition
		state[0][0] = XOR(state[0][0], p2_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p2_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p2_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p2_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p2_constants_bit4[round]);
	}
}


void p3(YMM(*state)[2]) {
	for (int round = 0; round < p2_rounds; round++) {
		
		//Sub Bytes
		//sbox(state);

		//Shift Rows
		//shiftrows(state);

		//Mix Columns
		mixcolumns(state);

		//Constant Addition
		state[0][0] = XOR(state[0][0], p3_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p3_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p3_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p3_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p3_constants_bit4[round]);
	}
}


void sbox(__m256i (*x)[2]) {
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

		y[0] = NEG(XOR4(x[0][i], x[3][i], t[1], t[6]));
		y[1] = XOR5(x[4][i], t[0], t[3], t[7], t[8]);
		y[2] = XOR5(x[3][i], x[4][i], t[10], t[3], t[9]);
		y[3] = XOR5(x[1][i], x[4][i], t[10], t[5], t[7]);
		y[4] = XOR7(x[1][i], x[2][i], x[3][i], t[4], t[2], t[6], t[8]);

		x[0][i] = y[0];
		x[1][i] = y[1];
		x[2][i] = y[2];
		x[3][i] = y[3];
		x[4][i] = y[4];

	}
}

void shiftrows(__m256i (*state)[2]) {
	__m256i shuffleControlMaskFirstReg = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		9, 10, 11, 12, 13, 14, 15, 8, //1 
		18, 19, 20, 21, 22, 23, 16, 17, //2
		27, 28, 29, 30, 28, 24, 25, 26); //3
	__m256i shuffleControlMaskSecondReg = _mm256_setr_epi8(
		4, 5, 6, 7, 0, 1, 2, 3, //4
		13, 14, 15, 8, 9, 10, 11, 12, //5
		23, 16, 17, 18, 19, 20, 21, 22, //7
		255, 255, 255, 255, 255, 255, 255, 255); //Setting it to 0xFF makes shuffle zero the bits
	for (int reg = 0; reg < 5; reg++) {

		state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
		state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
	}
}

void mixcolumns(__m256i (*old)[2]) {
	__m256i T2_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i temp_regs[5][2];

	__m256i new_state[5][2];

	//printf("\n\nOld:");
	//print_state_as_hex(old);

	//T2
	T2(old, T2_regs);

	//printf("T2:");
	//print_state_as_hex(T2_regs);

	T2(T2_regs, temp_regs); //T4
	
	//printf("T4:");
	//print_state_as_hex(temp_regs);

	T2(temp_regs, T9_regs); //T8
	
	//printf("T8:");
	//print_state_as_hex(T9_regs);

	T2(T9_regs, T15_regs); //T16

	//printf("T16:");
	//print_state_as_hex(T15_regs);

	//T9
	for (int i = 0; i < 5; i++) { 
		T9_regs[i][0] = XOR(old[i][0], T9_regs[i][0]);
		T9_regs[i][1] = XOR(old[i][1], T9_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) { 
		T15_regs[i][0] = XOR(old[i][0], T15_regs[i][0]);
		T15_regs[i][1] = XOR(old[i][1], T15_regs[i][1]);
	}

	//printf("T15:");
	//print_state_as_hex(T15_regs);

	//printf("T9:");
	//print_state_as_hex(T9_regs);

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

void T2(__m256i (*state)[2], __m256i (*new_state)[2]) {
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