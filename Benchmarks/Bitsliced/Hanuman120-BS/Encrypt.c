#include "Encrypt.h"
#include "Primate.h"
#include <stdio.h>
#include <string.h>

void init_bitslice_state(YMM *key, const u8 *n, const u8 *k, YMM(*state)[2]);
int load_data_into_u64(const u8 *m, u64 mlen, u64 rates[5], u64 *progress);
YMM expand_bits_to_bytes(int x);

void initialize_common(YMM(*state)[2], const u8 *k, const u8 *nonce, YMM key[5], u64 adlen, const u8 *ad) {
	//V = p1(0^r || K || N)
	init_bitslice_state(key, nonce, k, state);
	
	//Add a different constant to each capacity (identically to how primate permutations adds constants) to avoid ECB'esque problems... Constants chosen: 01, 02, 05, 0a, 15, 0b, 17, 0e, 
	state[0][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000001010111000000000), state[0][0]);
	state[1][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000101011100000000), state[1][0]);
	state[2][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000010101100000000), state[2][0]);
	state[3][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000001010100000000), state[3][0]);
	state[4][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000000101000000000), state[4][0]);

	p1(state);

	//AD
	if (adlen > 0) {
		u64 progress = 0;
		u64 data[5] = { 0 };
		while (progress + 40 < adlen) {
			//XOR next 40 bytes of data to rate of states;
			load_data_into_u64(ad, adlen, data, &progress);
			state[0][0] = XOR(_mm256_setr_epi64x(data[0], 0, 0, 0), state[0][0]);
			state[1][0] = XOR(_mm256_setr_epi64x(data[1], 0, 0, 0), state[1][0]);
			state[2][0] = XOR(_mm256_setr_epi64x(data[2], 0, 0, 0), state[2][0]);
			state[3][0] = XOR(_mm256_setr_epi64x(data[3], 0, 0, 0), state[3][0]);
			state[4][0] = XOR(_mm256_setr_epi64x(data[4], 0, 0, 0), state[4][0]);

			p4(state);
		}

		//Handle last upto 40 bytes of data
		int shouldPadCapacity = load_data_into_u64(ad, adlen, data, &progress);
		if (shouldPadCapacity) {
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(state[i][0], _mm256_setr_epi64x(0, 0xFF, 0, 0));
			}
		}
		state[0][0] = XOR(_mm256_setr_epi64x(data[0], 0, 0, 0), state[0][0]);
		state[1][0] = XOR(_mm256_setr_epi64x(data[1], 0, 0, 0), state[1][0]);
		state[2][0] = XOR(_mm256_setr_epi64x(data[2], 0, 0, 0), state[2][0]);
		state[3][0] = XOR(_mm256_setr_epi64x(data[3], 0, 0, 0), state[3][0]);
		state[4][0] = XOR(_mm256_setr_epi64x(data[4], 0, 0, 0), state[4][0]);

		p1(state);
	}
}

void crypto_aead_encrypt(
	u8 *c,
	const u8 *m, const u64 mlen,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u8 *tag) {

	YMM state[5][2];
	YMM key[5];

	//common start-steps between encrypt and decrypt
	initialize_common(state, k, nonce, key, adlen, ad);

	u64 progress = 0;
	u64 data_u64[5];
	while (progress < mlen) {
		//Get next 40 bytes of data
		int shouldPadCapacity = load_data_into_u64(m, mlen, data_u64, &progress);
		if (shouldPadCapacity) {
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(state[i][0], _mm256_setr_epi64x(0, 0xFF, 0, 0));
			}
		}

		//Load it into registers and create ciphertext and new state_rate by XOR v_r with message.
		for (int i = 0; i < 5; i++) {
			state[i][0] = XOR(_mm256_setr_epi64x(data_u64[i], 0, 0, 0), state[i][0]);
		}

		//Extract ciphertext
		u64 c0 = _mm256_extract_epi64(state[0][0], 0);
		u64 c1 = _mm256_extract_epi64(state[1][0], 0);
		u64 c2 = _mm256_extract_epi64(state[2][0], 0);
		u64 c3 = _mm256_extract_epi64(state[3][0], 0);
		u64 c4 = _mm256_extract_epi64(state[4][0], 0);
		memcpy(&c[progress - 40], &c0, sizeof(u64));
		memcpy(&c[progress - 32], &c1, sizeof(u64));
		memcpy(&c[progress - 24], &c2, sizeof(u64));
		memcpy(&c[progress - 16], &c3, sizeof(u64));
		memcpy(&c[progress - 8], &c4, sizeof(u64));

		p1(state);
	}

	//**Calculate tag**
	//Create single state by XORing all state together. It will be stored kept in the highest bit of each byte, since there is an intrinsic to easily extract the highest bits of each byte.
	YMM XORedStates[5][2];
	for (int i = 0; i < 5; i++) {
		//Two states XORed together
		XORedStates[i][0] = _mm256_xor_si256(state[i][0], _mm256_slli_epi64(state[i][0], 1));
		XORedStates[i][1] = _mm256_xor_si256(state[i][1], _mm256_slli_epi64(state[i][1], 1));

		//Three
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 2));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 2));

		//Four
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 3));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 3));

		//Five
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 4));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 4));

		//Six
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 5));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 5));

		//Seven
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 6));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 6));

		//Eight
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 7));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 7));

		//XOR key to state
		XORedStates[i][0] = XOR(key[i], XORedStates[i][0]);
	}

	//Extract tag-bits
	int tag_bits[5];
	tag_bits[0] = _mm256_movemask_epi8(XORedStates[0][0]);
	tag_bits[1] = _mm256_movemask_epi8(XORedStates[1][0]);
	tag_bits[2] = _mm256_movemask_epi8(XORedStates[2][0]);
	tag_bits[3] = _mm256_movemask_epi8(XORedStates[3][0]);
	tag_bits[4] = _mm256_movemask_epi8(XORedStates[4][0]);

	//Tag is in the bits 8-23 in each of the 5 ints. 
	tag[0] = (tag_bits[0] >> 8) ^ 0xFF;
	tag[1] = (tag_bits[0] >> 16) ^ 0xFF;
	tag[2] = (tag_bits[0] >> 24) ^ 0xFF;

	tag[3] = (tag_bits[1] >> 8) ^ 0xFF;
	tag[4] = (tag_bits[1] >> 16) ^ 0xFF;
	tag[5] = (tag_bits[1] >> 24) ^ 0xFF;

	tag[6] = (tag_bits[2] >> 8) ^ 0xFF;
	tag[7] = (tag_bits[2] >> 16) ^ 0xFF;
	tag[8] = (tag_bits[2] >> 24) ^ 0xFF;

	tag[9] = (tag_bits[3] >> 8) ^ 0xFF;
	tag[10] = (tag_bits[3] >> 16) ^ 0xFF;
	tag[11] = (tag_bits[3] >> 24) ^ 0xFF;

	tag[12] = (tag_bits[4] >> 8) ^ 0xFF;
	tag[13] = (tag_bits[4] >> 16) ^ 0xFF;
	tag[14] = (tag_bits[4] >> 24) ^ 0xFF;
}

int crypto_aead_decrypt(
	u8 *c, u64 clen,
	u8 *m,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u8 *tag) {

	YMM state[5][2];
	YMM key[5];

	//common start-steps between encrypt and decrypt
	initialize_common(state, k, nonce, key, adlen, ad);

	u64 progress = 0;
	u64 data_u64[5];
	YMM dec_m[5];
	while (progress < clen) {

		//Get next 40 bytes of data
		int shouldPadCapacity = load_data_into_u64(c, clen, data_u64, &progress);
		if (shouldPadCapacity) {
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(state[i][0], _mm256_setr_epi64x(0, 0xFF, 0, 0));
			}
		}

		//Load it into registers and create ciphertext and new state_rate by XOR v_r with message.
		for (int i = 0; i < 5; i++) {
			dec_m[i] = XOR(_mm256_setr_epi64x(data_u64[i], _mm256_extract_epi64(state[i][0], 1), _mm256_extract_epi64(state[i][0], 2), _mm256_extract_epi64(state[i][0], 3)), state[i][0]);
			if (progress < clen) {
				__mm256_insert_epi64(state[i][0], data_u64[i], 0);
			}
		}

		//Extract ciphertext
		u64 m0 = _mm256_extract_epi64(dec_m[0], 0);
		u64 m1 = _mm256_extract_epi64(dec_m[1], 0);
		u64 m2 = _mm256_extract_epi64(dec_m[2], 0);
		u64 m3 = _mm256_extract_epi64(dec_m[3], 0);
		u64 m4 = _mm256_extract_epi64(dec_m[4], 0);
		memcpy(&m[progress - 40], &m0, sizeof(u64));
		memcpy(&m[progress - 32], &m1, sizeof(u64));
		memcpy(&m[progress - 24], &m2, sizeof(u64));
		memcpy(&m[progress - 16], &m3, sizeof(u64));
		memcpy(&m[progress - 8], &m4, sizeof(u64));

		//If we are done decrypting, we XOR the last decrypted message with potential padding to the state to prepare for creating the tag.
		if (progress > clen) {
			int clen_last_blocksize = clen % 40;
			int clen_pad_progress = 0;

			for (int i = 0; i < 5; i++) {

				//Do we need to pad this ymm?
				if (clen_last_blocksize >= clen_pad_progress + 8) {
					//No, there are 8 plaintext bytes in it. 
				}
				else {
					//There are a need to pad it.
					//Are any of the bytes in it from the plaintext?
					if (clen_last_blocksize < clen_pad_progress) {
						//No.
						dec_m[i] = _mm256_setzero_si256();
					}
					else {
						//Some of the bytes should be plaintext, some should be padding... How many should be padding?
						int padding_bytes = 8 - (clen_last_blocksize - clen_pad_progress);

						//Shift data to the left to add padding by shifting back afterwards (and insert 0x01 at the front, before shifting back)
						u64 t = _mm256_extract_epi64(dec_m[i], 0);
						t <<= 8 * padding_bytes;
						t >>= 8; //Make space for 0x01 byte
						t |= 0b0000000100000000000000000000000000000000000000000000000000000000;
						t >>= 8 * (padding_bytes - 1); //Shift remaining back.
						__mm256_insert_epi64(dec_m[i], t, 0);
					}
				}
				clen_pad_progress += 8;
				state[i][0] = XOR(state[i][0], dec_m[i]);
			}
		}
		p1(state);
	}

	//**Calculate tag**
	//Create single state by XORing all state together. It will be stored kept in the highest bit of each byte, since there is an intrinsic to easily extract the highest bits of each byte.
	YMM XORedStates[5][2];
	for (int i = 0; i < 5; i++) {
		//Two states XORed together
		XORedStates[i][0] = _mm256_xor_si256(state[i][0], _mm256_slli_epi64(state[i][0], 1));
		XORedStates[i][1] = _mm256_xor_si256(state[i][1], _mm256_slli_epi64(state[i][1], 1));

		//Three
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 2));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 2));

		//Four
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 3));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 3));

		//Five
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 4));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 4));

		//Six
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 5));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 5));

		//Seven
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 6));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 6));

		//Eight
		XORedStates[i][0] = _mm256_xor_si256(XORedStates[i][0], _mm256_slli_epi64(state[i][0], 7));
		XORedStates[i][1] = _mm256_xor_si256(XORedStates[i][1], _mm256_slli_epi64(state[i][1], 7));

		//XOR key to state
		XORedStates[i][0] = XOR(key[i], XORedStates[i][0]);
	}

	//Extract tag-bits
	int tag_bits[5];
	tag_bits[0] = _mm256_movemask_epi8(XORedStates[0][0]);
	tag_bits[1] = _mm256_movemask_epi8(XORedStates[1][0]);
	tag_bits[2] = _mm256_movemask_epi8(XORedStates[2][0]);
	tag_bits[3] = _mm256_movemask_epi8(XORedStates[3][0]);
	tag_bits[4] = _mm256_movemask_epi8(XORedStates[4][0]);

	//Tag is in the bits 8-23 in each of the 5 ints. 
	u8 actual_tag[10];
	actual_tag[0] = (tag_bits[0] >> 8) ^ 0xFF;
	actual_tag[1] = (tag_bits[0] >> 16) ^ 0xFF;
	actual_tag[2] = (tag_bits[0] >> 24) ^ 0xFF;

	actual_tag[3] = (tag_bits[1] >> 8) ^ 0xFF;
	actual_tag[4] = (tag_bits[1] >> 16) ^ 0xFF;
	actual_tag[5] = (tag_bits[1] >> 24) ^ 0xFF;

	actual_tag[6] = (tag_bits[2] >> 8) ^ 0xFF;
	actual_tag[7] = (tag_bits[2] >> 16) ^ 0xFF;
	actual_tag[8] = (tag_bits[2] >> 24) ^ 0xFF;

	actual_tag[9] = (tag_bits[3] >> 8) ^ 0xFF;
	actual_tag[10] = (tag_bits[3] >> 16) ^ 0xFF;
	actual_tag[11] = (tag_bits[3] >> 24) ^ 0xFF;

	actual_tag[12] = (tag_bits[4] >> 8) ^ 0xFF;
	actual_tag[13] = (tag_bits[4] >> 16) ^ 0xFF;
	actual_tag[14] = (tag_bits[4] >> 24) ^ 0xFF;

	for (int i = 0; i < 15; i++) {
		if (actual_tag[i] != tag[i]) {
			memset(m, 0, clen);
			return 1;
		}
	}
	return 0;
}

/*
Progress = progress in bytes.
Loads 40 bytes into 5x u64. 8 bytes per u64
*/
int load_data_into_u64(const u8 *m, u64 mlen, u64 rates[5], u64 *progress) {

	//Are there 40 available bytes? Handle them easy now then.
	if (*progress + 40 <= mlen) {
		memcpy(rates, &m[*progress], sizeof(u8) * 40);
		*progress += 40;

		if (*progress == mlen) {
			return 1;
		}
		return 0;
	}

	//5x u64. 8 bytes each. 40 bytes in total.
	//At some point during the next 40 bytes, we need to pad
	for (int i = 0; i < 5; i++) {

		//Do we need to pad this u64?
		if (mlen >= *progress + 8) {
			//No, there are 8 available bytes. 
			memcpy(&rates[i], &m[*progress], sizeof(u8) * 8);
		}
		else {
			//yes, there are less than 8 available. 
			//Are there any available?
			if (mlen < *progress) {
				//No.
				rates[i] = 0;
			}
			else
			{
				//Yes. How many are there left?
				i64 available_bytes = mlen - *progress;

				//Load remaining bytes into zeroed array (this is needed since we use XOR
				rates[i] = 0x01; //Pad with 1 after the data.
				for (int j = 0; j < available_bytes; j++) {
					rates[i] <<= 8;
					rates[i] |= m[*progress + (available_bytes - 1 - j)];
				}
			}
		}
		*progress += 8;
	}
	return 0;
}

void init_bitslice_state(YMM *key, const u8 *n, const u8 *k, YMM(*state)[2]) {

	//Create bitsliced state. Remember to transpose 1 bit to 1 byte, since we bitslice 8 states at once. We do this via AVX broadcasting.
	int key_bits[5] = { 0 };
	int nonce_bits[5] = { 0 };

	//The low 8 bits are kept zero each time, as the rate should be zero and is stored here. 
	key_bits[0] = (k[0] << 8) | (k[1] << 16) | (k[2] << 24);
	key_bits[1] = (k[3] << 8) | (k[4] << 16) | (k[5] << 24);
	key_bits[2] = (k[6] << 8) | (k[7] << 16) | (k[8] << 24);
	key_bits[3] = (k[9] << 8) | (k[10] << 16) | (k[11] << 24);
	key_bits[4] = (k[12] << 8) | (k[13] << 16) | (k[14] << 24);

	//The high 8 bits are kept zeroed each time, as the size of 8 primate states takes 2240 bits. The first registers uses 1280 bits, so there is 64 bits per register in the second half unused
	//It is more efficient to keep the high bits unused than the lower bits due to unneccesary shifting otherwise.
	nonce_bits[0] = n[0] | (n[1] << 8) | (n[2] << 16);
	nonce_bits[1] = n[3] | (n[4] << 8) | (n[5] << 16);
	nonce_bits[2] = n[6] | (n[7] << 8) | (n[8] << 16);
	nonce_bits[3] = n[9] | (n[10] << 8) | (n[11] << 16);
	nonce_bits[4] = n[12] | (n[13] << 8) | (n[14] << 16);

	//broadcast each of the 32 bits (24 if excluding zeroed space) to a 256bit YMM register. This means that each bit gets broadcast to 8 bits, which is ideal here.
	//Key
	state[0][0] = key[0] = expand_bits_to_bytes(key_bits[0]);
	state[1][0] = key[1] = expand_bits_to_bytes(key_bits[1]);
	state[2][0] = key[2] = expand_bits_to_bytes(key_bits[2]);
	state[3][0] = key[3] = expand_bits_to_bytes(key_bits[3]);
	state[4][0] = key[4] = expand_bits_to_bytes(key_bits[4]);

	//Nonce
	state[0][1] = expand_bits_to_bytes(nonce_bits[0]);
	state[1][1] = expand_bits_to_bytes(nonce_bits[1]);
	state[2][1] = expand_bits_to_bytes(nonce_bits[2]);
	state[3][1] = expand_bits_to_bytes(nonce_bits[3]);
	state[4][1] = expand_bits_to_bytes(nonce_bits[4]);
}

YMM expand_bits_to_bytes(int x)
{
	// we only use the low 32bits of each lane, but this is fine with AVX2
	__m256i xbcast = _mm256_set1_epi32(x);    

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