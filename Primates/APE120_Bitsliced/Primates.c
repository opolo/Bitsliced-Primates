#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>
#include "Debug.h"

#define false 0
#define true 1
#define bool unsigned char

static const __m256i m256iAllOne = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 
									 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 
									 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 
									 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, };
#define XOR(a, b) _mm256_xor_si256(a, b)
#define AND(a, b) _mm256_and_si256(a, b)
#define NEG(a) _mm256_xor_si256(m256iAllOne, a)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR3(a, b, c) _mm256_xor_si256(a, _mm256_xor_si256(b, c))
#define XOR4(a, b, c, d) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, d)))

void transpose_nonce_to_rate_u64(const unsigned char n[4][NonceLength], u64 transposedNonce[5][4][3]);
void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 dataLen, u64 transposedData[5][4], u64 dataTransposedProgress);
void primates120_decrypt(const unsigned char k[4][keyLength], 
	const unsigned char *ciphertexts[4], u64 cLen, 
	const unsigned char *ad[4], u64 adlen, 
	const unsigned char npub[4][NonceLength],
	unsigned char *m[4],
	unsigned char tag[4][keyLength]);

void primate(__m256i *states);
void p1_inv(__m256i *states);

void test_rate_transpose();
void test_capacity_transpose();

void schwabe_bitsliced_primate(__m256i (*state)[2]);
void schwabe_bitsliced_primate_inv(__m256i (*state)[2]);

void detranspose_capacity_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][CapacitySize]);
void detranspose_rate_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][RateSize]);

void transpose_key_to_capacity_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]);

void primates120_encrypt(const unsigned char k[4][keyLength],
	const unsigned char *m[4], u64 mlen,
	const unsigned char *ad[4], u64 adlen,
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
	//test_rate_transpose();
	//test_capacity_transpose();
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
		u64 transposedDataProgress = 0;
		__m256i adYmm;

		while (transposedDataProgress < adlen) {

			transpose_data_to_ratesize_u64(ad, adlen, transposedData, transposedDataProgress);
			transposedDataProgress += 8;
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
	
	//Handle message. If any is present, we do this - else we skip the step.
	if (mlen != 0) {
		u64 transposedDataProgress = 0;
		__m256i msgYmm;

		while (transposedDataProgress< mlen) {

			transpose_data_to_ratesize_u64(m, mlen, transposedData, transposedDataProgress);
			transposedDataProgress += 8; 
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
					ciphertexts[i][transposedDataProgress - 8 + j] = temp_rate[i][j];
				}
			}			
		}
	}

	//TODO: take some bits from last/secondlast state, if message is not integral
	
	//XOR final capacity with key
	for (int YMM_reg = 0; YMM_reg < 5; YMM_reg++) {
		YMM[YMM_reg] = _mm256_xor_si256(YMM[YMM_reg], keys_YMM[YMM_reg]);
	}

	//detranspose key and return it as tag.
	detranspose_capacity_to_bytes(YMM, tag);
}

#define test_rate_data1 { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, 
#define test_rate_data2 { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }

//We only use the first 5 bits in a byte, as a primate element contains only 5 bits.
unsigned char byte_to_primate_element(unsigned char byte) {
	return byte & 31; //31 = 11111 in bits.
}

#define test_cap_data1 { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, 
#define test_cap_data2 { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 },
#define test_cap_data3 { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, { 0xFF }, 
#define test_cap_data4 { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }, { 0x00 }

void test_capacity_transpose() {
	__m256i test_YMM[5];
	_mm256_zeroall;

	unsigned char  test_capacity_after[4][CapacitySize];
	unsigned char test_capacity_before[4][CapacitySize] = { { test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 }, { test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 }, 
															 { test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 }, { test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 } };
	
	u64 test_capacity_YMM_u64[5][4];
	memset(test_capacity_YMM_u64, 0, sizeof(test_capacity_YMM_u64));

	//Print capacity for state 1 (0):
	printf("Before/after capacity transpose/detranspose (of capacity): \n");
	for (int i = 0; i < CapacitySize; i++) {
		printf("%02x ", byte_to_primate_element(test_capacity_before[0][i]));
	}
	printf("\n");

	//transpose capacity
	transpose_key_to_capacity_u64(test_capacity_before, test_capacity_YMM_u64);

	//Load capacity into YMM
	for (int i = 0; i < 5; i++) {
		test_YMM[i] = _mm256_set_epi64x(test_capacity_YMM_u64[i][0], test_capacity_YMM_u64[i][1],
										test_capacity_YMM_u64[i][2], test_capacity_YMM_u64[i][3]);
	}

	//Load capacity from YMM
	detranspose_capacity_to_bytes(test_YMM, test_capacity_after);

	//Print capacity (state 1 (0)) detransposed from YMM:
	for (int i = 0; i < CapacitySize; i++) {
		printf("%02x ", test_capacity_after[0][i]);
	}
	printf("\n");
}

void test_rate_transpose() {
	__m256i test_YMM[5];
	
	unsigned char test_rate_after[4][RateSize * 3];
	unsigned char *test_rate_before[4];
	unsigned char test_rate_before_0123[24] = { test_rate_data1 test_rate_data2 };
	test_rate_before[0] = &test_rate_before_0123;
	test_rate_before[1] = &test_rate_before_0123;
	test_rate_before[2] = &test_rate_before_0123;
	test_rate_before[3] = &test_rate_before_0123;

	u64 test_rate_YMM_u64[5][4] = { 0 };
	u64 transpose_progress = 0;
	u64 dataLen = 24;

	//Print state 1 (0):
	printf("Before/after rate transpose/detranspose (of rate): \n");
	for (int i = 0; i < 24; i++) {
		printf("%02x ", byte_to_primate_element(test_rate_before[0][i]));
	}
	printf("\n");

	//Transpose/detranspose 3 rate sections
	while (transpose_progress < 24) {
		//Transpose rate
		transpose_data_to_ratesize_u64(test_rate_before, dataLen, test_rate_YMM_u64, transpose_progress);
		
		//Load into YMM
		for (int i = 0; i < 5; i++) {
			test_YMM[i] = _mm256_set_epi64x(test_rate_YMM_u64[i][0], test_rate_YMM_u64[i][1], test_rate_YMM_u64[i][2], test_rate_YMM_u64[i][3]);
		}

		//Load from YMM
		detranspose_rate_to_bytes(test_YMM, test_rate_after);

		//Print current rata-data after.
		for (int i = 0; i < 8; i++) {
			printf("%02x ", test_rate_after[0][i]);
		}
	}
	printf("\n");
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
		int index = 0; //index of detransposed primate element. It should be 0-47.
		for (int capacity_byte = 5; capacity_byte >= 0; capacity_byte--) {
			
			int shift = 7;
			for (int bit = 7; bit >= 0; bit--) {
				
				detransposedBytes[state][index] = ((YMM0[capacity_byte + state_offset] >> bit) & 1) |
										   		  (((YMM1[capacity_byte + state_offset] >> bit) & 1) << 1) |
										   		  (((YMM2[capacity_byte + state_offset] >> bit) & 1) << 2) |
										   		  (((YMM3[capacity_byte + state_offset] >> bit) & 1) << 3) |
										   		  (((YMM4[capacity_byte + state_offset] >> bit) & 1) << 4);
				index++;
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

	//We need to store the primate element at index 55 (i.e. the first element of the state) 
	//at index 0 in the bytes that we return.
	//LSB = YMM0, MSB = YMM4

	unsigned char *YMM0 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM1 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM2 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM3 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM4 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned

	//Store YMMs in allocated memory
	_mm256_store_si256(YMM0, YMMs[0]);
	_mm256_store_si256(YMM1, YMMs[1]);
	_mm256_store_si256(YMM2, YMMs[2]);
	_mm256_store_si256(YMM3, YMMs[3]);
	_mm256_store_si256(YMM4, YMMs[4]);

	for (int state = 0; state < 4; state++) {

		int offset = state * 8; //8 bytes = 64 bit
		int shift = 7;
		for (int bit = 0; bit < 8; bit++) {

			detransposedBytes[state][bit] = ((YMM0[6 + offset] >> shift) & 1) |
											((YMM1[6 + offset] >> shift) & 1) << 1 |
											((YMM2[6 + offset] >> shift) & 1) << 2 |
											((YMM3[6 + offset] >> shift) & 1) << 3 |
											((YMM4[6 + offset] >> shift) & 1) << 4;
			shift--;
		}
	}

	_aligned_free(YMM0);
	_aligned_free(YMM1);
	_aligned_free(YMM2);
	_aligned_free(YMM3);
	_aligned_free(YMM4);
}


/*
We need to transpose chunks of the data of sizes that are equal to the rate-part (40 bits) of the primate-cipher.
Thus we transpose 8 primate elements (stored in 8 bytes) at a time.
*/
void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 data_len, u64 transposed_data[5][4], u64 transpose_progress) {

	for (int state_no = 0; state_no < 4; state_no++) {
		//transpose if there are more bytes left
		if ((data_len - transpose_progress) > 0) {
			for (int element = 0; element < 8; element++) {
				transposed_data[0][state_no] = (transposed_data[0][state_no] << 1) |  data[state_no][transpose_progress + element] & 1;
				transposed_data[1][state_no] = (transposed_data[1][state_no] << 1) | (data[state_no][transpose_progress + element] >> 1) & 1;
				transposed_data[2][state_no] = (transposed_data[2][state_no] << 1) | (data[state_no][transpose_progress + element] >> 2) & 1;
				transposed_data[3][state_no] = (transposed_data[3][state_no] << 1) | (data[state_no][transpose_progress + element] >> 3) & 1;
				transposed_data[4][state_no] = (transposed_data[4][state_no] << 1) | (data[state_no][transpose_progress + element] >> 4) & 1;
			}
			//The ratepart is located at the bits 48-55, while these nonce-bits would be placed at bits 0-7. Thus we shift.
			transposed_data[0][state_no] = transposed_data[0][state_no] << 48;
			transposed_data[1][state_no] = transposed_data[1][state_no] << 48;
			transposed_data[2][state_no] = transposed_data[2][state_no] << 48;
			transposed_data[3][state_no] = transposed_data[3][state_no] << 48;
			transposed_data[4][state_no] = transposed_data[4][state_no] << 48;
		}
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
* This function takes an array with 4 capacities of 48 elements and transposes them into a register with the dimensions:
* data[register_no][state_no]
*/void transpose_key_to_capacity_u64(const unsigned char k[4][keyLength], u64 transposedKey[5][4]) {

	//We expect that the key is 240 bits = 48 primate elements (stored in bytes) long.
	for (int state_no = 0; state_no < 4; state_no++) {
		for (int cap_element = 0; cap_element < keyLength; cap_element++) {
			transposedKey[0][state_no] = (transposedKey[0][state_no] << 1) | k[state_no][cap_element] & 1;
			transposedKey[1][state_no] = (transposedKey[1][state_no] << 1) | (k[state_no][cap_element] >> 1) & 1;
			transposedKey[2][state_no] = (transposedKey[2][state_no] << 1) | (k[state_no][cap_element] >> 2) & 1;
			transposedKey[3][state_no] = (transposedKey[3][state_no] << 1) | (k[state_no][cap_element] >> 3) & 1;
			transposedKey[4][state_no] = (transposedKey[4][state_no] << 1) | (k[state_no][cap_element] >> 4) & 1;
		}
	}
}
void primates120_decrypt(const unsigned char k[4][keyLength],
	const unsigned char *ciphertexts[4], u64 cLen,
	const unsigned char *ad[4], u64 adlen,
	const unsigned char npub[4][NonceLength],
	unsigned char *m[4],
	unsigned char tag[4][keyLength])
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

	_mm256_zeroall();

	memset(transposedData, 0, sizeof(transposedData));
	memset(transposedNonce, 0, sizeof(transposedNonce));
	memset(transposedKey, 0, sizeof(transposedKey));
	memset(transposedTag, 0, sizeof(transposedTag));

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

	//Transpose nonces to bitsliced format
	transpose_nonce_to_rate_u64(npub, transposedNonce);

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
		u64 transposedDataProgress = 0;
		__m256i adYmm;

		while (transposedDataProgress < adlen) {

			transpose_data_to_ratesize_u64(ad, adlen, transposedData, transposedDataProgress, 0);
			transposedDataProgress += 8; 
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
		
		//Initialize YMM_V by loading tag XOR key into capacity. Then load rate of last cipherblock into YMM_V.
		u64 transposedTag[5][4] = { 0 };
		u64 cipherBackwards = cLen; 
		cipherBackwards -= 8; //Load last block
		transpose_key_to_capacity_u64(tag, transposedTag);
		transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0);
		for (int i = 0; i < 5; i++) {
			YMM_V[i] = _mm256_xor_si256(keys_YMM[i], _mm256_set_epi64x(transposedTag[i][0], transposedTag[i][1], transposedTag[i][2], transposedTag[i][3]));
			YMM_V[i] = _mm256_xor_si256(YMM_V[i], _mm256_set_epi64x(transposedData[i][0], transposedData[i][1], transposedData[i][2], transposedData[i][3]));
		}
		p1_inv(YMM_V);
		memset(transposedData, 0, sizeof(transposedData));

		//Initialize YMM_M by taking rate part of v and XORing with secondlast ciphertext and store in YMM_M.
		cipherBackwards -= 8; //Load secondlast
		transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0);
		for (int i = 0; i < 5; i++) {
			__m256i cipherblock = _mm256_set_epi64x(transposedData[i][0], transposedData[i][1], transposedData[i][2], transposedData[i][3]);
			__m256i Vrate = _mm256_and_si256(YMM_V[i], ratemask);
			YMM_M[i] = _mm256_xor_si256(cipherblock, Vrate);
		}

		//detranspose M[w] from registers and store to message
		unsigned char temp_rate[4][RateSize];
		memset(temp_rate, 0, sizeof(temp_rate)); //Is arrays on the stack initialized with 0s? i.e. is this needed?
		detranspose_rate_to_bytes(YMM_M, temp_rate);
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				int index = j + cLen - 8;
				m[i][index] = temp_rate[i][j];
			}
		}

		//XOR V with M[w]10* || 0c
		YMM_V[0] = _mm256_xor_si256(YMM_V[0], _mm256_and_si256(YMM_M[0], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[1] = _mm256_xor_si256(YMM_V[1], _mm256_and_si256(YMM_M[1], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[2] = _mm256_xor_si256(YMM_V[2], _mm256_and_si256(YMM_M[2], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[3] = _mm256_xor_si256(YMM_V[3], _mm256_and_si256(YMM_M[3], ratemask)); //Is it neccessary to use the rate-mask in these?
		YMM_V[4] = _mm256_xor_si256(YMM_V[4], _mm256_and_si256(YMM_M[4], ratemask)); //Is it neccessary to use the rate-mask in these?
		
		//decipher all remaining cipherblocks. start by taking W[i-2] (i.e. thirdlast block)
		cipherBackwards -= 8;

		for (int remaining_cipherblock_count = (cLen / RateSize) - 1; remaining_cipherblock_count > 0; remaining_cipherblock_count--) {

			p1_inv(YMM_V);

			memset(transposedData, 0, sizeof(transposedData));
			__m256i cipher_temp[5];
			if (remaining_cipherblock_count < 2)
			{
				//We need to transpose C[0] instead now. 
				transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0); 
				for (int i = 0; i < 5; i++) {
					//load current cipher-blocks to ymm
					cipher_temp[i] = _mm256_and_si256(YMM_IV[i], ratemask);
				}
			}
			else {
				transpose_data_to_ratesize_u64(ciphertexts, cLen, transposedData, cipherBackwards, 0);
				for (int i = 0; i < 5; i++) {
					//load current cipher-blocks to ymm
					cipher_temp[i] = _mm256_set_epi64x(transposedData[i][0], transposedData[i][1], transposedData[i][2], transposedData[i][3]);
				}
			}

			cipherBackwards -= 8; 
			
			for (int i = 0; i < 5; i++) {
				//XOR V_r with relevant C-block and store as plaintextmessage block
				YMM_M[i] = _mm256_xor_si256(cipher_temp[i], YMM_V[i]);

				//use new rate with old capacity as new V
				YMM_V[i] = _mm256_xor_si256(cipher_temp[i], _mm256_and_si256(YMM_V[i], capacitymask));
			}

			//Store the plaintextblock.
			detranspose_rate_to_bytes(YMM_M, temp_rate);
			for (int state = 0; state < 4; state++) {
				int offset = (remaining_cipherblock_count-1) * 8;
				for (int index = 0; index < 8; index++) {
					m[state][index + offset] = temp_rate[state][index];
				}
			}

			cipherBackwards -= 8;
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


void schwabe_primate_test() {
	__m256i state[5][2];

	for (int i = 0; i < 5; i++) {
		state[i][0] = _mm256_setzero_si256();
		state[i][1] = _mm256_setzero_si256();
	}

	schwabe_bitsliced_primate(state);
	schwabe_bitsliced_primate_inv(state);
}

void schwabe_bitsliced_primate_inv(__m256i (*state)[2]) {
	__m256i x[5][2];
	for (int i = 0; i < 5; i++) {
		x[i][0] = state[i][0];
		x[i][1] = state[i][1];
	}

	for (int round = 0; round < PrimateRounds; round++) {
	
		//SHIFT ROWS INVERSE START
		//shifted to the right (from top down):
		//0,1,2,3,4,5,7
		//TODO
		__m256i shuffleControlMaskFirstReg = _mm256_set_epi8(	0,	0,	0,	0,	0,	0,	0,	0,
																15,	8,	9,	10,	11,	12,	13,	14,
																22,	23,	16,	17,	18,	19,	20,	21,
																29, 30, 31, 24, 25, 26, 27, 28);
		__m256i shuffleControlMaskSecondReg = _mm256_set_epi8(	4,	5,	6,	7,	0,	1,	2,	3,
																11,	12,	13,	14,	15,	8,	9,	10,
																17,	18,	19,	20,	21,	22,	23,	16,
																256, 256, 256, 256, 256, 256, 256, 256); //Setting it to 256 makes shuffle zero the bits
		for (int reg = 0; reg < 5; reg++) {

			x[reg][0] = _mm256_shuffle_epi8(x[reg][0], shuffleControlMaskFirstReg);
			x[reg][1] = _mm256_shuffle_epi8(x[reg][1], shuffleControlMaskSecondReg);
		}
		//SHIFT ROWS INVERSE END

		//SUB BYTES INVERSE START
		for (int i = 0; i < 2; i++) {
			//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).

			//Helper variables
			__m256i q[20];
			__m256i t[13];
			__m256i y[5];

			q[0] = XOR3(x[0][i], x[1][i], x[2][i]);
			q[1] = NEG(XOR(x[2][i], x[4][i]));
			t[0] = AND(q[0], q[1]);

			q[2] = x[0][i];
			q[3] = x[1][i];
			t[1] = AND(q[2], q[3]);

			q[4] = XOR3(x[2][i], x[3][i], t[0]);
			q[5] = NEG(x[1][i]);
			t[2] = AND(q[4], q[5]);

			q[6] = XOR3(x[1][i], t[1], t[2]);
			q[7] = XOR(x[2][i], x[4][i]);
			t[3] = AND(q[6], q[7]);

			q[8] = XOR4(x[2][i], t[0], t[2], t[3]);
			q[9] = XOR4(x[0][i], x[3][i], x[4][i], XOR3(t[1], t[2], t[3]));
			t[4] = AND(q[8], q[9]);

			q[10] = XOR4(x[0][i], x[2][i], x[3][i], XOR3(t[1], t[2], t[3]));
			q[11] = XOR4(x[1][i], x[3][i], t[0], t[2]);
			t[5] = AND(q[10], q[11]);

			q[12] = XOR(x[0][i], x[4][i]);
			q[13] = XOR4(t[0], t[3], t[4], t[5]);
			t[6] = AND(q[12], q[13]);

			q[14] = NEG(XOR4(x[0][i], x[1][i], x[2][i], XOR4(x[4][i], t[0], t[1], XOR4(t[3], t[4], t[5], t[6]))));
			q[15] = XOR4(x[0][i], x[3][i], t[0], XOR4(t[1], t[2], t[4], t[6]));
			t[7] = AND(q[14], q[15]);

			q[16] = NEG(XOR4(x[2][i], x[3][i], t[2], t[5]));
			q[17] = NEG(XOR4(x[0][i], x[1][i], x[4][i], XOR4(t[0], t[1], t[2], XOR3(t[3], t[6], t[7]))));
			t[8] = AND(q[16], q[17]);

			q[18] = XOR4(x[4][i], t[2], t[5], XOR(t[6], t[8]));
			q[19] = NEG(XOR4(x[0][i], x[1][i], x[4][i], XOR3(t[4], t[7], t[8])));
			t[9] = AND(q[18], q[19]);

			y[0] = XOR4(x[0][i], x[1][i], t[0], XOR3(t[6], t[7], t[9]));
			y[1] = XOR(t[0], t[3], t[6]);
			y[2] = XOR4(t[3], t[5], t[6], t[7]);
			y[3] = XOR3(t[1], t[2], t[4]);;
			y[4] = XOR4(x[1][i], t[0], t[4], t[8]);

			x[0][i] = y[0];
			x[1][i] = y[1];
			x[2][i] = y[2];
			x[3][i] = y[3];
			x[4][i] = y[4];



		}
		//SUB BYTES INVERSE END
	}

	for (int i = 0; i < 5; i++) {
		state[i][0] = x[i][0];
		state[i][1] = x[i][1];
	}


}


void primate(__m256i *state) {
}


//This one has the Schwabe layout
void schwabe_bitsliced_primate(__m256i (*state)[2]) {

	__m256i x[5][2];
	for (int i = 0; i < 5; i++) {
		x[i][0] = state[i][0];
		x[i][1] = state[i][1];
	}

	for (int round = 0; round < PrimateRounds; round++) {

		//SHIFT ROWS (primate 120) START
		//shifted to the left (from top down):
		//0,1,2,3,4,5,7
		//TODO
		__m256i shuffleControlMaskFirstReg = _mm256_set_epi8(	0,	0,	0,	0,	0,	0,	0, 0, 
																9,	10, 11, 12, 13, 14, 15, 8,
																18, 19, 20, 21, 22, 23, 16, 17,
																27, 28, 29, 30, 31, 24, 25, 26); 
		__m256i shuffleControlMaskSecondReg = _mm256_set_epi8(	4,	5,	6,	7,	0,	1,	2,	3,
																13, 14, 15, 8,	9,	10,	11,	12,
																23, 16, 17, 18, 19, 20, 21, 22,
																256, 256, 256, 256, 256, 256, 256, 256); //Setting it to 256 makes shuffle zero the bits
		for (int reg = 0; reg < 5; reg++) {
			
			x[reg][0] = _mm256_shuffle_epi8(x[reg][0], shuffleControlMaskFirstReg);
			x[reg][1] = _mm256_shuffle_epi8(x[reg][1], shuffleControlMaskSecondReg);
		}
		//SHIFT ROWS END


		//SBOX START
		for (int i = 0; i < 2; i++) {
			//We make this loop to handle both pairs of the registers identically (since 8 states fills 2 registers (or 10, due to 5*2 from bitlicing).

			//Helper variables
			__m256i z[13];
			__m256i q[13];
			__m256i t[13];
			__m256i y[5];

			z[0] = XOR(x[0][i], x[4][i]);
			z[1] = XOR(x[1][i], x[2][i]);
			z[2] = XOR(x[2][i], x[3][i]);

			q[0] = XOR(x[0][i], x[3][i]);
			t[0] = OR(q[0],		x[1][i]);
			q[2] = XOR(x[1][i], x[3][i]);

			q[3] = NEG(XOR(x[0][i], x[2][i]));
			t[1] = OR(q[2],			q[3]);
			q[4] = XOR(x[1][i],		z[0]);

			q[5] = XOR(x[0][i],		z[2]);
			t[2] = AND(q[4],		q[5]);
			q[6] = NEG(XOR(x[4][i], q[5]));


			q[7] = XOR(x[4][i],	z[1]);
			t[3] = OR(q[6],		q[7]);
			q[8] = XOR(q[4],	z[2]);

			z[9] = XOR(t[0],	t[3]);
			q[9] = XOR(x[2][i], z[9]);
			t[4] = AND(q[8],	q[9]);

			q[10] = NEG(XOR(x[3][i],	z[0]));
			t[5]  = AND(q[10],			z[0]);
			q[12] = NEG(XOR4(z[1],		z[9], t[2], t[4]));

			t[6] = AND(q[12],	z[2]);
			z[3] = XOR(t[5],	t[6]);
			z[4] = XOR(t[3],	z[3]);


			z[5] = XOR(t[2], z[4]);
			z[6] = XOR(t[1], t[6]);
			z[7] = XOR(t[4], z[5]);

			z[8] = XOR(t[1],	z[7]);
			z[10] = XOR(t[0],	z[7]);
			z[11] = XOR(t[4],	z[4]);
			z[12] = XOR(z[6], z[11]);

			y[0] = NEG(XOR(q[2],		z[5]));
			y[1] = XOR(z[0],			z[8]);
			y[2] = XOR(q[7],			z[12]);
			y[3] = XOR(q[6],			z[11]);
			y[4] = XOR(x[2][i],	z[10]);

			x[0][i] = y[0];
			x[1][i] = y[1];
			x[2][i] = y[2];
			x[3][i] = y[3];
			x[4][i] = y[4];

		}
		//SBOX END
	}


	for (int i = 0; i < 5; i++) {
		state[i][0] = x[i][0];
		state[i][1] = x[i][1];
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


