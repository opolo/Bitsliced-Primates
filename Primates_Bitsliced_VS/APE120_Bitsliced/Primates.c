#include <immintrin.h>
#include "Primates.h"

void primates120_encrypt(const unsigned char *k[4],
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
	for (int nonce_rate_part = 0; nonce_rate_part < 3; nonce_rate_part++) {
				//TODO: asas
	}

}

void primate120_encrypt_permutate_state(__m256i YMM) {
}

void transpose_nonce_to_u64(const unsigned char *n, u64 transposedNonce[5]) {
	unsigned char singleByte;

	for (int index = 0; index < NonceLength; index += 5) {
		singleByte = n[index];
		transposedNonce[0] = (transposedNonce[0] << 1) | (singleByte & 1); //first bit
		transposedNonce[1] = (transposedNonce[1] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[2] = (transposedNonce[2] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[3] = (transposedNonce[3] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[4] = (transposedNonce[4] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[0] = (transposedNonce[0] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[1] = (transposedNonce[1] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[2] = (transposedNonce[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = n[index + 1];
		transposedNonce[3] = (transposedNonce[0] << 1) | (singleByte & 1); //first bit
		transposedNonce[4] = (transposedNonce[1] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[0] = (transposedNonce[2] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[1] = (transposedNonce[3] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[2] = (transposedNonce[4] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[3] = (transposedNonce[0] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[4] = (transposedNonce[1] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[0] = (transposedNonce[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = n[index + 2];
		transposedNonce[1] = (transposedNonce[0] << 1) | (singleByte & 1); //first bit
		transposedNonce[2] = (transposedNonce[1] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[3] = (transposedNonce[2] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[4] = (transposedNonce[3] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[0] = (transposedNonce[4] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[1] = (transposedNonce[0] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[2] = (transposedNonce[1] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[3] = (transposedNonce[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = n[index + 3];
		transposedNonce[4] = (transposedNonce[0] << 1) | (singleByte & 1); //first bit
		transposedNonce[0] = (transposedNonce[1] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[1] = (transposedNonce[2] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[2] = (transposedNonce[3] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[3] = (transposedNonce[4] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[4] = (transposedNonce[0] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[0] = (transposedNonce[1] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[1] = (transposedNonce[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = n[index + 4];
		transposedNonce[2] = (transposedNonce[0] << 1) | (singleByte & 1); //first bit
		transposedNonce[3] = (transposedNonce[1] << 1) | ((singleByte >> 1) & 1);
		transposedNonce[4] = (transposedNonce[2] << 1) | ((singleByte >> 2) & 1);
		transposedNonce[0] = (transposedNonce[3] << 1) | ((singleByte >> 3) & 1);
		transposedNonce[1] = (transposedNonce[4] << 1) | ((singleByte >> 4) & 1);
		transposedNonce[2] = (transposedNonce[0] << 1) | ((singleByte >> 5) & 1);
		transposedNonce[3] = (transposedNonce[1] << 1) | ((singleByte >> 6) & 1);
		transposedNonce[4] = (transposedNonce[2] << 1) | ((singleByte >> 7) & 1); //eight bit
	}
}

void transpose_key_to_u64(const unsigned char *k, u64 transposedKey[5]) {

	unsigned char singleByte;

	//We expect that the key is 240 bits = 30 bytes long.
	for (int index = 0; index < keyLength; index += 5) {

		singleByte = k[index];
		transposedKey[0] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
		transposedKey[1] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
		transposedKey[2] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
		transposedKey[3] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
		transposedKey[4] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
		transposedKey[0] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
		transposedKey[1] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
		transposedKey[2] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 1];
		transposedKey[3] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
		transposedKey[4] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
		transposedKey[0] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
		transposedKey[1] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
		transposedKey[2] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
		transposedKey[3] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
		transposedKey[4] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
		transposedKey[0] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 2];
		transposedKey[1] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
		transposedKey[2] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
		transposedKey[3] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
		transposedKey[4] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
		transposedKey[0] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
		transposedKey[1] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
		transposedKey[2] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
		transposedKey[3] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 3];
		transposedKey[4] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
		transposedKey[0] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
		transposedKey[1] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
		transposedKey[2] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
		transposedKey[3] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
		transposedKey[4] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
		transposedKey[0] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
		transposedKey[1] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit

		singleByte = k[index + 4];
		transposedKey[2] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
		transposedKey[3] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
		transposedKey[4] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
		transposedKey[0] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
		transposedKey[1] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
		transposedKey[2] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
		transposedKey[3] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
		transposedKey[4] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit
	}
}





void primates120_decrypt() {
}