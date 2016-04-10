#include <immintrin.h>
#include "Primates.h"
#include <stdio.h>
#include <string.h>

void u64_to_string(char *str, u64 number);
void transpose_nonce_to_u64(const unsigned char *n, u64 transposedNonce[5][3]);
void print_keys(const unsigned char *k[4]);
void transpose_key_to_u64(const unsigned char *k[4], u64 transposedKey[5][4]);

void primates120_encrypt(const unsigned char k[4][keyLength],
						 const unsigned char *m[4], u64 mlen[4],
						 const unsigned char *ad[4], u64 adlen[4],
						 const unsigned char *npub[4]) {
	//Declarations
	__m256i YMM[5]; //YMM registers
	u64 transposedData[4][5]; //4x blocks with individual keys/nonces. 5x registers

	//Prepare registers
	_mm256_zeroall;
	memset(transposedData, 0, sizeof(transposedData));

	//Implementation note: Each YMM is 256 bits. Each primate state takes up 56 bits in each YMM.
	//Thus there is room for a total of 4 primate states. To make the implementation more efficient, we align
	//each state with 64 bits. There will still only be space in the vector for a total of 4 states anyway.

	//TEST 
	print_keys(k);
	//ENDTEST 

	//Transpose keys to bitsliced format.
	for (int key_no = 0; key_no < 4; key_no++) {
		transpose_key_to_u64(k[key_no], transposedData[key_no]);
	}

	//Load transposed keys into registers.
	for (int YMM_no = 0; YMM_no < 5; YMM_no++) {
		YMM[YMM_no] = _mm256_set_epi64x(transposedData[0][YMM_no], transposedData[1][YMM_no],
			transposedData[2][YMM_no], transposedData[4][YMM_no]);
	}

	//Transpose nonces to bitsliced format
	memset(transposedData, 0, sizeof(transposedData));
	for (int nonce_no = 0; nonce_no < 4; nonce_no++) {
		transpose_nonce_to_u64(npub[nonce_no], transposedData[nonce_no]);
	}

	//XOR the nonce in rate-size chuncks to the rate-part of the state and do primate permutation.
	//The rate-size is 40 bits and the nonce is 120 bits. 
	//Note: The nonce is 120 bits long, but stored in 4x u64's (=256 bits). This is okay as the rest of the u64's are zeroed. 
	for (int nonce_rate_part = 0; nonce_rate_part < 3; nonce_rate_part++) {
	//	YMM[0] = _mm256_xor_si256(YMM[0], _mm256_set_epi64x());
	//	YMM[1] = ;
		//YMM[2] = ;
//		YMM[3] = ;
	//	YMM[4] = ;

		//primate120_encrypt_permutate_state(YMM);
	}
	
}

/*
* This functions accepts 4 nonces of minimum length 15 bytes and transposes the bits from these
* into a multidimensional array of size n[YMM_no][nonce-section]. Each nonce-section is 40 bits.
*/
void transpose_nonce_to_u64(const unsigned char *n, u64 transposedNonce[5][3]) {
	unsigned char singleByte;
	int offset = 0;

	for (int index = 0; index < NonceLength; index += 5) {
		singleByte = n[index];
		transposedNonce[0][offset] = (transposedNonce[0][offset] << 1) | (singleByte & 1); //1. bit
		transposedNonce[1][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[2][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[3][offset] = (transposedNonce[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[4][offset] = (transposedNonce[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[0][offset] = (transposedNonce[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[1][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[2][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 7) & 1);

		singleByte = n[index + 1];
		transposedNonce[3][offset] = (transposedNonce[0][offset] << 1) | (singleByte & 1); //9. bit
		transposedNonce[4][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[0][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[1][offset] = (transposedNonce[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[2][offset] = (transposedNonce[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[3][offset] = (transposedNonce[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[4][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[0][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 7) & 1);

		singleByte = n[index + 2];
		transposedNonce[1][offset] = (transposedNonce[0][offset] << 1) | (singleByte & 1); //17. bit
		transposedNonce[2][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[3][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[4][offset] = (transposedNonce[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[0][offset] = (transposedNonce[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[1][offset] = (transposedNonce[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[2][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[3][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 7) & 1); 

		singleByte = n[index + 3];
		transposedNonce[4][offset] = (transposedNonce[0][offset] << 1) | (singleByte & 1); //25. bit
		transposedNonce[0][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[1][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[2][offset] = (transposedNonce[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[3][offset] = (transposedNonce[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[4][offset] = (transposedNonce[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[0][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[1][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 7) & 1);

		singleByte = n[index + 4];
		transposedNonce[2][offset] = (transposedNonce[0][offset] << 1) | (singleByte & 1); //33. bit
		transposedNonce[3][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[4][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[0][offset] = (transposedNonce[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[1][offset] = (transposedNonce[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[2][offset] = (transposedNonce[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[3][offset] = (transposedNonce[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[4][offset] = (transposedNonce[2][offset] << 1) | ((singleByte >> 7) & 1); //40. bit

		//Increment offset for next nonce-section in this loop.
		offset++;
	}
}

/*
* This function takes 4 keys and transpose them into a register with the dimensions:
* k[register_no][key_no]
*/
void transpose_key_to_u64(const unsigned char *k[4], u64 transposedKey[5][4]) {

	unsigned char singleByte;
	int offset = 0;

	//We expect that the key is 240 bits = 30 bytes long.
	for (int index = 0; index < keyLength; index += 5) {

		singleByte = k[index];
		transposedKey[0][offset] = (transposedKey[0][offset] << 1) | (singleByte & 1); //first bit
		transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedKey[3][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedKey[4][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedKey[0][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedKey[1][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedKey[2][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 1];
		transposedKey[3][offset] = (transposedKey[0][offset] << 1) | (singleByte & 1); //first bit
		transposedKey[4][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedKey[0][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedKey[1][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedKey[2][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedKey[3][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedKey[4][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedKey[0][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 2];
		transposedKey[1][offset] = (transposedKey[0][offset] << 1) | (singleByte & 1); //first bit
		transposedKey[2][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedKey[3][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedKey[4][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedKey[0][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedKey[1][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedKey[2][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedKey[3][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 3];
		transposedKey[4][offset] = (transposedKey[0][offset] << 1) | (singleByte & 1); //first bit
		transposedKey[0][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedKey[1][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedKey[2][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedKey[3][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedKey[4][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedKey[0][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedKey[1][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 4];
		transposedKey[2][offset] = (transposedKey[0][offset] << 1) | (singleByte & 1); //first bit
		transposedKey[3][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 1) & 1);
		transposedKey[4][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 2) & 1);
		transposedKey[0][offset] = (transposedKey[3][offset] << 1) | ((singleByte >> 3) & 1);
		transposedKey[1][offset] = (transposedKey[4][offset] << 1) | ((singleByte >> 4) & 1);
		transposedKey[2][offset] = (transposedKey[0][offset] << 1) | ((singleByte >> 5) & 1);
		transposedKey[3][offset] = (transposedKey[1][offset] << 1) | ((singleByte >> 6) & 1);
		transposedKey[4][offset] = (transposedKey[2][offset] << 1) | ((singleByte >> 7) & 1); //eight bit
	}
}



/* EVERYTHING BELOW IS FOR TESTING AND NOT MEANT FOR THE ACTUAL IMPLEMENTATION*/

#define YMMCount 5
#define YMMLength 256

//void print_YMMs(__m256i YMM[5]) {}

void print_keys(const unsigned char k[4][keyLength]) {
	printf("Key 0: %s \n", k[0]);
	printf("Key 1: %s \n", k[1]);
	printf("Key 2: %s \n", k[2]);
	printf("Key 3: %s \n", k[3]);
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