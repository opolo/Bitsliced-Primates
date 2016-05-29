#include "Encrypt.h"
#include "Primate.h"
#include <stdio.h>
#include <string.h>

void init_bitslice_state(YMM *key, const u8 *n, const u8 *k, YMM(*state)[2]);
void load_data_into_u64(u8 *m, u64 mlen, u64 rates[8], u64 *progress);
YMM expand_bits_to_bytes(int x);

void crypto_aead_encrypt(
	u8 *c, u64 *clen,
	const u8 *m, const u64 mlen,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u8 *tag) {

	YMM state[5][2];
	YMM key[5];

	//init_bitslice_state. V = p1(0^r || K || N)
	init_bitslice_state(key, nonce, k, state);

	//V = V_r || (K || 0^(c/2) XOR V_c
	state[0][0] = _mm256_xor_si256(state[0][0], key[0]);
	state[1][0] = _mm256_xor_si256(state[1][0], key[1]);
	state[2][0] = _mm256_xor_si256(state[2][0], key[2]);
	state[3][0] = _mm256_xor_si256(state[3][0], key[3]);
	state[4][0] = _mm256_xor_si256(state[4][0], key[4]);

	
	//Add a constant to each state to avoid ECB'esque problems.
	//TODO

	if (adlen > 0) {
		//TODO ADLEN
	}

	p3(state);

	u64 progress = 0;
	u64 message_u64[5];
	YMM message_YMM[5];
	while (progress < mlen) {
		
		//Get next 40 bytes of data
		load_data_into_u64(m, mlen, message_u64, &progress);

		//Load it into registers and create ciphertext by XOR v_r with message.
		for (int i = 0; i < 5; i++) {
			
			//Getting some weird bug, if we dont do it like this. Question has been opened about it: http://stackoverflow.com/questions/37509129/potential-bug-in-visual-studio-c-compiler-or-in-intel-intrinsics-avx2-mm256-s
			YMM rate_ymm = { 0 };
			rate_ymm.m256i_u64[0] = message_u64[i];

			message_YMM[i] = XOR(rate_ymm, state[i][0]);
		}

		//Extract ciphertext
		memcpy(&c[progress - 40], &message_YMM[0].m256i_u64, sizeof(u64));
		memcpy(&c[progress - 32], &message_YMM[1].m256i_u64, sizeof(u64));
		memcpy(&c[progress - 24], &message_YMM[2].m256i_u64, sizeof(u64));
		memcpy(&c[progress - 16], &message_YMM[3].m256i_u64, sizeof(u64));
		memcpy(&c[progress - 8],  &message_YMM[4].m256i_u64, sizeof(u64));
	}

	
	//XOR key to state
	for (int i = 0; i < 5; i++) {
		state[0][0] = XOR(state[0][0], key[0]);
		state[1][0] = XOR(state[1][0], key[1]);
		state[2][0] = XOR(state[2][0], key[2]);
		state[3][0] = XOR(state[3][0], key[3]);
		state[4][0] = XOR(state[4][0], key[4]);
	}
	p1(state);

	//Calculate tag
	for (int i = 0; i < 5; i++) {
		state[0][0] = XOR(state[0][0], key[0]);
		state[1][0] = XOR(state[1][0], key[1]);
		state[2][0] = XOR(state[2][0], key[2]);
		state[3][0] = XOR(state[3][0], key[3]);
		state[4][0] = XOR(state[4][0], key[4]);
	}

	//Ciphertext length is the length of the plaintext + padding
	*clen = (mlen % 40) + mlen;
	
	//Extract 120bit tag that is not part of rate..., and to where key was just XORed 
	memcpy(&tag[0], &state[0][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[8], &state[0][0].m256i_u64[1], sizeof(u8)*7);
	
}

void crypto_aead_decrypt(
	u8 *c, u64 clen,
	const u8 *m,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u8 *tag) {

	YMM state[5][2];
	YMM key[5];

	//init_bitslice_state. V = p1(0^r || K || N)
	init_bitslice_state(key, nonce, k, state);

	//V = V_r || (K || 0^(c/2) XOR V_c
	state[0][0] = _mm256_xor_si256(state[0][0], key[0]);
	state[1][0] = _mm256_xor_si256(state[1][0], key[1]);
	state[2][0] = _mm256_xor_si256(state[2][0], key[2]);
	state[3][0] = _mm256_xor_si256(state[3][0], key[3]);
	state[4][0] = _mm256_xor_si256(state[4][0], key[4]);

	//Add a constant to each state to avoid ECB'esque problems.
	//TODO

	if (adlen > 0) {
		//TODO ADLEN
	}

	p3(state);
	print_state_as_hex(state);
	u64 progress = 0;
	u64 cipher_u64[5];
	YMM cipher_YMM[5];
	while (progress < clen) {

		//Get next 40 bytes of data
		load_data_into_u64(c, clen, cipher_u64, &progress);

		//Load it into registers and create ciphertext by XOR v_r with message.
		for (int i = 0; i < 5; i++) {

			//Getting some weird bug, if we dont do it like this. Question has been opened about it: http://stackoverflow.com/questions/37509129/potential-bug-in-visual-studio-c-compiler-or-in-intel-intrinsics-avx2-mm256-s
			YMM rate_ymm = { 0 };
			rate_ymm.m256i_u64[0] = cipher_u64[i];

			cipher_YMM[i] = XOR(rate_ymm, state[i][0]);

		}

		//Extract ciphertext
		memcpy(&m[progress - 40], &cipher_YMM[0].m256i_u64, sizeof(u64));
		memcpy(&m[progress - 32], &cipher_YMM[1].m256i_u64, sizeof(u64));
		memcpy(&m[progress - 24], &cipher_YMM[2].m256i_u64, sizeof(u64));
		memcpy(&m[progress - 16], &cipher_YMM[3].m256i_u64, sizeof(u64));
		memcpy(&m[progress - 8], &cipher_YMM[4].m256i_u64, sizeof(u64));
	}

	//Calculate tag later.
}

/*
Progress = progress in bytes.
Loads 40 bytes into 5x u64. 8 bytes per u64
*/
void load_data_into_u64(u8 *m, u64 mlen, u64 rates[8], u64 *progress) {
	
	//Are there 40 available bytes? Handle them easy now then.
	if (*progress + 40 <= mlen) {
		memcpy(rates, &m[*progress], sizeof(u8) * 40);
		*progress += 40;
		return;
	}

	//5x u64. 8 bytes each. 40 bytes in total.
	//At some point during the next 40 bytes, we need to pad
	for (int i = 0; i < 5; i++) {

		//Do we need to pad this u64?
		if (mlen >= progress + 8) {
			//No, there are 8 available bytes. 
			memcpy(&rates[i], &m[*progress], sizeof(u8) * 8);
		}
		else {
			//yes, there are less than 8 available. 
			//Are there any available?
			if (mlen <= *progress) {
				//No.
				rates[i] = 0;
			}

			//How many are there left?
			int available_bytes = mlen - *progress;
			int zero_padding_bytes = 8 - available_bytes -1; //-1 as we need to pad the first byte with 1, not zero.

			//Load remaining bytes
			for (int j = 0; j < available_bytes; j++) {
				rates[i] |= m[*progress + j];
				rates[i] <<= 8;
			}

			//Pad with 1 at bit 8 (since we just shifted 8 bits in the loop before); 128 = 1000 0000
			rates[i] |= 128;

			//Shift in the 0 value bytes as padding.
			rates[i] <<= 8 * zero_padding_bytes;
		}

		*progress += 8;
		 
	}

}

static const __m256i ymm_all_zero = { 0, 0, 0, 0 };
static const __m256i ymm_all_one = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF };

void init_bitslice_state(YMM *key, const u8 *n, const u8 *k, YMM(*state)[2]) {

	//Create bitsliced state. Remember to transpose 1 bit to 1 byte, since we bitslice 8 states at once. We do this via AVX broadcasting.
	int key_bits[5] = { 0 };
	int nonce_bits[5] = { 0 };

	//The low 8 bits are kept zero each time, as the rate should be zero and is stored here. 
	key_bits[0] = (k[0] << 8) | (k[1]  << 16) | (k[2]  << 24);
	key_bits[1] = (k[3] << 8) | (k[4]  << 16) | (k[5]  << 24);
	key_bits[2] = (k[6] << 8) | (k[7]  << 16) | (k[8]  << 24);
	key_bits[3] = (k[9] << 8) | (k[10] << 16) | (k[11] << 24);
	key_bits[4] = (k[12]<< 8) | (k[13] << 16) | (k[14] << 24);

	//The high 8 bits are kept zeroed each time, as the size of 8 primate states takes 2240 bits. The first registers uses 1280 bits, so there is 64 bits per register in the second half unused
	//It is more efficient to keep the high bits unused than the lower bits due to unneccesary shifting otherwise.
	nonce_bits[0] = (n[0] << 16) | (n[1]  << 8) | n[2];
	nonce_bits[1] = (n[3] << 16) | (n[4]  << 8) | n[5];
	nonce_bits[2] = (n[6] << 16) | (n[7]  << 8) | n[8];
	nonce_bits[3] = (n[9] << 16) | (n[10] << 8) | n[11];
	nonce_bits[4] = (n[12]<< 16) | (n[13] << 8) | n[14];

	//broadcast each of the 32 bits (24 if excluding zeroed space) to a 256bit YMM register. This means that each bit gets broadcast to 8 bits, which is ideal here.
	//Key
	state[0][0] = key[0] = expand_bits_to_bytes(key_bits[0]);
	state[1][0] = key[1] = expand_bits_to_bytes(key_bits[1]);
	state[2][0] = key[2] = expand_bits_to_bytes(key_bits[2]);
	state[3][0] = key[3] = expand_bits_to_bytes(key_bits[3]);
	state[4][0] = key[4] = expand_bits_to_bytes(key_bits[4]);

	//Nonce
	state[0][1] = expand_bits_to_bytes(nonce_bits[0]);
	state[1][1] = expand_bits_to_bytes(nonce_bits[0]);
	state[2][1] = expand_bits_to_bytes(nonce_bits[0]);
	state[3][1] = expand_bits_to_bytes(nonce_bits[0]);
	state[4][1] = expand_bits_to_bytes(nonce_bits[0]);

	p1(state);
}

YMM expand_bits_to_bytes(int x)
{
	__m256i xbcast = _mm256_set1_epi32(x);    // we only use the low 32bits of each lane, but this is fine with AVX2

											  // Each byte gets the source byte containing the corresponding bit
	__m256i shufmask = _mm256_set_epi64x(
		0x0303030303030303, 0x0202020202020202,
		0x0101010101010101, 0x0000000000000000);
	__m256i shuf = _mm256_shuffle_epi8(xbcast, shufmask);

	__m256i andmask = _mm256_set1_epi64x(0x8040201008040201);  // every 8 bits -> 8 bytes, pattern repeats.
	__m256i isolated_inverted = _mm256_andnot_si256(shuf, andmask);

	// this is the extra step: compare each byte == 0 to produce 0 or -1
	return _mm256_cmpeq_epi8(isolated_inverted, _mm256_setzero_si256());
}