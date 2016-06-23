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

	p1(state);

	//V = V_r || (K || 0^(c/2) XOR V_c
	state[0][0] = _mm256_xor_si256(state[0][0], key[0]);
	state[1][0] = _mm256_xor_si256(state[1][0], key[1]);
	state[2][0] = _mm256_xor_si256(state[2][0], key[2]);
	state[3][0] = _mm256_xor_si256(state[3][0], key[3]);
	state[4][0] = _mm256_xor_si256(state[4][0], key[4]);
	
	//Add a different constant to second element of each rate (identically to how primate permutations adds constants, but for the rate) to avoid ECB'esque problems... Constants chosen: 01, 02, 05, 0a, 15, 0b, 17, 0e, 
	state[0][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'1010'1110'00000000), state[0][0]);
	state[1][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0101'0111'00000000), state[1][0]);
	state[2][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0010'1011'00000000), state[2][0]);
	state[3][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0001'0101'00000000), state[3][0]);
	state[4][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0000'1010'00000000), state[4][0]);

	
	//AD
	if (adlen > 0) {
		p2(state);

		u64 progress = 0;
		u64 data[5] = {0};
		while (progress + 40 < adlen) {
		
			//XOR next 40 bytes of data to rate of states;
			load_data_into_u64(ad, adlen, data, &progress);
			state[0][0] = XOR(_mm256_setr_epi64x(data[0], 0, 0, 0), state[0][0]);
			state[1][0] = XOR(_mm256_setr_epi64x(data[1], 0, 0, 0), state[1][0]);
			state[2][0] = XOR(_mm256_setr_epi64x(data[2], 0, 0, 0), state[2][0]);
			state[3][0] = XOR(_mm256_setr_epi64x(data[3], 0, 0, 0), state[3][0]);
			state[4][0] = XOR(_mm256_setr_epi64x(data[4], 0, 0, 0), state[4][0]);

			p2(state);
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
	}

	p3(state);
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
		memcpy(&c[progress - 40], &state[0][0].m256i_u64[0], sizeof(u64));
		memcpy(&c[progress - 32], &state[1][0].m256i_u64[0], sizeof(u64));
		memcpy(&c[progress - 24], &state[2][0].m256i_u64[0], sizeof(u64));
		memcpy(&c[progress - 16], &state[3][0].m256i_u64[0], sizeof(u64));
		memcpy(&c[progress - 8],  &state[4][0].m256i_u64[0], sizeof(u64));
		
		p3(state);
		
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

	//Calculate tag (Actually the last 3 XORs are not needed, as we only need 120 bit for the tag... They could be removed, depending on which bits is used for the tag.
	for (int i = 0; i < 5; i++) {
		state[0][0] = XOR(state[0][0], key[0]);
		state[1][0] = XOR(state[1][0], key[1]);
		state[2][0] = XOR(state[2][0], key[2]);
		state[3][0] = XOR(state[3][0], key[3]);
		state[4][0] = XOR(state[4][0], key[4]);
	}
	
	

	//Extract 120bit tag that is not part of rate..., And to where key was just XORed 
	memcpy(&tag[0], &state[0][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[8], &state[1][0].m256i_u64[1], sizeof(u8)*7);
	
}

int crypto_aead_decrypt(
	u8 *c, u64 clen,
	const u8 *m,
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
			dec_m[i] = XOR(_mm256_setr_epi64x(data_u64[i], state[i][0].m256i_u64[1], state[i][0].m256i_u64[2], state[i][0].m256i_u64[3]), state[i][0]);
			if (progress < clen) {
				state[i][0].m256i_u64[0] = data_u64[i];
			}
		}

		//Extract ciphertext
		memcpy(&m[progress - 40], &dec_m[0].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 32], &dec_m[1].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 24], &dec_m[2].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 16], &dec_m[3].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 8], &dec_m[4].m256i_u64[0], sizeof(u64));

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
						dec_m[i].m256i_u64[0] <<= 8 * padding_bytes;
						dec_m[i].m256i_u64[0] >>= 8; //Make space for 0x01 byte
						dec_m[i].m256i_u64[0] |= 0b00000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
						dec_m[i].m256i_u64[0] >>= 8 * (padding_bytes - 1); //Shift remaining back.
					}
				}
				clen_pad_progress += 8;

				state[i][0] = XOR(state[i][0], dec_m[i]);
			}
		}

		p3(state);
		
	}

	
	//Calculate tag
	//XOR key to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR(key[i], state[i][0]);
	}
	p1(state);

	//XOR key to tag-part of state again. 
	//state[0][0].m256i_u64[1] and 7 first bytes of state[1][0].m256i_u64[1] contains the tag
	state[0][0] = XOR(key[0], state[0][0]);
	state[1][0] = XOR(key[1], state[1][0]);
	state[1][0].m256i_u8[15] = 0x00; //We only compare 120 bits, not 128.



	//Load received parametered tag into registers
	YMM old_tag_0 = _mm256_setr_epi8(
		0, 0, 0, 0, 0, 0, 0, 0,
		tag[0], tag[1], tag[2], tag[3], tag[4], tag[5], tag[6], tag[7],
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0);
	YMM old_tag_1 = _mm256_setr_epi8(
		0, 0, 0, 0, 0, 0, 0, 0,
		tag[8], tag[9], tag[10], tag[11], tag[12], tag[13], tag[14], 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0);

	YMM isEqual0 = _mm256_cmpeq_epi64(state[0][0], old_tag_0);
	YMM isEqual1 = _mm256_cmpeq_epi64(state[1][0], old_tag_1);

	if (isEqual0.m256i_u64[1], isEqual1.m256i_u64[1]) {
		return 0;
	}
	memset(m, 0, clen);
	return 1;
}

/*
Progress = progress in bytes.
Loads 40 bytes into 5x u64. 8 bytes per u64

Returns 1 if last bytes was loaded and message was integral. Else 0.
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
	nonce_bits[0] = n[0]  | (n[1]  << 8) | (n[2]  << 16);
	nonce_bits[1] = n[3]  | (n[4]  << 8) | (n[5]  << 16);
	nonce_bits[2] = n[6]  | (n[7]  << 8) | (n[8]  << 16);
	nonce_bits[3] = n[9]  | (n[10] << 8) | (n[11] << 16);
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