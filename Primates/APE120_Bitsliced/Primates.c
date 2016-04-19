#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>

 

void u64_to_string(char *str, u64 number);
void transpose_nonce_to_u64(const unsigned char n[4][NonceLength], u64 transposedNonce[5][4][3]);
void transpose_data_to_u64_ratesize(const unsigned char *data[4], u64 dataLen[4], u64 transposedData[5][4], u64 dataTransposedProgress[4], u64 sectionLengthWithoutPadding[4]);

void print_keys(const unsigned char k[4][keyLength]);
void print_keys_hex(const unsigned char k[4][keyLength]);
void print_nonces(const unsigned char npub[4][NonceLength]);
void print_nonces_hex(const unsigned char npub[4][NonceLength]);
void print_ad(const unsigned char *ad[4], u64 adlen[4]);
void print_ad_hex(const unsigned char *ad[4], u64 adlen[4]);
void print_YMMs(__m256i *YMMs);
void primate(__m256i state[5]);

void transpose_key_to_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]);

void primates120_encrypt(const unsigned char k[4][keyLength],
	const unsigned char *m[4], u64 mlen[4],
	const unsigned char *ad[4], u64 adlen[4],
	const unsigned char npub[4][NonceLength],
	unsigned *ciphertexts[4],
	unsigned char tag[4][keyLength]) {

	//Declarations
	__m256i YMM[5]; //YMM state registers
	u64 transposedKey[5][4]; //5x registers with 4x states in them
	u64 transposedData[5][4]; //5x registers with 4x states in them
	u64 transposedNonce[5][4][3]; //5x regs, 4x states, 3x nonce sections for each.

	//Prepare
	_mm256_zeroall;
	memset(transposedData, 0, sizeof(transposedData));
	memset(transposedNonce, 0, sizeof(transposedNonce));
	memset(transposedKey, 0, sizeof(transposedKey));

	//Implementation note: Each YMM is 256 bits. Each primate state takes up 56 bits in each YMM.
	//Thus there is room for a total of 4 primate states. To make the implementation more efficient, we align
	//each state with 64 bits. There will still only be space in the vector for a total of 4 states anyway.

	//TEST 
	print_keys_hex(k);
	print_nonces_hex(npub);
	print_ad_hex(ad, adlen);
	//ENDTEST 

	//Transpose keys to bitsliced format.
	transpose_key_to_u64(k, transposedKey);

	//Load transposed keys into registers.
	for (int YMM_no = 0; YMM_no < 5; YMM_no++) {
		YMM[YMM_no] = _mm256_set_epi64x(transposedKey[YMM_no][0],
			transposedKey[YMM_no][1],
			transposedKey[YMM_no][2],
			transposedKey[YMM_no][3]); //Maby use 
	}

	//Transpose nonces to bitsliced format
	transpose_nonce_to_u64(npub, transposedNonce);

	//XOR the nonce in rate-size chuncks to the rate-part of the state and do primate permutation.
	//The rate-size is 40 bits and the nonce is 120 bits, so 3 times will we do it.
	//each state is kept in 5 ymms, and there are 4 states, so 20 XORs per round, and 60 in total.
	//Note: The nonce is 120 bits long, but stored in 4x u64's (=256 bits). This is okay as the rest of the u64's are zeroed. 
	for (int nonceSection = 0; nonceSection < 3; nonceSection++) {

		//YMM0, [ymm][state][nonce-sect]
		__m256i nonce = _mm256_set_epi64x(transposedNonce[0][0][nonceSection], transposedNonce[0][0][nonceSection],
			transposedNonce[0][0][nonceSection], transposedNonce[0][0][nonceSection]);
		YMM[0] = _mm256_xor_si256(nonce, YMM[0]);

		//YMM1, [ymm][state][nonce-sect]
		nonce = _mm256_set_epi64x(transposedNonce[1][0][nonceSection], transposedNonce[1][0][nonceSection],
			transposedNonce[1][0][nonceSection], transposedNonce[1][0][nonceSection]);
		YMM[1] = _mm256_xor_si256(nonce, YMM[1]);

		//YMM2, [ymm][state][nonce-sect]
		nonce = _mm256_set_epi64x(transposedNonce[2][0][nonceSection], transposedNonce[2][0][nonceSection],
			transposedNonce[2][0][nonceSection], transposedNonce[2][0][nonceSection]);
		YMM[2] = _mm256_xor_si256(nonce, YMM[2]);

		//YMM3, [ymm][state][nonce-sect]
		nonce = _mm256_set_epi64x(transposedNonce[3][0][nonceSection], transposedNonce[3][0][nonceSection],
			transposedNonce[3][0][nonceSection], transposedNonce[3][0][nonceSection]);
		YMM[3] = _mm256_xor_si256(nonce, YMM[3]);

		//YMM4, [ymm][state][nonce-sect]
		nonce = _mm256_set_epi64x(transposedNonce[4][0][nonceSection], transposedNonce[4][0][nonceSection],
			transposedNonce[4][0][nonceSection], transposedNonce[4][0][nonceSection]);
		YMM[4] = _mm256_xor_si256(nonce, YMM[4]);

		primate120_encrypt_permutate_state(YMM);
	}

	//Handle associated data. If any is present, we do this - else we skip the step.
	if (adlen != 0) {
		u64 transposedDataProgress = 0;
		__m256i adYMM;
		while (transposedDataProgress < adlen) {
			transpose_data_to_u64_ratesize(ad, adlen, transposedData, transposedDataProgress, 0);

			//XOR ad to state
			for (int YMMReg = 0; YMMReg < 5; YMMReg++) {
				adYMM = _mm256_set_epi64x(transposedData[YMMReg][0], transposedData[YMMReg][1],
					transposedData[YMMReg][2], transposedData[YMMReg][3]);
				YMM[YMMReg] = _mm256_xor_si256(YMM[YMMReg], adYMM);
			}
			primate(YMM);
		}

		memset(transposedData, 0, sizeof(transposedData));
	}


	//XOR state with [0]^(b-1) | 1 //i.e. xor last bit with 1.
	YMM[4] = _mm256_xor_si256(YMM[4], _mm256_set1_epi64x(0, 0, 0, 1));

	//Var to store length of last block.
	u64 lastBlockLengths[4];

	//Handle message
	if (mlen != 0) {
		u64 transposedDataProgress = 0;
		__m256i mYMM;

		while (transposedDataProgress < mlen) {
			transpose_data_to_u64_ratesize(m, mlen, transposedData, transposedDataProgress, lastBlockLengths);

			//XOR m to state
			for (int YMMReg = 0; YMMReg < 5; YMMReg++) {
				mYMM = _mm256_set_epi64x(transposedData[YMMReg][0], transposedData[YMMReg][1],
										 transposedData[YMMReg][2], transposedData[YMMReg][3]);
				YMM[YMMReg] = _mm256_xor_si256(YMM[YMMReg], mYMM);
			}

			//Primate permutation
			primate(YMM);

			//detranspose rate part (5 bytes) of current state and store in corresponding place in C. 
			unsigned char detransposedRatePart[5]; 
			convert_rateparts_to_bytes(YMM, detransposedRatePart[5]);
			memcpy(ciphertexts[0][transposedDataProgress - 5], detransposedRatePart[0], 5);
			memcpy(ciphertexts[1][transposedDataProgress - 5], detransposedRatePart[1], 5);
			memcpy(ciphertexts[2][transposedDataProgress - 5], detransposedRatePart[2], 5);
			memcpy(ciphertexts[3][transposedDataProgress - 5], detransposedRatePart[3], 5);
		}

		//encryption done. Last state's capacity is our tag, 
		//and we need to do something weird about the secondlast block TODO (...whatever it is)
		unsigned char tags[4][keyLength];
		convert_capacityparts_to_bytes(YMM, tags);
		memcpy(tag[0], tags[0], keyLength);
		memcpy(tag[1], tags[1], keyLength);
		memcpy(tag[2], tags[2], keyLength);
		memcpy(tag[3], tags[3], keyLength);
	}


}

void convert_capacityparts_to_bytes(__m256i YMMs[5], unsigned char deTransposedBytes[4][keyLength]) {
	//In each YMM, the capacities is stored at the bits, 0-47 (bytes index 0-5), 64-111 (bytes index 8-13), 
	//128-175 (16-21) , 192-239 (24-29).
	//The MSB of each element is stored in YMM5.
	//The first element of the capacity is stored in the last bit of the YMMs (right?)	
	unsigned char *YMM0 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM1 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM2 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM3 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM4 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned

	_mm256_store_si256(YMM0, YMMs[0]);
	_mm256_store_si256(YMM1, YMMs[1]);
	_mm256_store_si256(YMM2, YMMs[2]);
	_mm256_store_si256(YMM3, YMMs[3]);
	_mm256_store_si256(YMM4, YMMs[4]);

	for (int state = 0; state < 4; state++) {
		int YMM_byte_offset = state * 8;

		//detranspose all 30 capacity bytes of a state
		for (int detransposed_byte = 0; detransposed_byte < 30; detransposed_byte += 5) {
			
			deTransposedBytes[state][0 + detransposed_byte] =
				(YMM0[YMM_byte_offset] & 1) |
				(YMM1[YMM_byte_offset] & 1) << 1 |
				(YMM2[YMM_byte_offset] & 1) << 2 |
				(YMM3[YMM_byte_offset] & 1) << 3 |
				(YMM4[YMM_byte_offset] & 1) << 4 |
				(YMM0[YMM_byte_offset] & 2) << 5 |
				(YMM1[YMM_byte_offset] & 2) << 6 |
				(YMM2[YMM_byte_offset] & 2) << 7;

			deTransposedBytes[state][1 + detransposed_byte] =
				(YMM3[YMM_byte_offset] & 2) |
				(YMM4[YMM_byte_offset] & 2) << 1 |
				(YMM0[YMM_byte_offset] & 3) << 2 |
				(YMM1[YMM_byte_offset] & 3) << 3 |
				(YMM2[YMM_byte_offset] & 3) << 4 |
				(YMM3[YMM_byte_offset] & 3) << 5 |
				(YMM4[YMM_byte_offset] & 3) << 6 |
				(YMM0[YMM_byte_offset] & 4) << 7;

			deTransposedBytes[state][2 + detransposed_byte] =
				(YMM1[YMM_byte_offset] & 4) |
				(YMM2[YMM_byte_offset] & 4) << 1 |
				(YMM3[YMM_byte_offset] & 4) << 2 |
				(YMM4[YMM_byte_offset] & 4) << 3 |
				(YMM0[YMM_byte_offset] & 5) << 4 |
				(YMM1[YMM_byte_offset] & 5) << 5 |
				(YMM2[YMM_byte_offset] & 5) << 6 |
				(YMM3[YMM_byte_offset] & 5) << 7;

			deTransposedBytes[state][3 + detransposed_byte] =
				(YMM4[YMM_byte_offset] & 5) |
				(YMM0[YMM_byte_offset] & 6) << 1 |
				(YMM1[YMM_byte_offset] & 6) << 2 |
				(YMM2[YMM_byte_offset] & 6) << 3 |
				(YMM3[YMM_byte_offset] & 6) << 4 |
				(YMM4[YMM_byte_offset] & 6) << 5 |
				(YMM0[YMM_byte_offset] & 7) << 6 |
				(YMM1[YMM_byte_offset] & 7) << 7;

			deTransposedBytes[state][4 + detransposed_byte] =
				(YMM2[YMM_byte_offset] & 7) |
				(YMM3[YMM_byte_offset] & 7) << 1 |
				(YMM4[YMM_byte_offset] & 7) << 2 |
				(YMM0[YMM_byte_offset] & 8) << 3 |
				(YMM1[YMM_byte_offset] & 8) << 4 |
				(YMM2[YMM_byte_offset] & 8) << 5 |
				(YMM3[YMM_byte_offset] & 8) << 6 |
				(YMM4[YMM_byte_offset] & 8) << 7;

			YMM_byte_offset++;
		}
	}
	_aligned_free(YMM0);
	_aligned_free(YMM1);
	_aligned_free(YMM2);
	_aligned_free(YMM3);
	_aligned_free(YMM4);
}

void convert_rateparts_to_bytes(__m256i YMMs[5], unsigned char deTransposedBytes[4][RateSize]) {
	
	//In each YMM, the rates are stored at the bits, 48-55 (byte index 6), 112-119 (byte index 14), 
	//176-183 (byte index 22), 240-247 (byte index 30).
	
	//The MSB of each element is stored in YMM5.
	//The first element of the capacity is stored in the last bit of the YMMs (right?)
	unsigned char *YMM0 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 30byte, 32byte aligned
	unsigned char *YMM1 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 30byte, 32byte aligned
	unsigned char *YMM2 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 30byte, 32byte aligned
	unsigned char *YMM3 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 30byte, 32byte aligned
	unsigned char *YMM4 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 30byte, 32byte aligned

	_mm256_store_si256(YMM0, YMMs[0]);
	_mm256_store_si256(YMM1, YMMs[1]);
	_mm256_store_si256(YMM2, YMMs[2]);
	_mm256_store_si256(YMM3, YMMs[3]);
	_mm256_store_si256(YMM4, YMMs[4]);

	for (int state = 0; state < 4; state++) {
		int byte_offset = 6 + state * 64;

		deTransposedBytes[state][0] =
			(YMM0[byte_offset] & 1) |
			(YMM1[byte_offset] & 1) << 1 |
			(YMM2[byte_offset] & 1) << 2 |
			(YMM3[byte_offset] & 1) << 3 |
			(YMM4[byte_offset] & 1) << 4 |
			(YMM0[byte_offset] & 2) << 5 |
			(YMM1[byte_offset] & 2) << 6 |
			(YMM2[byte_offset] & 2) << 7;
		
		deTransposedBytes[state][1] =
			(YMM3[byte_offset] & 2) |
			(YMM4[byte_offset] & 2) << 1 |
			(YMM0[byte_offset] & 3) << 2 |
			(YMM1[byte_offset] & 3) << 3 |
			(YMM2[byte_offset] & 3) << 4 |
			(YMM3[byte_offset] & 3) << 5 |
			(YMM4[byte_offset] & 3) << 6 |
			(YMM0[byte_offset] & 4) << 7;

		deTransposedBytes[state][2] =
			(YMM1[byte_offset] & 4) |
			(YMM2[byte_offset] & 4) << 1 |
			(YMM3[byte_offset] & 4) << 2 |
			(YMM4[byte_offset] & 4) << 3 |
			(YMM0[byte_offset] & 5) << 4 |
			(YMM1[byte_offset] & 5) << 5 |
			(YMM2[byte_offset] & 5) << 6 |
			(YMM3[byte_offset] & 5) << 7;

		deTransposedBytes[state][3] =
			(YMM4[byte_offset] & 5) |
			(YMM0[byte_offset] & 6) << 1 |
			(YMM1[byte_offset] & 6) << 2 |
			(YMM2[byte_offset] & 6) << 3 |
			(YMM3[byte_offset] & 6) << 4 |
			(YMM4[byte_offset] & 6) << 5 |
			(YMM0[byte_offset] & 7) << 6 |
			(YMM1[byte_offset] & 7) << 7;

		deTransposedBytes[state][4] =
			(YMM2[byte_offset] & 7) |
			(YMM3[byte_offset] & 7) << 1 |
			(YMM4[byte_offset] & 7) << 2 |
			(YMM0[byte_offset] & 8) << 3 |
			(YMM1[byte_offset] & 8) << 4 |
			(YMM2[byte_offset] & 8) << 5 |
			(YMM3[byte_offset] & 8) << 6 |
			(YMM4[byte_offset] & 8) << 7;
	}

	_aligned_free(YMM0);
	_aligned_free(YMM1);
	_aligned_free(YMM2);
	_aligned_free(YMM3);
	_aligned_free(YMM4);

}

void primate(__m256i states[5]) {
	for (int round = 0; round < PrimateRounds; round++) {
		//Constant addition
		//XOR a roundconstant to the second element of the second row (47th element of the capacity <-- probably wrong qqq)
		//Round constants for p1: 01, 02, 05, 0a, 15, 0b, 17, 0e, 1d, 1b, 16, 0c
		//Array where the constants are transposed to the 47th bit:
		const u64 roundConstantBitsYMM0[] = { 70368744177664, 0, 70368744177664, 0, 70368744177664, 
											70368744177664, 70368744177664, 0, 70368744177664, 
											70368744177664, 0, 0};
		const u64 roundConstantBitsYMM1[] = { 0, 70368744177664, 0, 70368744177664,
											0, 70368744177664, 70368744177664, 70368744177664,
											0, 70368744177664, 70368744177664, 0 };
		const u64 roundConstantBitsYMM2[] = { 0, 0, 70368744177664, 0,
											70368744177664, 0, 70368744177664, 70368744177664,
											70368744177664, 0, 70368744177664, 70368744177664 };
		const u64 roundConstantBitsYMM3[] = { 0, 0, 0, 70368744177664,
											0, 70368744177664, 0, 70368744177664,
											70368744177664, 70368744177664, 0, 70368744177664 };
		const u64 roundConstantBitsYMM4[] = { 0, 0, 0, 0,
											70368744177664, 0, 70368744177664, 0,
											70368744177664, 70368744177664, 70368744177664, 0 };
		
		__m256i rc_ymm0 = _mm256_set1_epi64x(roundConstantBitsYMM0[round]);
		__m256i rc_ymm1 = _mm256_set1_epi64x(roundConstantBitsYMM1[round]);
		__m256i rc_ymm2 = _mm256_set1_epi64x(roundConstantBitsYMM2[round]);
		__m256i rc_ymm3 = _mm256_set1_epi64x(roundConstantBitsYMM3[round]);
		__m256i rc_ymm4 = _mm256_set1_epi64x(roundConstantBitsYMM4[round]);
		states[0] = _mm256_xor_si256(states[0], rc_ymm0);
		states[1] = _mm256_xor_si256(states[1], rc_ymm1);
		states[2] = _mm256_xor_si256(states[2], rc_ymm2);
		states[3] = _mm256_xor_si256(states[3], rc_ymm3);
		states[4] = _mm256_xor_si256(states[4], rc_ymm4);

		//Mix Columns
		//TODO

		//Shift Rows primate 120
		//shifted from top down:
		//0,1,2,3,4,5,7
		//TODO

		//Sub Elements
		//TODO
	}
}

/*
We need to transpose chunks of the data of sizes that are equal to the rate-part (40 bits) of the primate-cipher.
Thus we transpose 5 bytes of each AD at a time.
*/
void transpose_data_to_u64_ratesize(const unsigned char *data[4], u64 dataLen[4], u64 transposedData[5][4], u64 dataTransposedProgress[4], u64 sectionLengthWithoutPadding[4]) {
	
	unsigned char singleByte;

	//Transpose associated data for each state
	for (int stateNo = 0; stateNo < 4; stateNo++) {
		unsigned char dataArray[5];

		//Are there less than 5 bytes, but atleast one still not transposed? (i.e. we need padding)
		if (dataLen[stateNo] > dataTransposedProgress[stateNo] && dataLen[stateNo] < dataTransposedProgress[stateNo] + 5) {
			switch (dataLen[stateNo] - dataTransposedProgress[stateNo]) {
			case 1: //Pad after first byte
				dataArray[0] = data[4][dataTransposedProgress[stateNo]];
				dataArray[1] = 256; //1000 0000
				dataArray[2] = 0; dataArray[3] = 0; dataArray[4] = 0;
				if (sectionLengthWithoutPadding != 0) sectionLengthWithoutPadding[stateNo] = 1;
				break;
			case 2: //Pad after second
				dataArray[0] = data[4][dataTransposedProgress[stateNo]];
				dataArray[1] = data[4][dataTransposedProgress[stateNo] + 1];
				dataArray[2] = 256; dataArray[3] = 0; dataArray[4] = 0;
				if (sectionLengthWithoutPadding != 0) sectionLengthWithoutPadding[stateNo] = 2;
				break;
			case 3: //Pad after third
				dataArray[0] = data[4][dataTransposedProgress[stateNo]];
				dataArray[1] = data[4][dataTransposedProgress[stateNo] + 1];
				dataArray[2] = data[4][dataTransposedProgress[stateNo] + 2];
				dataArray[3] = 256; dataArray[4] = 0;
				if (sectionLengthWithoutPadding != 0) sectionLengthWithoutPadding[stateNo] = 3;
				break;
			case 4: //Pad after fourth
				dataArray[0] = data[4][dataTransposedProgress[stateNo]];
				dataArray[1] = data[4][dataTransposedProgress[stateNo] + 1];
				dataArray[2] = data[4][dataTransposedProgress[stateNo] + 2];
				dataArray[3] = data[4][dataTransposedProgress[stateNo] + 3];
				dataArray[4] = 256;
				if (sectionLengthWithoutPadding != 0) sectionLengthWithoutPadding[stateNo] = 4;
				break;
			default:
				printf("Something went wrong when padding");
			}
		}

		//Are there still bytes left to transpose for this state? Then transpose next five (padding included)
		if (dataLen[stateNo] > dataTransposedProgress[stateNo] + 4) {
			if (sectionLengthWithoutPadding != 0) sectionLengthWithoutPadding[stateNo] = 5;
			singleByte = dataArray[0];
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | (singleByte & 1); //1. bit
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 1) & 1);
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 2) & 1);
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 3) & 1);
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 4) & 1);
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 5) & 1);
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 6) & 1);
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 7) & 1);

			singleByte = dataArray[1];
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | (singleByte & 1); //9. bit
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 1) & 1);
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 2) & 1);
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 3) & 1);
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 4) & 1);
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 5) & 1);
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 6) & 1);
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 7) & 1);

			singleByte = dataArray[2];
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | (singleByte & 1); //17. bit
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 1) & 1);
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 2) & 1);
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 3) & 1);
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 4) & 1);
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 5) & 1);
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 6) & 1);
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 7) & 1);

			singleByte = dataArray[3];
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | (singleByte & 1); //25. bit
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 1) & 1);
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 2) & 1);
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 3) & 1);
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 4) & 1);
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 5) & 1);
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 6) & 1);
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 7) & 1);

			singleByte = dataArray[4];
			transposedData[1][stateNo] = (transposedData[2][stateNo] << 1) | (singleByte & 1); //33. bit
			transposedData[2][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 1) & 1);
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 2) & 1);
			transposedData[0][stateNo] = (transposedData[0][stateNo] << 1) | ((singleByte >> 3) & 1);
			transposedData[1][stateNo] = (transposedData[1][stateNo] << 1) | ((singleByte >> 4) & 1);
			transposedData[2][stateNo] = (transposedData[2][stateNo] << 1) | ((singleByte >> 5) & 1);
			transposedData[3][stateNo] = (transposedData[3][stateNo] << 1) | ((singleByte >> 6) & 1);
			transposedData[4][stateNo] = (transposedData[4][stateNo] << 1) | ((singleByte >> 7) & 1); //40. bit
		}
		dataTransposedProgress[stateNo] += 5;
	}
}

/*
* This functions accepts 4 nonces of minimum length 15 bytes (= 120 bits) and transposes the bits from these
* into a multidimensional array of size n[YMM_no][stateNo][keysection]. Each nonce-section is 40 bits.
*/
void transpose_nonce_to_u64(const unsigned char n[4][NonceLength], u64 transposedNonce[5][4][3]) {
	unsigned char singleByte;

	for (int stateNo = 0; stateNo < 4; stateNo++) {
		for (int nonceSect = 0; nonceSect < 3; nonceSect++) {
			
			singleByte = n[stateNo][0 + (nonceSect * 5)];
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | (singleByte & 1); //1. bit
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 1) & 1);
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 2) & 1);
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 3) & 1);
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 4) & 1);
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 5) & 1);
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 6) & 1);
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 7) & 1);

			singleByte = n[stateNo][1 + (nonceSect * 5)];
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | (singleByte & 1); //9. bit
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 1) & 1);
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 2) & 1);
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 3) & 1);
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 4) & 1);
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 5) & 1);
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 6) & 1);
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 7) & 1);

			singleByte = n[stateNo][2 + (nonceSect * 5)];
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | (singleByte & 1); //17. bit
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 1) & 1);
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 2) & 1);
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 3) & 1);
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 4) & 1);
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 5) & 1);
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 6) & 1);
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 7) & 1);
		
			singleByte = n[stateNo][3 + (nonceSect * 5)];
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | (singleByte & 1); //25. bit
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 1) & 1);
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 2) & 1);
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 3) & 1);
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 4) & 1);
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 5) & 1);
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 6) & 1);
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 7) & 1);

			singleByte = n[stateNo][4 + (nonceSect * 5)];
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | (singleByte & 1); //33 bit.
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 1) & 1);
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 2) & 1);
			transposedNonce[0][stateNo][nonceSect] = (transposedNonce[0][stateNo][nonceSect] << 1) | ((singleByte >> 3) & 1);
			transposedNonce[1][stateNo][nonceSect] = (transposedNonce[1][stateNo][nonceSect] << 1) | ((singleByte >> 4) & 1);
			transposedNonce[2][stateNo][nonceSect] = (transposedNonce[2][stateNo][nonceSect] << 1) | ((singleByte >> 5) & 1);
			transposedNonce[3][stateNo][nonceSect] = (transposedNonce[3][stateNo][nonceSect] << 1) | ((singleByte >> 6) & 1);
			transposedNonce[4][stateNo][nonceSect] = (transposedNonce[4][stateNo][nonceSect] << 1) | ((singleByte >> 7) & 1); //40. bit
		}
	}
}

/*
* This function takes 4 keys and transpose them into a register with the dimensions:
* k[register_no][key_no]
*/
void transpose_key_to_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]) {

	unsigned char singleByte;	
	int offset;
	unsigned char *k_single;

	for (int keyNo = 0; keyNo < 4; keyNo++) {
		k_single = k[keyNo];
		offset = keyNo;
		//We expect that the key is 240 bits = 30 bytes long.
		for (int index = 0; index < keyLength; index += 5) {

			singleByte = k_single[index];
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | (singleByte & 1); //first bit
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 1) & 1);
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 2) & 1);
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 3) & 1);
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 4) & 1);
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 5) & 1);
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 6) & 1);
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

			singleByte = k_single[index + 1];
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | (singleByte & 1); //first bit
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 1) & 1);
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 2) & 1);
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 3) & 1);
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 4) & 1);
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 5) & 1);
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 6) & 1);
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

			singleByte = k_single[index + 2];
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | (singleByte & 1); //first bit
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 1) & 1);
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 2) & 1);
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 3) & 1);
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 4) & 1);
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 5) & 1);
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 6) & 1);
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

			singleByte = k_single[index + 3];
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | (singleByte & 1); //first bit
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 1) & 1);
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 2) & 1);
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 3) & 1);
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 4) & 1);
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 5) & 1);
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 6) & 1);
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

			singleByte = k_single[index + 4];
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | (singleByte & 1); //first bit
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 1) & 1);
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 2) & 1);
			transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 3) & 1);
			transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 4) & 1);
			transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 5) & 1);
			transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 6) & 1);
			transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 7) & 1); //eight bit
		}
	}
}



/* EVERYTHING BELOW IS FOR TESTING AND NOT MEANT FOR THE ACTUAL IMPLEMENTATION*/

#define YMMCount 5
#define YMMLength 256

//void print_YMMs(__m256i YMM[5]) {}

void print_keys_hex(const unsigned char k[4][keyLength]) {
	
	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = k[strNo];
		printf("Key %i : ", strNo);
		
		for (int byte = 0; byte < keyLength; byte++) {
			printf("%02x ", s[byte]);
		}
		printf("\n");
	}
}
void print_keys(const unsigned char k[4][keyLength]) {
	printf("Key 0: %s \n", k[0]);
	printf("Key 1: %s \n", k[1]);
	printf("Key 2: %s \n", k[2]);
	printf("Key 3: %s \n", k[3]);
}

void print_nonces_hex(const unsigned char npub[4][NonceLength]) {

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = npub[strNo];
		printf("Nonce %i : ", strNo);

		for (int byte = 0; byte < NonceLength; byte++) {
			printf("%02x ", s[byte]);
		}
		printf("\n");
	}
}
void print_nonces(const unsigned char npub[4][NonceLength]) {
	printf("Nonce 0: %s \n", npub[0]);
	printf("Nonce 1: %s \n", npub[1]);
	printf("Nonce 2: %s \n", npub[2]);
	printf("Nonce 3: %s \n", npub[3]);
}

void print_ad_hex(const unsigned char *ad[4], u64 adlen[4]) {
	if (adlen == 0) {
		printf("No associated data with any current encryption \n");
		return;
	}

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = ad[strNo];
		printf("Ass. Data %i : ", strNo);

		if (adlen[strNo] != 0) {
			for (int byte = 0; byte < adlen[strNo]; byte++) {
				printf("%02x ", s[byte]);
			}
		}
		else {
			printf("No data.");
		}
		printf("\n");
	}
}
void print_ad(const unsigned char *ad[4], u64 adlen[4]) {
	if (adlen == 0) {
		printf("No associated data \n");
		return;
	}

	if (adlen[0] != 0)
		printf("Ass. Data 0: %s \n", ad[0]);
	if (adlen[1] != 0)
		printf("Ass. Data 1: %s \n", ad[1]);
	if (adlen[2] != 0)
		printf("Ass. Data 2: %s \n", ad[2]);
	if (adlen[3] != 0)
		printf("Ass. Data 3: %s \n", ad[3]);
}

void print_YMMs(__m256i *YMMs) {
	unsigned char *YMM0 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM1 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM2 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM3 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM4 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned


	_mm256_store_si256(YMM0, YMMs[0]);
	_mm256_store_si256(YMM1, YMMs[1]);
	_mm256_store_si256(YMM2, YMMs[2]);
	_mm256_store_si256(YMM3, YMMs[3]);
	_mm256_store_si256(YMM4, YMMs[4]);

	printf("YMM 0: %02X \n", YMM0);
}


void print_u64s_as_binary(u64 transposedData[5][4]) {
	char reg[YMMCount][YMMLength];

	for (int YMM_no = 0; YMM_no < YMMCount; YMM_no++) {
		char registerString[256];
		char tempString[64];

		u64_to_string(tempString, transposedData[YMM_no][0]);
		strcat(registerString, tempString);
		u64_to_string(tempString, transposedData[YMM_no][1]);
		strcat(registerString, tempString);
		u64_to_string(tempString, transposedData[YMM_no][2]);
		strcat(registerString, tempString);
		u64_to_string(tempString, transposedData[YMM_no][3]);
		strcat(registerString, tempString);
		
		printf("Reg %i: %s \n", YMM_no, registerString);
	}
}

//Allocated string-space must be atleast as long as the amount of bits in the u64 (= 64).
void u64_to_string(char *str, u64 number) {
	
	//Add next bit to string
	int next_bit = number & 1;
	if (next_bit == 0) strcat(str, '0');
	if (next_bit == 1) strcat(str, '1');
	
	//Are we at last bit of number?
	if (number == 0 || number == 1) {
		return;
	} 
	u64_to_string(str, number >> 1);
}

void primates120_decrypt() {
}