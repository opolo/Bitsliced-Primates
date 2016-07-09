#include "Primate.h"
#include "Debug.h"
#include <stdio.h>

void sbox(__m256i (*x)[2]);
void shiftrows(__m256i (*state)[2]);
void mixcolumns(__m256i (*old)[2]);
void T2(__m256i (*state)[2], __m256i (*new_state)[2]);

static __m256i m256iAllOne;
static YMM shuffleControlMaskFirstReg;
static YMM shuffleControlMaskSecondReg;

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
	m256iAllOne = _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF);

	shuffleControlMaskFirstReg = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		9, 10, 11, 12, 13, 14, 15, 8, //1 
		18, 19, 20, 21, 22, 23, 16, 17, //2
		27, 28, 29, 30, 31, 24, 25, 26); //3
	shuffleControlMaskSecondReg = _mm256_setr_epi8(
		4, 5, 6, 7, 0, 1, 2, 3, //4
		13, 14, 15, 8, 9, 10, 11, 12, //5
		23, 16, 17, 18, 19, 20, 21, 22, //7
		255, 255, 255, 255, 255, 255, 255, 255); //Setting it to 0xFF makes shuffle zero the bits

	//Set the bits to 1111'1111 in the column two, second row byte, if the roundconstant has a onebit on this indice
	//p1
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

	p1_constants_bit2[0]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[1]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[2]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[3]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[4]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[5]  = _mm256_set_epi64x(0, 0, 0, 0);
	p1_constants_bit2[6]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[7]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[8]  = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p1_constants_bit2[9]  = _mm256_set_epi64x(0, 0, 0, 0);
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


	//p2
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


	//p3
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
	YMM_p1_output_expected[0][0] = _mm256_setr_epi64x(18374966855136771840ULL, 72056498804555775ULL, 72056494526300160ULL, 18446742978476114175ULL);
	YMM_p1_output_expected[1][0] = _mm256_setr_epi64x(71776123339472895ULL, 18446463698227757055ULL, 281474959933440ULL, 281474959998975ULL);
	YMM_p1_output_expected[2][0] = _mm256_setr_epi64x(18446744069414649600ULL, 72056498804555775ULL, 71777218572779520ULL, 72056494526300415ULL);
	YMM_p1_output_expected[3][0] = _mm256_setr_epi64x(281470681743615ULL, 18374967950370078975ULL, 18446742974197989375ULL, 4278255615ULL);
	YMM_p1_output_expected[4][0] = _mm256_setr_epi64x(280379759984895ULL, 71776123339472895ULL, 18374687574904996095ULL, 18374966859414896895ULL);

	YMM_p1_output_expected[0][1] = _mm256_setr_epi64x(18446462603027808255ULL, 18374686483949813760ULL, 18446462598732906495ULL, 0ULL);
	YMM_p1_output_expected[1][1] = _mm256_setr_epi64x(281470681808640ULL, 18446463693949566720ULL, 18446744073692839680ULL, 0ULL);
	YMM_p1_output_expected[2][1] = _mm256_setr_epi64x(71776119061217535ULL, 1099511562495ULL, 18374966859414961920ULL, 0ULL);
	YMM_p1_output_expected[3][1] = _mm256_setr_epi64x(18374966855136771840ULL, 18374967950370078720ULL, 1095216660735ULL, 0ULL);
	YMM_p1_output_expected[4][1] = _mm256_setr_epi64x(18446744069414584575ULL, 18446463698227757055ULL, 18374967950353367295ULL, 0ULL);


	//Test if results matched
	//They get set to all 1 if equal. Else 0.
	YMM p1_compared_0 = _mm256_cmpeq_epi64(YMM_p1_output_expected[0][0], YMM_p1_input[0][0]);
	YMM p1_compared_1 = _mm256_cmpeq_epi64(YMM_p1_output_expected[0][1], YMM_p1_input[0][1]);
	if (_mm256_extract_epi64(p1_compared_0, 0) == 0 || _mm256_extract_epi64(p1_compared_0, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_0, 2) == 0 || _mm256_extract_epi64(p1_compared_0, 3) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 0) == 0 || _mm256_extract_epi64(p1_compared_1, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 2) == 0 || _mm256_extract_epi64(p1_compared_1, 3) == 0) {
		printf("P1 not working \n");
	}

	//They get set to all 1 if equal. Else 0.
	p1_compared_0 = _mm256_cmpeq_epi64(YMM_p1_output_expected[1][0], YMM_p1_input[1][0]);
	p1_compared_1 = _mm256_cmpeq_epi64(YMM_p1_output_expected[1][1], YMM_p1_input[1][1]);
	if (_mm256_extract_epi64(p1_compared_0, 0) == 0 || _mm256_extract_epi64(p1_compared_0, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_0, 2) == 0 || _mm256_extract_epi64(p1_compared_0, 3) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 0) == 0 || _mm256_extract_epi64(p1_compared_1, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 2) == 0 || _mm256_extract_epi64(p1_compared_1, 3) == 0) {
		printf("P1 not working \n");
	}

	//They get set to all 1 if equal. Else 0.
	p1_compared_0 = _mm256_cmpeq_epi64(YMM_p1_output_expected[2][0], YMM_p1_input[2][0]);
	p1_compared_1 = _mm256_cmpeq_epi64(YMM_p1_output_expected[2][1], YMM_p1_input[2][1]);
	if (_mm256_extract_epi64(p1_compared_0, 0) == 0 || _mm256_extract_epi64(p1_compared_0, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_0, 2) == 0 || _mm256_extract_epi64(p1_compared_0, 3) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 0) == 0 || _mm256_extract_epi64(p1_compared_1, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 2) == 0 || _mm256_extract_epi64(p1_compared_1, 3) == 0) {
		printf("P1 not working \n");
	}

	//They get set to all 1 if equal. Else 0.
	p1_compared_0 = _mm256_cmpeq_epi64(YMM_p1_output_expected[3][0], YMM_p1_input[3][0]);
	p1_compared_1 = _mm256_cmpeq_epi64(YMM_p1_output_expected[3][1], YMM_p1_input[3][1]);
	if (_mm256_extract_epi64(p1_compared_0, 0) == 0 || _mm256_extract_epi64(p1_compared_0, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_0, 2) == 0 || _mm256_extract_epi64(p1_compared_0, 3) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 0) == 0 || _mm256_extract_epi64(p1_compared_1, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 2) == 0 || _mm256_extract_epi64(p1_compared_1, 3) == 0) {
		printf("P1 not working \n");
	}

	//They get set to all 1 if equal. Else 0.
	p1_compared_0 = _mm256_cmpeq_epi64(YMM_p1_output_expected[4][0], YMM_p1_input[4][0]);
	p1_compared_1 = _mm256_cmpeq_epi64(YMM_p1_output_expected[4][1], YMM_p1_input[4][1]);
	if (_mm256_extract_epi64(p1_compared_0, 0) == 0 || _mm256_extract_epi64(p1_compared_0, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_0, 2) == 0 || _mm256_extract_epi64(p1_compared_0, 3) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 0) == 0 || _mm256_extract_epi64(p1_compared_1, 1) == 0 ||
		_mm256_extract_epi64(p1_compared_1, 2) == 0 || _mm256_extract_epi64(p1_compared_1, 3) == 0) {
		printf("P1 not working \n");
	}
}

void p1(YMM(*state)[2]) {
	for (int round = 0; round < p1_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
		}

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
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
		}

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
	for (int round = 0; round < p3_rounds; round++) {
		
		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg);
		}

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
	__m256i T5_regs[5];
	__m256i T7_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T23_regs[5];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T29
	for (int i = 0; i < 5; i++) {

		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T7_regs[i] = XOR(T5_regs[i], T2_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i] = XOR(T8_regs[i][0], T2_regs[i][0]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T12_regs[i] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T11_regs[i][0], T4_regs[i][0]);
		T15_regs[i][1] = XOR(T11_regs[i][1], T4_regs[i][1]);

		T17_regs[i] = XOR(state[i][0], T16_regs[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i] = XOR(T20_regs[i][1], T3_regs[i][1]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T25_regs[i] = XOR(T24_regs[i][0], state[i][0]);

		T26_regs[i][0] = XOR(T24_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR(T24_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T26_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR(T26_regs[i][1], state[i][1]);

		T28_regs[i] = XOR(T20_regs[i][0], T8_regs[i][0]);

		T29_regs[i][0] = XOR(T20_regs[i][0], T9_regs[i][0]);
		T29_regs[i][1] = XOR(T20_regs[i][1], T9_regs[i][1]);

		T31_regs[i][0] = XOR(T29_regs[i][0], T2_regs[i][0]);
		T31_regs[i][1] = XOR(T29_regs[i][1], T2_regs[i][1]);
	}

	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T2_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(state[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i], 1), _mm256_extract_epi64(T20_regs[i][0], 1), _mm256_extract_epi64(T9_regs[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 2), _mm256_extract_epi64(T28_regs[i], 2), _mm256_extract_epi64(T3_regs[i][0], 2), _mm256_extract_epi64(T27_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][0], 3), _mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3), _mm256_extract_epi64(T10_regs[i], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][1], 0), _mm256_extract_epi64(T27_regs[i][1], 0), _mm256_extract_epi64(T4_regs[i][1], 0), _mm256_extract_epi64(T12_regs[i], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T23_regs[i], 1), _mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T11_regs[i][1], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][1], 2), _mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(state[i][1], 2), _mm256_extract_epi64(T31_regs[i][1], 2)));


		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][0], 0), _mm256_extract_epi64(T16_regs[i][0], 0), _mm256_extract_epi64(T29_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T15_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][0], 2), _mm256_extract_epi64(T17_regs[i], 2), _mm256_extract_epi64(T16_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T7_regs[i], 3), _mm256_extract_epi64(T25_regs[i], 3), _mm256_extract_epi64(T31_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i], 0), _mm256_extract_epi64(T3_regs[i][1], 0), _mm256_extract_epi64(T23_regs[i], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T24_regs[i][1], 1), _mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T9_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T16_regs[i][1], 2), _mm256_extract_epi64(T29_regs[i][1], 2), _mm256_extract_epi64(T2_regs[i][1], 2), 0));
	}
}

void T2(__m256i (*state)[2], __m256i (*new_state)[2]) {
	//Shift the bits in each primate element one to the left (i.e. shift bits between regs). and reduce with GF(2^5) mask x^5+x^2+1.
	new_state[0][0] = state[4][0];
	new_state[0][1] = state[4][1];

	new_state[1][0] = state[0][0];
	new_state[1][1] = state[0][1];

	new_state[2][0] = XOR(state[1][0], state[4][0]);
	new_state[2][1] = XOR(state[1][1], state[4][1]);

	new_state[3][0] = state[2][0];
	new_state[3][1] = state[2][1];

	new_state[4][0] = state[3][0];
	new_state[4][1] = state[3][1];
}