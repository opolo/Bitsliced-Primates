#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"

#define false 0
#define true 1
#define bool unsigned char

void transpose_nonce_to_rate_u64(const unsigned char n[4][NonceLength], u64 transposedNonce[5][4][3]);
void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 dataLen[4], u64 transposedData[5][4], u64 dataTransposedProgress[4], u64 sectionLengthWithoutPadding[4]);

void primate(__m256i *state);

void detranspose_capacity_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][CapacitySize]);
void detranspose_rate_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][RateSize]);

void transpose_key_to_capacity_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]);

void primates120_encrypt(const unsigned char k[4][keyLength],
	const unsigned char *m[4], u64 mlen[4],
	const unsigned char *ad[4], u64 adlen[4],
	const unsigned char npub[4][NonceLength],
	unsigned char *ciphertexts[4],
	unsigned char tag[4][keyLength]) {

	//Declarations
	__m256i YMM[5]; //YMM state registers
	__m256i keys_YMM[5]; //YMM state registers
	u64 transposedKey[5][4]; //5x registers with 4x states in them
	u64 transposedData[5][4]; //5x registers with 4x states in them
	u64 transposedNonce[5][4][3]; //5x regs, 4x states, 3x nonce sections for each.
	bool DoneTransposingState[4];

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

	//Transpose keys to bitsliced format and load them into YMM registers. Also make a copy of key for later
	transpose_key_to_capacity_u64(k, transposedKey);
	YMM[0] = _mm256_set_epi64x(transposedKey[0][0], transposedKey[0][1], transposedKey[0][2], transposedKey[0][3]);
	YMM[1] = _mm256_set_epi64x(transposedKey[1][0], transposedKey[1][1], transposedKey[1][2], transposedKey[1][3]);
	YMM[2] = _mm256_set_epi64x(transposedKey[2][0], transposedKey[2][1], transposedKey[2][2], transposedKey[2][3]);
	YMM[3] = _mm256_set_epi64x(transposedKey[3][0], transposedKey[3][1], transposedKey[3][2], transposedKey[3][3]);
	YMM[4] = _mm256_set_epi64x(transposedKey[4][0], transposedKey[4][1], transposedKey[4][2], transposedKey[4][3]);
	for (int i = 0; i < 5; i++) {
		keys_YMM[i] = YMM[0]; //This should not create a reference, but actually copy data as compiler should generate a vmovdqa instruction.
	}

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

		primate(YMM);
	}

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
			}
			memset(transposedData, 0, sizeof(transposedData));
			primate(YMM); 
		}
	}

	//XOR state with [0]^(b-1) | 1 //i.e. xor last bit with 1.
	__m256i temp = _mm256_set1_epi64x(1);
	YMM[4] = _mm256_xor_si256(YMM[4],temp);

	//test qqq
	print_state_as_binary(YMM, 0);
	
	//Var to store length of last block.
	u64 lastBlockLengths[4];

	//Handle message. If any is present, we do this - else we skip the step.
	if (mlen != 0) {
		u64 transposedDataProgress[4] = { 0 };
		__m256i msgYmm;

		while (transposedDataProgress[0] < mlen[0] && transposedDataProgress[1] < mlen[1] &&
			   transposedDataProgress[2] < mlen[2] && transposedDataProgress[3] < mlen[3]) {

			transpose_data_to_ratesize_u64(m, mlen, transposedData, transposedDataProgress, 0);
			//XOR next elements to rate-part of state
			for (int YMMReg = 0; YMMReg < 5; YMMReg++) {
				msgYmm = _mm256_set_epi64x(transposedData[YMMReg][0], transposedData[YMMReg][1], transposedData[YMMReg][2], transposedData[YMMReg][3]);
				YMM[YMMReg] = _mm256_xor_si256(YMM[YMMReg], msgYmm);
			}
			memset(transposedData, 0, sizeof(transposedData));
			primate(YMM);
			
			//Save the current rate r_i to c_i
			unsigned char temp_rate[4][RateSize] = { 0 };
			detranspose_rate_to_bytes(YMM, temp_rate);
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 8; j++) {
					ciphertexts[i][transposedDataProgress[i] - 8 + j] = temp_rate[i][j];
				}
			}			
		}
	}

	//TODO: Do something weird about the last ciphertext block 
	
	//XOR final capacity with key
	for (int YMM_reg = 0; YMM_reg < 5; YMM_reg++) {
		YMM[0] = _mm256_xor_si256(YMM[YMM_reg], keys_YMM[0]);
	}

	//detranspose key and return it as tag.
	detranspose_capacity_to_bytes(YMM, tag);
}

void detranspose_capacity_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][CapacitySize]) {
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

		int state_offset = state * 8; //8 bytes = 64 bit
		for (int capacity_byte = 0; capacity_byte < CapacitySize/8; capacity_byte++) {
			
			int offset = capacity_byte * 8;
			int shift = 0;
			for (int bit = 0; bit < 8; bit++) {
				
				detransposedBytes[state][bit + offset] = ((YMM0[capacity_byte + state_offset] >> shift) & 1) |
										   				 ((YMM1[capacity_byte + state_offset] >> shift) & 1 << 1) |
										   				 ((YMM2[capacity_byte + state_offset] >> shift) & 1 << 2) |
										   				 ((YMM3[capacity_byte + state_offset] >> shift) & 1 << 3) |
										   				 ((YMM4[capacity_byte + state_offset] >> shift) & 1 << 4);
				shift++;
			}
		}
	}
	_aligned_free(YMM0);
	_aligned_free(YMM1);
	_aligned_free(YMM2);
	_aligned_free(YMM3);
	_aligned_free(YMM4);
}

void detranspose_rate_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][RateSize]) {
	
	//In each YMM, the rates are stored at the bits, 48-55 (byte index 6), 112-119 (byte index 14), 
	//176-183 (byte index 22), 240-247 (byte index 30).

	unsigned char *YMM0 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM1 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM2 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM3 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM4 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned

	for (int state = 0; state < 4; state++) {

		int offset = state * 8; //8 bytes = 64 bit
		int shift = 0;
		for (int bit = 0; bit < 8; bit++) {

			detransposedBytes[state][bit] = ((YMM0[6 + offset] >> shift) & 1) |
											((YMM1[6 + offset] >> shift) & 1 << 1) |
											((YMM2[6 + offset] >> shift) & 1 << 2) |
											((YMM3[6 + offset] >> shift) & 1 << 3) |
											((YMM4[6 + offset] >> shift) & 1 << 4);
			shift++;
		}
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
				data_i[i] = data[state_no][transpose_progress[state_no] + i];

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
				transposed_data[0][state_no] = (transposed_data[0][state_no] << 1) |  data_ptr[transpose_progress[state_no] + element] & 1;
				transposed_data[1][state_no] = (transposed_data[1][state_no] << 1) | (data_ptr[transpose_progress[state_no] + element] >> 1) & 1;
				transposed_data[2][state_no] = (transposed_data[2][state_no] << 1) | (data_ptr[transpose_progress[state_no] + element] >> 2) & 1;
				transposed_data[3][state_no] = (transposed_data[3][state_no] << 1) | (data_ptr[transpose_progress[state_no] + element] >> 3) & 1;
				transposed_data[4][state_no] = (transposed_data[4][state_no] << 1) | (data_ptr[transpose_progress[state_no] + element] >> 4) & 1;
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
void transpose_key_to_capacity_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]) {

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

void primates120_decrypt(const unsigned char k[4][keyLength],
	const unsigned char npub[4][NonceLength], 
	const unsigned char *ciphertexts[4], u64 cLen[4],
	const unsigned char *ad[4], u64 adlen[4],
	unsigned char tag[4][keyLength],
	unsigned char *m[4]) 
{
	//Declarations
	__m256i keys_YMM[5]; //YMM state registers
	__m256i YMM_IV[5];
	__m256i YMM_V[5];
	__m256i YMM_M[5];
	__m256i ratemask = _mm256_set1_epi64x(71776119061217280);
	__m256i capacitymask = _mm256_set1_epi64x(281474976710655);
	u64 transposedData[5][4]; //5x registers with 4x states in them
	u64 transposedNonce[5][4][3]; //5x regs, 4x states, 3x nonce sections for each.
	u64 transposedKey[5][4]; //5x registers with 4x states in them
	u64 transposedTag[5][4]; //5x registers with 4x states in them

	//Transpose keys to bitsliced format and load them into YMM registers. Also make a copy of key for later
	transpose_key_to_capacity_u64(k, transposedKey);
	YMM_IV[0] = _mm256_set_epi64x(transposedKey[0][0], transposedKey[0][1], transposedKey[0][2], transposedKey[0][3]);
	YMM_IV[1] = _mm256_set_epi64x(transposedKey[1][0], transposedKey[1][1], transposedKey[1][2], transposedKey[1][3]);
	YMM_IV[2] = _mm256_set_epi64x(transposedKey[2][0], transposedKey[2][1], transposedKey[2][2], transposedKey[2][3]);
	YMM_IV[3] = _mm256_set_epi64x(transposedKey[3][0], transposedKey[3][1], transposedKey[3][2], transposedKey[3][3]);
	YMM_IV[4] = _mm256_set_epi64x(transposedKey[4][0], transposedKey[4][1], transposedKey[4][2], transposedKey[4][3]);
	for (int i = 0; i < 5; i++) {
		keys_YMM[i] = YMM_IV[i]; //This should not create a reference, but actually copy data as compiler should generate a vmovdqa instruction.
	}

	//XOR the nonce in rate-size chuncks to the rate-part of the state and do primate permutation.
	//The rate-size is 40 bits and the nonce is 120 bits, so 3 times will we do it.
	//each state is kept in 5 ymms, and there are 4 states, so 20 XORs per round, and 60 in total.
	//Note: The nonce is 120 bits long, but stored in 4x u64's (=256 bits). This is okay as the rest of the u64's are zeroed. 
	for (int nonceSection = 0; nonceSection < 3; nonceSection++) {
		YMM_IV[0] = _mm256_xor_si256(YMM_IV[0], _mm256_set_epi64x(transposedNonce[0][0][nonceSection], transposedNonce[0][1][nonceSection], transposedNonce[0][2][nonceSection], transposedNonce[0][3][nonceSection]));
		YMM_IV[1] = _mm256_xor_si256(YMM_IV[1], _mm256_set_epi64x(transposedNonce[1][0][nonceSection], transposedNonce[1][1][nonceSection], transposedNonce[1][2][nonceSection], transposedNonce[1][3][nonceSection]));
		YMM_IV[2] = _mm256_xor_si256(YMM_IV[2], _mm256_set_epi64x(transposedNonce[2][0][nonceSection], transposedNonce[2][1][nonceSection], transposedNonce[2][2][nonceSection], transposedNonce[2][3][nonceSection]));
		YMM_IV[3] = _mm256_xor_si256(YMM_IV[3], _mm256_set_epi64x(transposedNonce[3][0][nonceSection], transposedNonce[3][1][nonceSection], transposedNonce[3][2][nonceSection], transposedNonce[3][3][nonceSection]));
		YMM_IV[4] = _mm256_xor_si256(YMM_IV[4], _mm256_set_epi64x(transposedNonce[4][0][nonceSection], transposedNonce[4][1][nonceSection], transposedNonce[4][2][nonceSection], transposedNonce[4][3][nonceSection]));

		primate(YMM_IV);
	}

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
				YMM_IV[YMMReg] = _mm256_xor_si256(YMM_IV[YMMReg], adYmm);
			}
			memset(transposedData, 0, sizeof(transposedData));
			primate(YMM_IV);
		}
	}

	

	//Handle ciphertext
	if (cLen != 0) {
		//Do magic with last 2 message  sections if message is not integral	
		//TODO 

		//Initialize YMM_V by loading tag XOR key into capacity. Then load rate of last cipherblock into YMM_V.
		u64 transposedTag[5][4] = { 0 };
		u64 cipherBackwards[4]; cipherBackwards[0] -= 8; cipherBackwards[1] -= 8; cipherBackwards[2] -= 8; cipherBackwards[3] -= 8;
		transpose_key_to_capacity_u64(tag, transposedTag);
		transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0);
		for (int i = 0; i < 5; i++) {
			YMM_V[i] = _mm256_xor_si256(keys_YMM[i], _mm256_set_epi64x(transposedTag[i][0], transposedTag[i][1], transposedTag[i][2], transposedTag[i][3]));
			_mm256_xor_si256(YMM_V[i], _mm256_set_epi64x(transposedData[i][0], transposedData[i][1], transposedData[i][2], transposedData[i][3]));
		}
		p1_inv(YMM_V);
		memset(transposedData, 0, sizeof(transposedData));

		//Initialize YMM_M by taking rate part of v and XORing with secondlast ciphertext and store in YMM_M.
		cipherBackwards[0] -= 16; cipherBackwards[1] -= 16; cipherBackwards[2] -= 16; cipherBackwards[3] -= 16;
		transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0);
		for (int i = 0; i < 5; i++) {
			__m256i cipherblock = _mm256_set1_epi64x(transposedData[i][0], transposedData[i][1], transposedData[i][2], transposedData[i][3]);
			__m256i ratemask = _mm256_set1_epi64x(71776119061217280, 71776119061217280, 71776119061217280, 71776119061217280);
			__m256i Vrate = _mm256_and_si256(YMM_V[i], ratemask);
			YMM_M[i] = _mm256_xor_si256(cipherblock, Vrate);
		}

		//XOR V with M[w]10* || 0c
		__m256i temp = _mm256_set1_epi64x(1);
		YMM_V[0] = _mm256_xor_si256(YMM_V[0], _mm256_and_si256(YMM_M[0], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[1] = _mm256_xor_si256(YMM_V[1], _mm256_and_si256(YMM_M[1], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[2] = _mm256_xor_si256(YMM_V[2], _mm256_and_si256(YMM_M[2], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[3] = _mm256_xor_si256(YMM_V[3], _mm256_and_si256(YMM_M[3], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[4] = _mm256_xor_si256(YMM_V[4], _mm256_xor_si256(temp, _mm256_and_si256(YMM_M[4], ratemask))); //Is it neccessary to use the rate-mask in these?
		 
		//detranspose M[w] from registers and store to message
		unsigned char temp_rate[4][RateSize] = { 0 };
		detranspose_rate_to_bytes(YMM_M, temp_rate);
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				int index = j + cLen - 8;
				m[i][index] = temp_rate[i][j];
			}
		}

		//decipher all remaining cipherblocks
		cipherBackwards[0] -= 16; cipherBackwards[1] -= 16; cipherBackwards[2] -= 16; cipherBackwards[3] -= 16;
		while (cipherBackwards[0] > 0 && cipherBackwards[1] > 0 && cipherBackwards[2] > 0 && cipherBackwards[3] > 0) {
			p1_inv(YMM_V);
			transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0); //Increments cipherBackwards with 8, be aware of that
			cipherBackwards[0] -= 8; cipherBackwards[1] -= 8; cipherBackwards[2] -= 8; cipherBackwards[3] -= 8;
			__m256i cipher_temp[5];
			for (int i = 0; i < 5; i++) {
				//load current cipher-blocks to ymm
				cipher_temp[i] = _mm256_set_epi64x(transposedData[i][0], transposedData[i][1], transposedData[i][2], transposedData[i][3]);

				//XOR V_r with relevant C-block and store as plaintextmessage block
				YMM_M[i] = _mm256_xor_si256(cipher_temp[i], YMM_V[i]);

				//use new rate with old capacity as new V
				YMM_V[i] = _mm256_xor_si256(cipher_temp[i], _mm256_and_si256(YMM_V[i], capacitymask));
			}

			//Store the plaintextblock.
			detranspose_rate_to_bytes(YMM_M, temp_rate);
			for (int state = 0; state < 4; state++) {
				for (int index = 0; index < 8; index++) {
					m[state][index + cipherBackwards[0]] = temp_rate[state][index];
				}
			}

			cipherBackwards[0] -= 8; cipherBackwards[1] -= 8; cipherBackwards[2] -= 8; cipherBackwards[3] -= 8;
		}
	}

	YMM_V[0] = _mm256_xor_si256(_mm256_and_si256(YMM_V[0], capacitymask), _mm256_set1_epi64x(1)); //xor with one bit at the very end of the capacity.
	YMM_V[1] = _mm256_and_si256(YMM_V[1], capacitymask), _mm256_set1_epi64x(1);
	YMM_V[2] = _mm256_and_si256(YMM_V[2], capacitymask), _mm256_set1_epi64x(1);
	YMM_V[3] = _mm256_and_si256(YMM_V[3], capacitymask), _mm256_set1_epi64x(1);
	YMM_V[4] = _mm256_and_si256(YMM_V[4], capacitymask), _mm256_set1_epi64x(1);

	bool matches = true;
	for (int i = 0; i < 5; i++) {
		//Compare if V_c and IV_c are equal.
		__m256i equal = _mm256_cmpeq_epi64(YMM_V[i], _mm256_and_si256(YMM_IV[i], capacitymask));
		if (equal.m256i_u64[0] == 0 && equal.m256i_u64[1] == 0 && equal.m256i_u64[2] == 0 && equal.m256i_u64[3] == 0) {
			//Some of them was not equal, since they had the value 0. 
			matches = false;
		}
	}

	if (matches) {
		//return plaintext
	}
	else {
		//invalidate plaintext
	}
	
}
