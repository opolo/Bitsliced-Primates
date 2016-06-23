#include <immintrin.h>
#include "Primate.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"

void T2(__m256i (*state)[2], __m256i (*new_state)[2]);

void sbox(__m256i (*state)[2]);
void shiftrows(__m256i (*state)[2]);
void mixcolumns(__m256i (*state)[2]);
void mixcolumns2(__m256i (*state)[2]);
void mixcolumns_inv_2(__m256i (*state)[2]);

void sbox_inv(__m256i (*state)[2]);
void shiftrows_inv(__m256i (*state)[2]);
void mixcolumns_inv(__m256i (*state)[2]);

static const __m256i m256iAllOne = { 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static YMM p1_constants_bit0[12];
static YMM p1_constants_bit1[12];
static YMM p1_constants_bit2[12];
static YMM p1_constants_bit3[12];
static YMM p1_constants_bit4[12];


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
	//p1
	p1_constants_bit0[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[7] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit0[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit0[11] = _mm256_set_epi64x(0, 0, 0, 0);

	p1_constants_bit1[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[7] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit1[8] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit1[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
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

	p1_constants_bit3[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[6] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[7] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit3[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit3[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p1_constants_bit4[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[5] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[7] = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit4[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[10] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit4[11] = _mm256_set_epi64x(0, 0, 0, 0);
}

void test_primates() {

	//Prepare test vectors
	YMM YMM_p1_input[5][2];


	for (int i = 0; i < 5; i++) {
		YMM_p1_input[i][0] = _mm256_setzero_si256();
		YMM_p1_input[i][1] = _mm256_setzero_si256();
	}

	//use test vectors
	p1(YMM_p1_input);

	//Expected values p1:
	YMM YMM_p1_output_expected[5][2];
	YMM_p1_output_expected[0][0] = _mm256_setr_epi64x(18374966855136771840, 72056498804555775, 72056494526300160, 18446742978476114175);
	YMM_p1_output_expected[1][0] = _mm256_setr_epi64x(71776123339472895, 18446463698227757055, 281474959933440, 281474959998975);
	YMM_p1_output_expected[2][0] = _mm256_setr_epi64x(18446744069414649600, 72056498804555775, 71777218572779520, 72056494526300415);
	YMM_p1_output_expected[3][0] = _mm256_setr_epi64x(281470681743615, 18374967950370078975, 18446742974197989375, 4278255615);
	YMM_p1_output_expected[4][0] = _mm256_setr_epi64x(280379759984895, 71776123339472895, 18374687574904996095, 18374966859414896895);

	YMM_p1_output_expected[0][1] = _mm256_setr_epi64x(18446462603027808255, 18374686483949813760, 18446462598732906495, 0);
	YMM_p1_output_expected[1][1] = _mm256_setr_epi64x(281470681808640, 18446463693949566720, 18446744073692839680, 0);
	YMM_p1_output_expected[2][1] = _mm256_setr_epi64x(71776119061217535, 1099511562495, 18374966859414961920, 0);
	YMM_p1_output_expected[3][1] = _mm256_setr_epi64x(18374966855136771840, 18374967950370078720, 1095216660735, 0);
	YMM_p1_output_expected[4][1] = _mm256_setr_epi64x(18446744069414584575, 18446463698227757055, 18374967950353367295, 0);


	//Test if results matched
	for (int i = 0; i < 5; i++) {
		//They get set to all 1 if equal. Else 0.
		YMM p1_compared_0 = _mm256_cmpeq_epi64(YMM_p1_output_expected[i][0], YMM_p1_input[i][0]);
		YMM p1_compared_1 = _mm256_cmpeq_epi64(YMM_p1_output_expected[i][1], YMM_p1_input[i][1]);

		//Iterate over each of the 4 u64 in each ymm
		for (int j = 0; j < 4; j++) {
			u64 equal_p1_0 = p1_compared_0.m256i_u64[j];
			u64 equal_p1_1 = p1_compared_1.m256i_u64[j];


			if (equal_p1_0 == 0 || equal_p1_1 == 0)
				printf("P1 not working \n");
		}
	}

	p1_inv(YMM_p1_input);

	//The last u64 of the second reg-section is not used by our state, and SE makes it all 1's. Make it zero to have it match the expected value of 0 again.
	for (int i = 0; i < 5; i++) {
		YMM_p1_input[i][1].m256i_u64[3] = 0;
	}

	//test if inversing results matched
	for (int i = 0; i < 5; i++) {
		//They get set to all 1 if equal. Else 0.
		YMM p1_compared_0 = _mm256_cmpeq_epi64(_mm256_setzero_si256(), YMM_p1_input[i][0]);
		YMM p1_compared_1 = _mm256_cmpeq_epi64(_mm256_setzero_si256(), YMM_p1_input[i][1]);

		//Iterate over each of the 4 u64 in each ymm
		for (int j = 0; j < 4; j++) {
			u64 equal_p1_0 = p1_compared_0.m256i_u64[j];
			u64 equal_p1_1 = p1_compared_1.m256i_u64[j];


			if (equal_p1_0 == 0 || equal_p1_1 == 0)
				printf("P1 inv not working \n");
		}
	}
}



void p1(YMM(*state)[2]) {
	if (DisablePrimates) {
		return;
	}
	for (int round = 0; round < p1_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		shiftrows(state);

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


void p1_inv(YMM(*state)[2]) {
	if (DisablePrimates) {
		return;
	}
	for (int round = 0; round < p1_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p1_constants_bit0[(p1_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p1_constants_bit1[(p1_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p1_constants_bit2[(p1_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p1_constants_bit3[(p1_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p1_constants_bit4[(p1_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv_2(state);

		//Shift Rows
		shiftrows_inv(state);

		//Sub Bytes
		sbox_inv(state);
	}
}


void sbox_inv(__m256i (*x)[2]) {
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

		y[0] = NEG(XOR10(x[0][i], x[1][i], t[2], t[21], t[26], t[22], t[20], t[17], t[24], t[19]));
		y[1] = XOR7(t[1], t[21], t[26], t[28], t[14], t[15], t[27]);
		y[2] = XOR8(x[1][i], x[4][i], t[0], t[22], t[21], t[20], t[8], t[25]);
		y[3] = XOR6(x[2][i], t[2], t[20], t[23], t[25], t[27]);
		y[4] = XOR8(x[3][i], t[4], t[28], t[6], t[20], t[8], t[23], t[24]);

		x[0][i] = y[0];
		x[1][i] = y[1];
		x[2][i] = y[2];
		x[3][i] = y[3];
		x[4][i] = y[4];
	}
}


void shiftrows_inv(__m256i (*state)[2]) {
	__m256i shuffleControlMaskFirstReg = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		15, 8, 9, 10, 11, 12, 13, 14, //1 
		22, 23, 16, 17, 18, 19, 20, 21, //2
		29, 30, 31, 24, 25, 26, 27, 28); //3
	__m256i shuffleControlMaskSecondReg = _mm256_setr_epi8(
		4, 5, 6, 7, 0, 1, 2, 3, //4
		11, 12, 13, 14, 15, 8, 9, 10, //5
		17, 18, 19, 20, 21, 22, 23, 16, //7
		255, 255, 255, 255, 255, 255, 255, 255); //Setting it to 0xFF makes shuffle zero the bits
	for (int reg = 0; reg < 5; reg++) {

		state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
		state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
	}
}


void mixcolumns_inv(__m256i (*state)[2]) {
	__m256i T2_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T4_regs[5][2];

	__m256i temp_state[5][2];
	for (int i = 0; i < 5; i++) {
		temp_state[i][0] = _mm256_setzero_si256();
		temp_state[i][1] = _mm256_setzero_si256();
	}
	for (int row = 0; row < state_row_count; row++)
	{
		//printf("Before: \n");
		//print_state_as_hex(state);

		//T2
		T2(state, T2_regs);

		T2(T2_regs, T4_regs); //T4
		T2(T4_regs, T8_regs); //T8

							  //T9
		for (int i = 0; i < 5; i++) {
			T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
			T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
		}

		//T15
		for (int i = 0; i < 5; i++) {
			T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
			T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
		}

		//Shuffle bytes downwards due to lines 2,3,4,5,6,7 in inv mix col matrix.
		for (int i = 0; i < 5; i++) {
			temp_state[i][0] = _mm256_permute4x64_epi64(state[i][0], 0b10'01'00'00); // fill second 64 bits with first 64 bits (01 index), third 64 with second 64 bits (10 index) etc.
			temp_state[i][0].m256i_u64[0] = 0;

			temp_state[i][1] = _mm256_permute4x64_epi64(state[i][1], 0b10'01'00'00);
			temp_state[i][1].m256i_u64[0] = state[i][0].m256i_u64[3];
			temp_state[i][1].m256i_u64[3] = 0;	
		}

		//handle first line of inv_matrix. Align all rows on the third u64
		YMM aligned_first_row0[5];
		YMM aligned_first_row1[5];
		YMM aligned_first_row2[5];
		YMM aligned_first_row3[5];
		YMM aligned_first_row4[5];
		YMM aligned_first_row5[5];
		YMM aligned_first_row6[5];
		for (int i = 0; i < 5; i++) {
			aligned_first_row0[i] = _mm256_setr_epi64x(T2_regs[i][0].m256i_u64[0], 0, 0, 0);
			aligned_first_row1[i] = _mm256_setr_epi64x(T15_regs[i][0].m256i_u64[1], 0, 0, 0);
			aligned_first_row2[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[2], 0, 0, 0);
			aligned_first_row3[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[3], 0, 0, 0);
			aligned_first_row4[i] = _mm256_setr_epi64x(T15_regs[i][1].m256i_u64[0], 0, 0, 0);
			aligned_first_row5[i] = _mm256_setr_epi64x(T2_regs[i][1].m256i_u64[1], 0, 0, 0);
			aligned_first_row6[i] = _mm256_setr_epi64x(state[i][1].m256i_u64[2], 0, 0, 0);
		}

		YMM first_row_calculated[5];
		for (int i = 0; i < 5; i++) {
			first_row_calculated[i] = XOR7(aligned_first_row0[i], aligned_first_row1[i], aligned_first_row2[i], aligned_first_row3[i], aligned_first_row4[i], aligned_first_row5[i], aligned_first_row6[i]);
		}

		//Assign data to state
		for (int i = 0; i < 5; i++) {
			state[i][0] = XOR(first_row_calculated[i], temp_state[i][0]);
			state[i][1] = temp_state[i][1];
		}
	}
}

void mixcolumns_inv_2(__m256i (*state)[2]) {
	/*
	Inv_A7:
	2       9       23      31      16      15      29
	29      29      3       25      17      26      16
	16      24      22      7       29      26      31
	31      11      12      10      27      9       1
	1       29      4       5       3       20      11
	11      23      27      29      28      5       2
	2       15      9       9       15      2       1
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T7_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5][2];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T03
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);
	}

	//T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T7
	for (int i = 0; i < 5; i++) {
		T7_regs[i][0] = XOR3(state[i][0], T4_regs[i][0], T2_regs[i][0]);
		T7_regs[i][1] = XOR3(state[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T10
	for (int i = 0; i < 5; i++) {
		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T12
	for (int i = 0; i < 5; i++) {
		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T17
	for (int i = 0; i < 5; i++) {
		T17_regs[i][0] = XOR(state[i][0], T16_regs[i][0]);
		T17_regs[i][1] = XOR(state[i][1], T16_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T22
	for (int i = 0; i < 5; i++) {
		T22_regs[i][0] = XOR3(T16_regs[i][0], T4_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR3(T16_regs[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T24
	for (int i = 0; i < 5; i++) {
		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);
	}

	//T25
	for (int i = 0; i < 5; i++) {
		T25_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], state[i][0]);
		T25_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], state[i][1]);
	}

	//T26
	for (int i = 0; i < 5; i++) {
		T26_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//T31
	for (int i = 0; i < 5; i++) {
		T31_regs[i][0] = XOR5(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], T2_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR5(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	/*
	Inv_A7:
	2       9       23      31      16      15      29
	29      29      3       25      17      26      16
	16      24      22      7       29      26      31
	31      11      12      10      27      9       1
	1       29      4       5       3       20      11
	11      23      27      29      28      5       2
	2       15      9       9       15      2       1
	*/

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_second_state_row_0[5];
	YMM aligned_second_state_row_1[5];
	YMM aligned_second_state_row_2[5];
	YMM aligned_second_state_row_3[5];
	YMM aligned_second_state_row_4[5];
	YMM aligned_second_state_row_5[5];
	YMM aligned_second_state_row_6[5];

	YMM aligned_first_state_row_0[5];
	YMM aligned_first_state_row_1[5];
	YMM aligned_first_state_row_2[5];
	YMM aligned_first_state_row_3[5];
	YMM aligned_first_state_row_4[5];
	YMM aligned_first_state_row_5[5];
	YMM aligned_first_state_row_6[5];
	for (int i = 0; i < 5; i++) {

		aligned_first_state_row_0[i] = _mm256_setr_epi64x(T2_regs[i][0].m256i_u64[0],	T29_regs[i][0].m256i_u64[0], T16_regs[i][0].m256i_u64[0],	T31_regs[i][0].m256i_u64[0]);
		aligned_first_state_row_1[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[1],	T29_regs[i][0].m256i_u64[1], T24_regs[i][0].m256i_u64[1],	T11_regs[i][0].m256i_u64[1]);
		aligned_first_state_row_2[i] = _mm256_setr_epi64x(T23_regs[i][0].m256i_u64[2],	T3_regs[i][0].m256i_u64[2],  T22_regs[i][0].m256i_u64[2],	T12_regs[i][0].m256i_u64[2]);
		aligned_first_state_row_3[i] = _mm256_setr_epi64x(T31_regs[i][0].m256i_u64[3],	T25_regs[i][0].m256i_u64[3], T7_regs[i][0].m256i_u64[3],	T10_regs[i][0].m256i_u64[3]);
		aligned_first_state_row_4[i] = _mm256_setr_epi64x(T16_regs[i][1].m256i_u64[0],	T17_regs[i][1].m256i_u64[0], T29_regs[i][1].m256i_u64[0],	T27_regs[i][1].m256i_u64[0]);
		aligned_first_state_row_5[i] = _mm256_setr_epi64x(T15_regs[i][1].m256i_u64[1],	T26_regs[i][1].m256i_u64[1], T26_regs[i][1].m256i_u64[1],	T9_regs[i][1].m256i_u64[1]);
		aligned_first_state_row_6[i] = _mm256_setr_epi64x(T29_regs[i][1].m256i_u64[2],	T16_regs[i][1].m256i_u64[2], T31_regs[i][1].m256i_u64[2],	state[i][1].m256i_u64[2]);

		aligned_second_state_row_0[i] = _mm256_setr_epi64x(state[i][0].m256i_u64[0],	T11_regs[i][0].m256i_u64[0], T2_regs[i][0].m256i_u64[0], 0);
		aligned_second_state_row_1[i] = _mm256_setr_epi64x(T29_regs[i][0].m256i_u64[1], T23_regs[i][0].m256i_u64[1], T15_regs[i][0].m256i_u64[1], 0);
		aligned_second_state_row_2[i] = _mm256_setr_epi64x(T4_regs[i][0].m256i_u64[2],	T27_regs[i][0].m256i_u64[2], T9_regs[i][0].m256i_u64[2], 0);
		aligned_second_state_row_3[i] = _mm256_setr_epi64x(T5_regs[i][0].m256i_u64[3],	T29_regs[i][0].m256i_u64[3], T9_regs[i][0].m256i_u64[3], 0);
		aligned_second_state_row_4[i] = _mm256_setr_epi64x(T3_regs[i][1].m256i_u64[0],  T28_regs[i][1].m256i_u64[0], T15_regs[i][1].m256i_u64[0], 0);
		aligned_second_state_row_5[i] = _mm256_setr_epi64x(T20_regs[i][1].m256i_u64[1], T5_regs[i][1].m256i_u64[1],  T2_regs[i][1].m256i_u64[1], 0);
		aligned_second_state_row_6[i] = _mm256_setr_epi64x(T11_regs[i][1].m256i_u64[2], T2_regs[i][1].m256i_u64[2],  state[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	YMM last_rowfirststate_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_rowfirststate_calculated[i] = XOR7(aligned_first_state_row_0[i], aligned_first_state_row_1[i], aligned_first_state_row_2[i], aligned_first_state_row_3[i], aligned_first_state_row_4[i], aligned_first_state_row_5[i], aligned_first_state_row_6[i]);
		last_row_calculated[i] = XOR7(aligned_second_state_row_0[i], aligned_second_state_row_1[i], aligned_second_state_row_2[i], aligned_second_state_row_3[i], aligned_second_state_row_4[i], aligned_second_state_row_5[i], aligned_second_state_row_6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = last_rowfirststate_calculated[i];
		state[i][1] = last_row_calculated[i];
	}
}

void mixcolumns(__m256i (*state)[2]) {
	/*
	A7:
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16
	16      26      17      25      3       29      29
	29      15      16      31      23      9       2
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T7_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5][2];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	//T03
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);
	}

	//T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T7
	for (int i = 0; i < 5; i++) {
		T7_regs[i][0] = XOR3(state[i][0], T4_regs[i][0], T2_regs[i][0]);
		T7_regs[i][1] = XOR3(state[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T10
	for (int i = 0; i < 5; i++) {
		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T12
	for (int i = 0; i < 5; i++) {
		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T17
	for (int i = 0; i < 5; i++) {
		T17_regs[i][0] = XOR(state[i][0], T16_regs[i][0]);
		T17_regs[i][1] = XOR(state[i][1], T16_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T22
	for (int i = 0; i < 5; i++) {
		T22_regs[i][0] = XOR3(T16_regs[i][0], T4_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR3(T16_regs[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T24
	for (int i = 0; i < 5; i++) {
		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);
	}

	//T25
	for (int i = 0; i < 5; i++) {
		T25_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], state[i][0]);
		T25_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], state[i][1]);
	}

	//T26
	for (int i = 0; i < 5; i++) {
		T26_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//T31
	for (int i = 0; i < 5; i++) {
		T31_regs[i][0] = XOR5(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], T2_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR5(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];

	YMM aligned_firststate_row0[5];
	YMM aligned_firststate_row1[5];
	YMM aligned_firststate_row2[5];
	YMM aligned_firststate_row3[5];
	YMM aligned_firststate_row4[5];
	YMM aligned_firststate_row5[5];
	YMM aligned_firststate_row6[5];
	for (int i = 0; i < 5; i++) {

		aligned_firststate_row0[i] = _mm256_setr_epi64x(state[i][0].m256i_u64[0], T2_regs[i][0].m256i_u64[0], T11_regs[i][0].m256i_u64[0], state[i][0].m256i_u64[0]);
		aligned_firststate_row1[i] = _mm256_setr_epi64x(T2_regs[i][0].m256i_u64[1], T5_regs[i][0].m256i_u64[1], T20_regs[i][0].m256i_u64[1], T9_regs[i][0].m256i_u64[1]);
		aligned_firststate_row2[i] = _mm256_setr_epi64x(T15_regs[i][0].m256i_u64[2], T28_regs[i][0].m256i_u64[2], T3_regs[i][0].m256i_u64[2], T27_regs[i][0].m256i_u64[2]);
		aligned_firststate_row3[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[3], T29_regs[i][0].m256i_u64[3], T5_regs[i][0].m256i_u64[3], T10_regs[i][0].m256i_u64[3]);
		aligned_firststate_row4[i] = _mm256_setr_epi64x(T9_regs[i][1].m256i_u64[0], T27_regs[i][1].m256i_u64[0], T4_regs[i][1].m256i_u64[0], T12_regs[i][1].m256i_u64[0]);
		aligned_firststate_row5[i] = _mm256_setr_epi64x(T15_regs[i][1].m256i_u64[1], T23_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1], T11_regs[i][1].m256i_u64[1]);
		aligned_firststate_row6[i] = _mm256_setr_epi64x(T2_regs[i][1].m256i_u64[2], T11_regs[i][1].m256i_u64[2], state[i][1].m256i_u64[2], T31_regs[i][1].m256i_u64[2]);

		aligned_last_row0[i] = _mm256_setr_epi64x(T31_regs[i][0].m256i_u64[0], T16_regs[i][0].m256i_u64[0], T29_regs[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(T26_regs[i][0].m256i_u64[1], T26_regs[i][0].m256i_u64[1], T15_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(T29_regs[i][0].m256i_u64[2], T17_regs[i][0].m256i_u64[2], T16_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(T7_regs[i][0].m256i_u64[3], T25_regs[i][0].m256i_u64[3], T31_regs[i][0].m256i_u64[3], 0);
		aligned_last_row4[i] = _mm256_setr_epi64x(T22_regs[i][1].m256i_u64[0], T3_regs[i][1].m256i_u64[0], T23_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(T24_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1], T9_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(T16_regs[i][1].m256i_u64[2], T29_regs[i][1].m256i_u64[2], T2_regs[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	YMM last_rowfirststate_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_rowfirststate_calculated[i] = XOR7(aligned_firststate_row0[i], aligned_firststate_row1[i], aligned_firststate_row2[i], aligned_firststate_row3[i], aligned_firststate_row4[i], aligned_firststate_row5[i], aligned_firststate_row6[i]);
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = last_rowfirststate_calculated[i];
		state[i][1] = last_row_calculated[i];
	}
}

void mixcolumns2(__m256i (*state)[2]) {
	__m256i T2_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T4_regs[5][2];

	__m256i temp_state[5][2];

	for (int row = 0; row < state_row_count; row++)
	{
		//printf("Before: \n");
		//print_state_as_hex(state);

		//T2
		T2(state, T2_regs);

		T2(T2_regs, T4_regs); //T4
		T2(T4_regs, T8_regs); //T8

							  //T9
		for (int i = 0; i < 5; i++) {
			T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
			T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
		}

		//T15
		for (int i = 0; i < 5; i++) {
			T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
			T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
		}

		//Shuffle bytes upwards due to lines 1,2,3,4 in matrix.
		for (int i = 0; i < 5; i++) {
			temp_state[i][0] = _mm256_permute4x64_epi64(state[i][0], 0b00'11'10'01); // fill first 64 bits with second 64 bits (01 index), second 64 with third 64 bits (10 index) etc.
			temp_state[i][0].m256i_u64[3] = state[i][1].m256i_u64[0]; //Test if this is faster when done..

			temp_state[i][1] = _mm256_permute4x64_epi64(state[i][1], 0b00'00'10'01);
			temp_state[i][1].m256i_u64[3] = 0;
			temp_state[i][1].m256i_u64[2] = 0;
		}

		//handle last line of matrix. Align all rows on the third u64
		YMM aligned_last_row0[5];
		YMM aligned_last_row1[5];
		YMM aligned_last_row2[5];
		YMM aligned_last_row3[5];
		YMM aligned_last_row4[5];
		YMM aligned_last_row5[5];
		YMM aligned_last_row6[5];
		for (int i = 0; i < 5; i++) {
			aligned_last_row0[i] = _mm256_setr_epi64x(0, 0, state[i][0].m256i_u64[0], 0);
			aligned_last_row1[i] = _mm256_setr_epi64x(0, 0, T2_regs[i][0].m256i_u64[1], 0);
			aligned_last_row2[i] = _mm256_setr_epi64x(0, 0, T15_regs[i][0].m256i_u64[2], 0);
			aligned_last_row3[i] = _mm256_setr_epi64x(0, 0, T9_regs[i][0].m256i_u64[3], 0);
			aligned_last_row4[i] = _mm256_setr_epi64x(0, 0, T9_regs[i][1].m256i_u64[0], 0);
			aligned_last_row5[i] = _mm256_setr_epi64x(0, 0, T15_regs[i][1].m256i_u64[1], 0);
			aligned_last_row6[i] = _mm256_setr_epi64x(0, 0, T2_regs[i][1].m256i_u64[2], 0);
		}

		YMM last_row_calculated[5];
		for (int i = 0; i < 5; i++) {
			last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
		}

		//Assign data to state
		for (int i = 0; i < 5; i++) {
			state[i][0] = temp_state[i][0];
			state[i][1] = XOR(temp_state[i][1], last_row_calculated[i]);
		}
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
		27, 28, 29, 30, 31, 24, 25, 26); //3
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
