#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"


void transpose_nonce_to_rate_u64(const unsigned char n[4][NonceLength], u64 transposedNonce[5][4][3]);
void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 dataLen[4], u64 transposedData[5][4], u64 dataTransposedProgress[4], u64 sectionLengthWithoutPadding[4]);

void primate(__m256i *state);

void convert_capacityparts_to_bytes(__m256i *YMMs, unsigned char deTransposedBytes[4][keyLength]);
void convert_rateparts_to_bytes(__m256i *YMMs, unsigned char deTransposedBytes[4][RateSize]);

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

	//Transpose keys to bitsliced format and load them into registers.
	transpose_key_to_u64(k, transposedKey);
	YMM[0] = _mm256_set_epi64x(transposedKey[0][0], transposedKey[0][1], transposedKey[0][2], transposedKey[0][3]);
	YMM[1] = _mm256_set_epi64x(transposedKey[1][0], transposedKey[1][1], transposedKey[1][2], transposedKey[1][3]);
	YMM[2] = _mm256_set_epi64x(transposedKey[2][0], transposedKey[2][1], transposedKey[2][2], transposedKey[2][3]);
	YMM[3] = _mm256_set_epi64x(transposedKey[3][0], transposedKey[3][1], transposedKey[3][2], transposedKey[3][3]);
	YMM[4] = _mm256_set_epi64x(transposedKey[4][0], transposedKey[4][1], transposedKey[4][2], transposedKey[4][3]);

	//Transpose nonces to bitsliced format
	transpose_nonce_to_rate_u64(npub, transposedNonce);

	//XOR the nonce in rate-size chuncks to the rate-part of the state and do primate permutation.
	//The rate-size is 40 bits and the nonce is 120 bits, so 3 times will we do it.
	//each state is kept in 5 ymms, and there are 4 states, so 20 XORs per round, and 60 in total.
	//Note: The nonce is 120 bits long, but stored in 4x u64's (=256 bits). This is okay as the rest of the u64's are zeroed. 
	for (int nonceSection = 0; nonceSection < 3; nonceSection++) {
		YMM[0] = _mm256_xor_si256(YMM[0], _mm256_set_epi64x(transposedNonce[0][0][nonceSection], transposedNonce[0][1][nonceSection], transposedNonce[0][2][nonceSection], transposedNonce[0][3][nonceSection]));
		YMM[1] = _mm256_xor_si256(YMM[1], _mm256_set_epi64x(transposedNonce[1][0][nonceSection], transposedNonce[1][1][nonceSection], transposedNonce[1][2][nonceSection], transposedNonce[1][3][nonceSection]));
		YMM[2] = _mm256_xor_si256(YMM[2], _mm256_set_epi64x(transposedNonce[2][0][nonceSection], transposedNonce[2][1][nonceSection], transposedNonce[2][2][nonceSection], transposedNonce[2][3][nonceSection]));
		YMM[3] = _mm256_xor_si256(YMM[3], _mm256_set_epi64x(transposedNonce[3][0][nonceSection], transposedNonce[3][1][nonceSection], transposedNonce[3][2][nonceSection], transposedNonce[3][3][nonceSection]));
		YMM[4] = _mm256_xor_si256(YMM[4], _mm256_set_epi64x(transposedNonce[4][0][nonceSection], transposedNonce[4][1][nonceSection], transposedNonce[4][2][nonceSection], transposedNonce[4][3][nonceSection]));

	//	primate(YMM); QQQ
	}

	//TEST
	print_state_as_binary(YMM, 0);

	//Handle associated data. If any is present, we do this - else we skip the step.
	if (adlen != 0) {
		u64 transposedDataProgress[4] = { 0 };
		__m256i adYmm;
		while (transposedDataProgress[0] < adlen[0] && transposedDataProgress[1] < adlen[1] &&
			   transposedDataProgress[2] < adlen[2] && transposedDataProgress[3] < adlen[3]) {

			transpose_data_to_ratesize_u64(ad, adlen, transposedData, transposedDataProgress, 0);
			//XOR next add elements to rate-part of state
			for (int YMMReg = 0; YMMReg < 5; YMMReg++) {
				adYmm = _mm256_set_epi64x(transposedData[YMMReg][0], transposedData[YMMReg][1], transposedData[YMMReg][2], transposedData[YMMReg][3]);
				YMM[YMMReg] = _mm256_xor_si256(YMM[YMMReg], adYmm);
				print_state_as_binary(YMM, 0);
			}
			//primate(YMM);
		}

		memset(transposedData, 0, sizeof(transposedData)); //Empty transposedData variable, so that we can use it for the message-data next.
	}

	//TEST
	print_state_as_binary(YMM, 0);

	//XOR state with [0]^(b-1) | 1 //i.e. xor last bit with 1.
	YMM[4] = _mm256_xor_si256(YMM[4], _mm256_set1_epi64x(1));

	//Var to store length of last block.
	u64 lastBlockLengths[4];

	//Handle message
	if (mlen != 0) {
		u64 transposedDataProgress = 0;
		__m256i mYMM;

		while (transposedDataProgress < mlen) {
			transpose_data_to_ratesize_u64(m, mlen, transposedData, transposedDataProgress, lastBlockLengths);

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

void convert_capacityparts_to_bytes(__m256i *YMMs, unsigned char deTransposedBytes[4][keyLength]) {
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

void convert_rateparts_to_bytes(__m256i *YMMs, unsigned char deTransposedBytes[4][RateSize]) {
	
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

void primate(__m256i *states) {
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
Thus we transpose 8 primate elements (stored in 8 bytes) at a time.
*/
void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 data_len[4], u64 transposed_data[5][4], u64 transpose_progress[4], u64 sectionLengthWithoutPadding[4]) {

	for (int state_no = 0; state_no < 4; state_no++) {
		unsigned char data_i[8] = {0}; //internal data being used for transposing
		unsigned char *data_ptr;

		//How many elements are left?
		int remaining_elements = data_len[state_no] - transpose_progress[state_no];

		//If less than 8 are left, add padding.
		if (remaining_elements > 0 && remaining_elements < 8) {

			for (int i = 0; i < remaining_elements; i++) {
				data_i[i] = data[state_no][transpose_progress[state_no + i]];

				if (i + 1 == remaining_elements) {
					data_i[i + 1] = 1; //first element after end of data is a 1, the rest of the padding is 0.
				}
			}
			data_ptr = data_i;
		}
		else {
			data_ptr = data[state_no];
		}

		//transpose if there are more bytes left
		if (remaining_elements > 0) {
			for (int element = 0; element < 8; element++) {
				transposed_data[0][state_no] = (transposed_data[0][state_no] << 1) |  data_ptr[element] & 1;
				transposed_data[1][state_no] = (transposed_data[0][state_no] << 1) | (data_ptr[element] >> 1) & 1;
				transposed_data[2][state_no] = (transposed_data[0][state_no] << 1) | (data_ptr[element] >> 2) & 1;
				transposed_data[3][state_no] = (transposed_data[0][state_no] << 1) | (data_ptr[element] >> 3) & 1;
				transposed_data[4][state_no] = (transposed_data[0][state_no] << 1) | (data_ptr[element] >> 4) & 1;
			}
			//The ratepart is located at the bits 48-55, while these nonce-bits would be placed at bits 0-7. Thus we shift.
			transposed_data[0][state_no] = transposed_data[0][state_no] << 48;
			transposed_data[1][state_no] = transposed_data[1][state_no] << 48;
			transposed_data[2][state_no] = transposed_data[2][state_no] << 48;
			transposed_data[3][state_no] = transposed_data[3][state_no] << 48;
			transposed_data[4][state_no] = transposed_data[4][state_no] << 48;
		}

		//Always increment with 8, also if we did not pad.. we just need it to know,  whether we reached the end.
		transpose_progress[state_no] += 8;
	}
}

/*
* This functions accepts 4 nonces of length 120 bits ( = 24 primate elements) and transposes these bits 
* into an array of size n[YMM_no][stateNo][nonce-section]. Each nonce-section is 40 bits and 8 elements long.
*/
void transpose_nonce_to_rate_u64(const unsigned char n[4][NonceLength], u64 transposed_nonce[5][4][3]) {

	for (int state_no = 0; state_no < 4; state_no++) {
		for (int nonce_sec = 0; nonce_sec < 3; nonce_sec++) {

			int nonce_offset = nonce_sec * 8;
			for (int nonce_element = 0; nonce_element < 8; nonce_element++) {
				int nonce_index = nonce_offset + nonce_element;
				transposed_nonce[0][state_no][nonce_sec] = (transposed_nonce[0][state_no][nonce_sec] << 1) | n[state_no][nonce_index] & 1;
				transposed_nonce[1][state_no][nonce_sec] = (transposed_nonce[1][state_no][nonce_sec] << 1) | (n[state_no][nonce_index] >> 1) & 1;
				transposed_nonce[2][state_no][nonce_sec] = (transposed_nonce[2][state_no][nonce_sec] << 1) | (n[state_no][nonce_index] >> 2) & 1;
				transposed_nonce[3][state_no][nonce_sec] = (transposed_nonce[3][state_no][nonce_sec] << 1) | (n[state_no][nonce_index] >> 3) & 1;
				transposed_nonce[4][state_no][nonce_sec] = (transposed_nonce[4][state_no][nonce_sec] << 1) | (n[state_no][nonce_index] >> 4) & 1;
			}

			//The ratepart is located at the bits 48-55, while these nonce-bits would be placed at bits 0-7. Thus we shift.
			transposed_nonce[0][state_no][nonce_sec] = transposed_nonce[0][state_no][nonce_sec] << 48;
			transposed_nonce[1][state_no][nonce_sec] = transposed_nonce[1][state_no][nonce_sec]	<< 48;
			transposed_nonce[2][state_no][nonce_sec] = transposed_nonce[2][state_no][nonce_sec]	<< 48;
			transposed_nonce[3][state_no][nonce_sec] = transposed_nonce[3][state_no][nonce_sec]	<< 48;
			transposed_nonce[4][state_no][nonce_sec] = transposed_nonce[4][state_no][nonce_sec]	<< 48;
		}
	}
}

/*
* This function takes 4 keys and transpose them into a register with the dimensions:
* k[register_no][key_no]
*/
void transpose_key_to_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]) {

	//We expect that the key is 240 bits = 48 primate elements (stored in bytes) long.
	for (int key_no = 0; key_no < 4; key_no++) {
		for (int key_element = 0; key_element < keyLength; key_element++) {
			transposedKey[0][key_no] = (transposedKey[0][key_no] << 1) | k[key_no][key_element] & 1;
			transposedKey[1][key_no] = (transposedKey[1][key_no] << 1) | (k[key_no][key_element] >> 1) & 1;
			transposedKey[2][key_no] = (transposedKey[2][key_no] << 1) | (k[key_no][key_element] >> 2) & 1;
			transposedKey[3][key_no] = (transposedKey[3][key_no] << 1) | (k[key_no][key_element] >> 3) & 1;
			transposedKey[4][key_no] = (transposedKey[4][key_no] << 1) | (k[key_no][key_element] >> 4) & 1;
		}
	}
}

void primates120_decrypt() {
}