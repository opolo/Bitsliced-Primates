#include <immintrin.h>
#include "Primate.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"

void T2(__m256i (*state)[2], __m256i (*new_state)[2]);

void sbox(__m256i (*state)[2]);
void sbox_inv(__m256i (*state)[2]);

void mixcolumns(__m256i (*state)[2]);
void mixcolumns_inv(__m256i (*state)[2]);

void mixcolumns_80bit(__m256i (*state)[2]);
void mixcolumns_inv_80bit(__m256i (*state)[2]);

static YMM m256iAllOne;
static YMM invShuffleControlMaskFirstReg_120bit;
static YMM invShuffleControlMaskSecondReg_120bit;
static YMM shuffleControlMaskFirstReg_120bit;
static YMM shuffleControlMaskSecondReg_120bit;
static YMM invShuffleControlMaskFirstReg_80bit;
static YMM invShuffleControlMaskSecondReg_80bit;
static YMM shuffleControlMaskFirstReg_80bit;
static YMM shuffleControlMaskSecondReg_80bit;

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

static YMM p4_constants_bit0[12];
static YMM p4_constants_bit1[12];
static YMM p4_constants_bit2[12];
static YMM p4_constants_bit3[12];
static YMM p4_constants_bit4[12];


void Initialize() {
	/*
	Round constants for p_1:
	01, 02, 05, 0a, 15, 0b, 17, 0e, 1d, 1b, 16, 0c

	Round constants for p_2:
	18, 11, 03, 07, 0f, 1f

	Round constants for p_3:
	1e, 1c, 19, 13, 06, 0d
	*/

	m256iAllOne = _mm256_set1_epi64x(0xFFFFFFFFFFFFFFFF);

	invShuffleControlMaskFirstReg_120bit = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		15, 8, 9, 10, 11, 12, 13, 14, //1 
		22, 23, 16, 17, 18, 19, 20, 21, //2
		29, 30, 31, 24, 25, 26, 27, 28); //3
	invShuffleControlMaskSecondReg_120bit = _mm256_setr_epi8(
		4, 5, 6, 7, 0, 1, 2, 3, //4
		11, 12, 13, 14, 15, 8, 9, 10, //5
		17, 18, 19, 20, 21, 22, 23, 16, //7
		255, 255, 255, 255, 255, 255, 255, 255); //Setting it to 0xFF makes shuffle zero the bits
	shuffleControlMaskFirstReg_120bit = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		9, 10, 11, 12, 13, 14, 15, 8, //1 
		18, 19, 20, 21, 22, 23, 16, 17, //2
		27, 28, 29, 30, 31, 24, 25, 26); //3
	shuffleControlMaskSecondReg_120bit = _mm256_setr_epi8(
		4, 5, 6, 7, 0, 1, 2, 3, //4
		13, 14, 15, 8, 9, 10, 11, 12, //5
		23, 16, 17, 18, 19, 20, 21, 22, //7
		255, 255, 255, 255, 255, 255, 255, 255); //Setting it to 0xFF makes shuffle zero the bits



	shuffleControlMaskFirstReg_80bit = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		9, 10, 11, 12, 13, 14, 15, 8, //1 
		18, 19, 20, 21, 22, 23, 16, 17, //2
		28, 29, 30, 31, 24, 25, 26, 27); //4
	shuffleControlMaskSecondReg_80bit = _mm256_setr_epi8(
		7, 0, 1, 2, 3, 4, 5, 6, //7
		255, 255, 255, 255, 255, 255, 255, 255, //Setting it to 0xFF makes shuffle zero the bits
		255, 255, 255, 255, 255, 255, 255, 255, //Setting it to 0xFF makes shuffle zero the bits
		255, 255, 255, 255, 255, 255, 255, 255); //Setting it to 0xFF makes shuffle zero the bits

	invShuffleControlMaskFirstReg_80bit = _mm256_setr_epi8(
		0, 1, 2, 3, 4, 5, 6, 7, //0
		15, 8, 9, 10, 11, 12, 13, 14, //1 
		22, 23, 16, 17, 18, 19, 20, 21, //2
		28, 29, 30, 31, 24, 25, 26, 27); //4
	invShuffleControlMaskSecondReg_80bit = _mm256_setr_epi8(
		1, 2, 3, 4, 5, 6, 7, 0, //7
		255, 255, 255, 255, 255, 255, 255, 255, //Setting it to 0xFF makes shuffle zero the bits
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255);

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

	p4_constants_bit0[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit0[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[6] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit0[7] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit0[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit0[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit0[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p4_constants_bit1[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit1[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit1[2] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[7] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit1[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[9] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit1[10] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit1[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p4_constants_bit2[0] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit2[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit2[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit2[3] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit2[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit2[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit2[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit2[7] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit2[8] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit2[9] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit2[10] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit2[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p4_constants_bit3[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit3[1] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit3[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit3[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit3[4] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit3[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit3[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit3[7] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit3[8] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit3[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit3[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit3[11] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);

	p4_constants_bit4[0] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[1] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[2] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit4[3] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit4[4] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit4[5] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[6] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[7] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[8] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[9] = _mm256_set_epi64x(0, 0, 0b0000000000000000000000000000000000000000000000001111111100000000, 0);
	p4_constants_bit4[10] = _mm256_set_epi64x(0, 0, 0, 0);
	p4_constants_bit4[11] = _mm256_set_epi64x(0, 0, 0, 0);
}

void test_primates() {

	////////////////////TEST 120 BIT//////////////

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

	p1_inv(YMM_p1_input);

	//test if inversing results matched... Dont test last 64 bits of section 2, as they are not part of the state (and sub elements turn the 0s there to 1s...)
	if (_mm256_extract_epi64(YMM_p1_input[0][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[0][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[0][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input[0][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[1][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[1][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[1][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input[1][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[2][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[2][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[2][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input[2][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[3][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[3][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[3][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input[3][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[4][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[4][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[4][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input[4][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[0][1], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[0][1], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[0][1], 2) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[1][1], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[1][1], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[1][1], 2) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[2][1], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[2][1], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[2][1], 2) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[3][1], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[3][1], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[3][1], 2) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[4][1], 0) != 0 || _mm256_extract_epi64(YMM_p1_input[4][1], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input[4][1], 2) != 0) {
		printf("P1 inv not working \n");
	}


	////////////////////TEST 80 BIT//////////////
	//Prepare test vectors
	YMM YMM_p1_input_80bit[5][2];


	for (int i = 0; i < 5; i++) {
		YMM_p1_input_80bit[i][0] = _mm256_setzero_si256();
		YMM_p1_input_80bit[i][1] = _mm256_setzero_si256();
	}

	//use test vectors
	p1_80bit(YMM_p1_input_80bit);
	p1_inv_80bit(YMM_p1_input_80bit);

	//test if vectors are zero again... Dont test last 192 bits of section 2, as they are not part of the state (and sub elements turn the 0s there to 1s...)
	if (_mm256_extract_epi64(YMM_p1_input_80bit[0][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[0][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[0][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[0][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[1][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[1][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[1][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[1][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[2][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[2][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[2][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[2][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[3][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[3][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[3][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[3][0], 3) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[4][0], 0) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[4][0], 1) != 0 ||
		_mm256_extract_epi64(YMM_p1_input_80bit[4][0], 2) != 0 || _mm256_extract_epi64(YMM_p1_input_80bit[4][0], 3) != 0 ||

		_mm256_extract_epi64(YMM_p1_input_80bit[0][1], 0) != 0 || 
		_mm256_extract_epi64(YMM_p1_input_80bit[1][1], 0) != 0 || 
		_mm256_extract_epi64(YMM_p1_input_80bit[2][1], 0) != 0 || 
		_mm256_extract_epi64(YMM_p1_input_80bit[3][1], 0) != 0 || 
		_mm256_extract_epi64(YMM_p1_input_80bit[4][1], 0) != 0) {
		printf("P1 inv not working \n");
	}

}




//********Permutations********
void p1(YMM(*state)[2]) {
	for (int round = 0; round < p1_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_120bit);
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
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_120bit);
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
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_120bit);
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

void p4(YMM(*state)[2]) {
	for (int round = 0; round < p4_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_120bit);
		}

		//Mix Columns
		mixcolumns(state);

		//Constant Addition
		state[0][0] = XOR(state[0][0], p4_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p4_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p4_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p4_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p4_constants_bit4[round]);
	}
}

void p1_inv(YMM(*state)[2]) {
	for (int round = 0; round < p1_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p1_constants_bit0[(p1_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p1_constants_bit1[(p1_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p1_constants_bit2[(p1_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p1_constants_bit3[(p1_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p1_constants_bit4[(p1_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_120bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}

void p2_inv(YMM(*state)[2]) {
	for (int round = 0; round < p2_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p2_constants_bit0[(p2_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p2_constants_bit1[(p2_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p2_constants_bit2[(p2_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p2_constants_bit3[(p2_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p2_constants_bit4[(p2_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_120bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}

void p3_inv(YMM(*state)[2]) {
	for (int round = 0; round < p3_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p3_constants_bit0[(p3_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p3_constants_bit1[(p3_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p3_constants_bit2[(p3_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p3_constants_bit3[(p3_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p3_constants_bit4[(p3_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_120bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}

void p4_inv(YMM(*state)[2]) {
	for (int round = 0; round < p4_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p4_constants_bit0[(p4_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p4_constants_bit1[(p4_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p4_constants_bit2[(p4_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p4_constants_bit3[(p4_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p4_constants_bit4[(p4_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_120bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_120bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}


void p1_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p1_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_80bit);
		}

		//Mix Columns
		mixcolumns_80bit(state);

		//Constant Addition
		state[0][0] = XOR(state[0][0], p1_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p1_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p1_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p1_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p1_constants_bit4[round]);
	}
}

void p2_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p2_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_80bit);
		}

		//Mix Columns
		mixcolumns_80bit(state);

		//Constant Addition
		state[0][0] = XOR(state[0][0], p2_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p2_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p2_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p2_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p2_constants_bit4[round]);
	}
}

void p3_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p3_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_80bit);
		}

		//Mix Columns
		mixcolumns_80bit(state);

		//Constant Addition
		state[0][0] = XOR(state[0][0], p3_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p3_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p3_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p3_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p3_constants_bit4[round]);
	}
}

void p4_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p4_rounds; round++) {

		//Sub Bytes
		sbox(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], shuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], shuffleControlMaskSecondReg_80bit);
		}

		//Mix Columns
		mixcolumns_80bit(state);

		//Constant Addition
		state[0][0] = XOR(state[0][0], p4_constants_bit0[round]);
		state[1][0] = XOR(state[1][0], p4_constants_bit1[round]);
		state[2][0] = XOR(state[2][0], p4_constants_bit2[round]);
		state[3][0] = XOR(state[3][0], p4_constants_bit3[round]);
		state[4][0] = XOR(state[4][0], p4_constants_bit4[round]);
	}
}

void p1_inv_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p1_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p1_constants_bit0[(p1_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p1_constants_bit1[(p1_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p1_constants_bit2[(p1_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p1_constants_bit3[(p1_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p1_constants_bit4[(p1_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv_80bit(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_80bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}

void p2_inv_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p2_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p2_constants_bit0[(p2_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p2_constants_bit1[(p2_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p2_constants_bit2[(p2_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p2_constants_bit3[(p2_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p2_constants_bit4[(p2_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv_80bit(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_80bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}

void p3_inv_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p3_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p3_constants_bit0[(p3_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p3_constants_bit1[(p3_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p3_constants_bit2[(p3_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p3_constants_bit3[(p3_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p3_constants_bit4[(p3_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv_80bit(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_80bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}

void p4_inv_80bit(YMM(*state)[2]) {
	for (int round = 0; round < p4_rounds; round++) {

		//Constant Addition
		state[0][0] = XOR(state[0][0], p4_constants_bit0[(p4_rounds - 1) - round]);
		state[1][0] = XOR(state[1][0], p4_constants_bit1[(p4_rounds - 1) - round]);
		state[2][0] = XOR(state[2][0], p4_constants_bit2[(p4_rounds - 1) - round]);
		state[3][0] = XOR(state[3][0], p4_constants_bit3[(p4_rounds - 1) - round]);
		state[4][0] = XOR(state[4][0], p4_constants_bit4[(p4_rounds - 1) - round]);

		//Mix Columns
		mixcolumns_inv_80bit(state);

		//Shift Rows
		for (int reg = 0; reg < 5; reg++) {
			state[reg][0] = _mm256_shuffle_epi8(state[reg][0], invShuffleControlMaskFirstReg_80bit);
			state[reg][1] = _mm256_shuffle_epi8(state[reg][1], invShuffleControlMaskSecondReg_80bit);
		}

		//Sub Bytes
		sbox_inv(state);
	}
}




//******120 bit transformations******
void mixcolumns_inv(__m256i (*state)[2]) {
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
	__m256i T7_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T23_regs[5];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5];
	__m256i T26_regs[5];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T7_regs[i] = XOR(T5_regs[i][0], T2_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i] = XOR(T8_regs[i][0], T2_regs[i][0]);

		T11_regs[i][0] = XOR(T9_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR(T9_regs[i][1], T2_regs[i][1]);

		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T3_regs[i][0], T12_regs[i][0]);
		T15_regs[i][1] = XOR(T3_regs[i][1], T12_regs[i][1]);

		T17_regs[i] = XOR(state[i][1], T16_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][0], T2_regs[i][0]);

		T23_regs[i] = XOR(T16_regs[i][0], T7_regs[i]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T25_regs[i] = XOR(T24_regs[i][0], state[i][0]);

		T26_regs[i] = XOR(T24_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T24_regs[i][0], T3_regs[i][0]);
		T27_regs[i][1] = XOR(T24_regs[i][1], T3_regs[i][1]);

		T28_regs[i] = XOR(T24_regs[i][1], T4_regs[i][1]);

		T29_regs[i][0] = XOR(T24_regs[i][0], T5_regs[i][0]);
		T29_regs[i][1] = XOR(T24_regs[i][1], T5_regs[i][1]);

		T31_regs[i][0] = XOR(T11_regs[i][0], T20_regs[i][0]);
		T31_regs[i][1] = XOR(T11_regs[i][1], T20_regs[i][1]);
	}

	YMM temp_first_half; //This one is needed since the old first half is needed to calculate the second half... 
	for (int i = 0; i < 5; i++) {
		temp_first_half = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 0), _mm256_extract_epi64(T29_regs[i][0], 0), _mm256_extract_epi64(T16_regs[i][0], 0), _mm256_extract_epi64(T31_regs[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][0], 1), _mm256_extract_epi64(T29_regs[i][0], 1), _mm256_extract_epi64(T24_regs[i][0], 1), _mm256_extract_epi64(T11_regs[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T23_regs[i], 2), _mm256_extract_epi64(T3_regs[i][0], 2), _mm256_extract_epi64(T22_regs[i], 2), _mm256_extract_epi64(T12_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][0], 3), _mm256_extract_epi64(T25_regs[i], 3), _mm256_extract_epi64(T7_regs[i], 3), _mm256_extract_epi64(T10_regs[i], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T16_regs[i][1], 0), _mm256_extract_epi64(T17_regs[i], 0), _mm256_extract_epi64(T29_regs[i][1], 0), _mm256_extract_epi64(T27_regs[i][1], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T26_regs[i], 1), _mm256_extract_epi64(T26_regs[i], 1), _mm256_extract_epi64(T9_regs[i][1], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][1], 2), _mm256_extract_epi64(T16_regs[i][1], 2), _mm256_extract_epi64(T31_regs[i][1], 2), _mm256_extract_epi64(state[i][1], 2)));

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(T2_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][0], 1), _mm256_extract_epi64(T23_regs[i], 1), _mm256_extract_epi64(T15_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T4_regs[i][0], 2), _mm256_extract_epi64(T27_regs[i][0], 2), _mm256_extract_epi64(T9_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i][0], 3), _mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64(T9_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T3_regs[i][1], 0), _mm256_extract_epi64(T28_regs[i], 0), _mm256_extract_epi64(T15_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i][1], 1), _mm256_extract_epi64(T5_regs[i][1], 1), _mm256_extract_epi64(T2_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(T2_regs[i][1], 2), _mm256_extract_epi64(state[i][1], 2), 0));

		state[i][0] = temp_first_half;
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

		T28_regs[i] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);

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


//******80 bit transformations******
void mixcolumns_80bit(__m256i (*state)[2]) {
	/*
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	20      1       8       19      15
	15      1       31      22      6
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T6_regs[5];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T30_regs[5];
	__m256i T31_regs[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T6_regs[i] = XOR(T2_regs[i][1], T4_regs[i][1]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i] = XOR(T18_regs[i][0], state[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][0], T2_regs[i][0]);

		T30_regs[i] = XOR(T22_regs[i], T8_regs[i][0]);

		T31_regs[i] = XOR(T30_regs[i], state[i][0]);
	}

	//15      1       31      22      6
	for (int i = 0; i < 5; i++) {

		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T6_regs[i], 0), 0, 0, 0));

		state[i][0] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(T20_regs[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i], 1), _mm256_extract_epi64(state[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i], 2), _mm256_extract_epi64(T30_regs[i], 2), _mm256_extract_epi64(T8_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3), _mm256_extract_epi64(T19_regs[i], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0), _mm256_extract_epi64(T20_regs[i][1], 0), _mm256_extract_epi64(T15_regs[i][1], 0)));
	}
}

void mixcolumns_inv_80bit(__m256i (*state)[2]) {
	/*
	6       22      31      1       15
	15      19      8       1       20
	20      5       30      5       11
	11      3       19      8       18
	18      2       2       18      1
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T6_regs[5];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T30_regs[5];
	__m256i T31_regs[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T6_regs[i] = XOR(T2_regs[i][0], T4_regs[i][0]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T11_regs[i][0], T4_regs[i][0]);
		T15_regs[i][1] = XOR(T11_regs[i][1], T4_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i] = XOR(T16_regs[i][0], T3_regs[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T16_regs[i][0], T6_regs[i]);

		T30_regs[i] = XOR(T22_regs[i], T8_regs[i][0]);

		T31_regs[i] = XOR(T30_regs[i], state[i][0]);
	}

	for (int i = 0; i < 5; i++) {

		state[i][0] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T6_regs[i], 0), _mm256_extract_epi64(T15_regs[i][0], 0), _mm256_extract_epi64(T20_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i], 1), _mm256_extract_epi64(T19_regs[i], 1), _mm256_extract_epi64(T5_regs[i], 1), _mm256_extract_epi64(T3_regs[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i], 2), _mm256_extract_epi64(T8_regs[i][0], 2), _mm256_extract_epi64(T30_regs[i], 2), _mm256_extract_epi64(T19_regs[i], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 3), _mm256_extract_epi64(state[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3), _mm256_extract_epi64(T8_regs[i][0], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 0), _mm256_extract_epi64(T20_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0), _mm256_extract_epi64(T18_regs[i][1], 0)));

		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][1], 0), 0, 0, 0));
	}
}

//******Common******
void sbox(__m256i (*x)[2]) {
	//YMM 0 = LSB
	//YMM 4 = MSB

	for (int i = 0; i < 2; i++) {
		//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).

		//Helper variables
		__m256i t[11];
		__m256i y[4];


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
		x[4][i] = XOR7(x[1][i], x[2][i], x[3][i], t[4], t[2], t[6], t[8]);

		x[0][i] = y[0];
		x[1][i] = y[1];
		x[2][i] = y[2];
		x[3][i] = y[3];

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