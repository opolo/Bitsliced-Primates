#include "Encrypt.h"
#include "Primate.h"
#include <stdio.h>
#include <string.h>

void create_key_YMM(const u8 *k, YMM(*key)[2]);
int load_data_into_u64(const u8 *m, u64 mlen, u64 rates[5], u64 *progress);
YMM expand_bits_to_bytes(int x);

void crypto_aead_encrypt(
	u8 *c,
	const u8 *m, const u64 mlen,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u64 *tag) {


	YMM state[5][2];
	YMM key[5][2];
	_mm256_zeroall(); //Makes sure all the just declared regs are 0.

	//XOR key to empty state... It will only be stored in the capacity
	create_key_YMM(k, key);
	for (int i = 0; i < 5; i++) {
		state[i][0] = key[i][0];
		state[i][1] = key[i][1];
	}

	//Add a different constant to first bit of each rate (identically to how primate permutations adds constants) to avoid ECB'esque problems... Constants chosen: 01, 02, 05, 0a, 15, 0b, 17, 0e, 
	state[0][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000001010111000000000), state[0][0]);
	state[1][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000101011100000000), state[1][0]);
	state[2][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000010101100000000), state[2][0]);
	state[3][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000001010100000000), state[3][0]);
	state[4][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000000101000000000), state[4][0]);

	p1(state);

	//Handle nonce (40 byte in my implementation)
	{
		u64 data_u64[5] = { 0 };
		u64 progress = 0;
		load_data_into_u64(nonce, 30, data_u64, &progress);
		state[0][0] = XOR(state[0][0], _mm256_setr_epi64x(data_u64[0], 0, 0, 0));
		state[1][0] = XOR(state[1][0], _mm256_setr_epi64x(data_u64[1], 0, 0, 0));
		state[2][0] = XOR(state[2][0], _mm256_setr_epi64x(data_u64[2], 0, 0, 0));
		state[3][0] = XOR(state[3][0], _mm256_setr_epi64x(data_u64[3], 0, 0, 0));
		state[4][0] = XOR(state[4][0], _mm256_setr_epi64x(data_u64[4], 0, 0, 0));
		p1(state);

	}

	//Handle potential AD.
	if (adlen) {
		u64 progress = 0;
		u64 ad_u64[5] = { 0 };
		while (progress < adlen) {

			//Get next 40 bytes of data
			int shouldPadCapacity = load_data_into_u64(ad, adlen, ad_u64, &progress);
			if (shouldPadCapacity) {
				for (int i = 0; i < 5; i++) {
					state[i][0] = XOR(state[i][0], _mm256_setr_epi64x(0, 0xFF, 0, 0));
				}
			}

			//Load it into registers and create and do p1
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(_mm256_setr_epi64x(ad_u64[i], 0, 0, 0), state[i][0]);
			}

			p1(state);
		}
	}


	//V <= V XOR (0^(b-1) || 1)
	state[4][1] = _mm256_xor_si256(state[4][1], _mm256_setr_epi64x(0, 0, 0b1111111100000000000000000000000000000000000000000000000000000000, 0));
	
	if (mlen) {
		u64 progress = 0;
		u64 data_u64[5] = { 0 };
		while (progress < mlen) {

			//Get next 40 bytes of data
			int shouldPadCapacity = load_data_into_u64(m, mlen, data_u64, &progress);
			if (shouldPadCapacity) {
				for (int i = 0; i < 5; i++) {
					state[i][0] = XOR(state[i][0], _mm256_setr_epi64x(0, 0xFF, 0, 0));
				}
			}

			//XOR it into registers and do p1
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(_mm256_setr_epi64x(data_u64[i], 0, 0, 0), state[i][0]);
			}
			
			p1(state);

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
		}
	}


	//Create tag (its size will be 240*8 = 1920 bits = 240 bytes.
	//XOR key to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR(state[i][0], key[i][0]);
		state[i][1] = XOR(state[i][1], key[i][1]);
	}

	//Extract tag
	tag[0] = _mm256_extract_epi64(state[0][0], 1);
	tag[1] = _mm256_extract_epi64(state[0][0], 2);
	tag[2] = _mm256_extract_epi64(state[0][0], 3);
	tag[3] = _mm256_extract_epi64(state[0][1], 0);
	tag[4] = _mm256_extract_epi64(state[0][1], 1);
	tag[5] = _mm256_extract_epi64(state[0][1], 2);
		    
	tag[6] = _mm256_extract_epi64(state[1][0], 1);
	tag[7] = _mm256_extract_epi64(state[1][0], 2);
	tag[8] = _mm256_extract_epi64(state[1][0], 3);
	tag[9] = _mm256_extract_epi64(state[1][1], 0);
	tag[10] = _mm256_extract_epi64(state[1][1], 1);
	tag[11] = _mm256_extract_epi64(state[1][1], 2);
		   
	tag[12] = _mm256_extract_epi64(state[2][0], 1);
	tag[13] = _mm256_extract_epi64(state[2][0], 2);
	tag[14] = _mm256_extract_epi64(state[2][0], 3);
	tag[15] = _mm256_extract_epi64(state[2][1], 0);
	tag[16] = _mm256_extract_epi64(state[2][1], 1);
	tag[17] = _mm256_extract_epi64(state[2][1], 2);
		    
	tag[18] = _mm256_extract_epi64(state[3][0], 1);
	tag[19] = _mm256_extract_epi64(state[3][0], 2);
	tag[20] = _mm256_extract_epi64(state[3][0], 3);
	tag[21] = _mm256_extract_epi64(state[3][1], 0);
	tag[22] = _mm256_extract_epi64(state[3][1], 1);
	tag[23] = _mm256_extract_epi64(state[3][1], 2);

	tag[24] = _mm256_extract_epi64(state[4][0], 1);
	tag[25] = _mm256_extract_epi64(state[4][0], 2);
	tag[26] = _mm256_extract_epi64(state[4][0], 3);
	tag[27] = _mm256_extract_epi64(state[4][1], 0);
	tag[28] = _mm256_extract_epi64(state[4][1], 1);
	tag[29] = _mm256_extract_epi64(state[4][1], 2);
}

int crypto_aead_decrypt(
	u8 *c, const u64 clen,
	u8 *m,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u64 *tag) {

	u64 paddedClen = 0;
	//was message fractional?
	if (clen % 40 != 0) {
		//Yes.
		int remaining = clen % 40;
		paddedClen = clen + (40 - remaining);
	}
	else {
		//No.
		paddedClen = clen;
	}

	YMM state_IV[5][2];
	YMM state_V[5][2];
	YMM key[5][2];
	_mm256_zeroall(); //Makes sure all the just declared regs are 0.

	//XOR key to empty state
	create_key_YMM(k, key);
	for (int i = 0; i < 5; i++) {
		state_IV[i][0] = key[i][0];
		state_IV[i][1] = key[i][1];
	}

	//Add a different constant to second element of each rate (identically to how primate permutations adds constants, but for the rate) to avoid ECB'esque problems... Constants chosen: 01, 02, 05, 0a, 15, 0b, 17, 0e, 
	state_IV[0][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000001010111000000000), state_IV[0][0]);
	state_IV[1][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000101011100000000), state_IV[1][0]);
	state_IV[2][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000010101100000000), state_IV[2][0]);
	state_IV[3][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000001010100000000), state_IV[3][0]);
	state_IV[4][0] = XOR(_mm256_set_epi64x(0, 0, 0, 0b0000000000000000000000000000000000000000000000000000101000000000), state_IV[4][0]);

	p1(state_IV);

	//Handle nonce (40 byte in my implementation)
	{
		u64 nonce_u64[5] = { 0 };
		u64 progress = 0;
		load_data_into_u64(nonce, 30, nonce_u64, &progress);
		state_IV[0][0] = XOR(state_IV[0][0], _mm256_setr_epi64x(nonce_u64[0], 0, 0, 0));
		state_IV[1][0] = XOR(state_IV[1][0], _mm256_setr_epi64x(nonce_u64[1], 0, 0, 0));
		state_IV[2][0] = XOR(state_IV[2][0], _mm256_setr_epi64x(nonce_u64[2], 0, 0, 0));
		state_IV[3][0] = XOR(state_IV[3][0], _mm256_setr_epi64x(nonce_u64[3], 0, 0, 0));
		state_IV[4][0] = XOR(state_IV[4][0], _mm256_setr_epi64x(nonce_u64[4], 0, 0, 0));
		p1(state_IV);
	}
	
	//Handle potential AD.
	if (adlen) {
		u64 progress = 0;
		u64 ad_u64[5] = { 0 };
		while (progress < adlen) {

			//Get next 40 bytes of data
			int shouldPadCapacity = load_data_into_u64(ad, adlen, ad_u64, &progress);

			if (shouldPadCapacity) {
				for (int i = 0; i < 5; i++) {
					state_IV[i][0] = XOR(state_IV[i][0], _mm256_setr_epi64x(0, 0xFF, 0, 0));
				}
			}

			//Load it into registers and create and do p1
			for (int i = 0; i < 5; i++) {
				state_IV[i][0] = XOR(_mm256_setr_epi64x(ad_u64[i], 0, 0, 0), state_IV[i][0]);
			}

			p1(state_IV);

		}
	}

	if (clen) {
		//V <= p1^-1(C[w] || K XOR T)
		//M[w] <= v_r XOR C[w-1]
		//V <= V XOR M[w]10* || 0^c
		{

			//V <= p1^-1(C[w] || K XOR T)
			u64 progress = paddedClen - 40;

			u64 lastMLocation = progress;
			u64 cipherblock[5] = { 0 };
			load_data_into_u64(c, paddedClen, cipherblock, &progress);

			for (int i = 0; i < 5; i++) {
				state_V[i][0] = XOR3(_mm256_setr_epi64x(cipherblock[i], 0, 0, 0),
					key[i][0],
					_mm256_setr_epi64x(0, tag[i * 6], tag[(i * 6) + 1], tag[(i * 6) + 2]));
				state_V[i][1] = XOR(key[i][1],
					_mm256_setr_epi64x(tag[(i * 6) + 3], tag[(i * 6) + 4], tag[(i * 6) + 5], 0));
			}
			p1_inv(state_V);

			//M[w] <= v_r XOR C[w-1]
			YMM plaintext_YMM[5];
			for (int i = 0; i < 5; i++) {
				//If progress is not lastMLocation, it means that we should not use C[0] here.
				if (lastMLocation == 0) {
					//Its 0. Use IV_r
					plaintext_YMM[i] = XOR(state_V[i][0], state_IV[i][0]);
				}
				else {
					//Its not zero, use secondlast cipherblock
					lastMLocation -= 40;
					load_data_into_u64(c, paddedClen, cipherblock, &lastMLocation);
					plaintext_YMM[i] = XOR(state_V[i][0], _mm256_setr_epi64x(cipherblock[i], 0, 0, 0));
				}

			}

			u64 m0 = _mm256_extract_epi64(plaintext_YMM[0], 0);
			u64 m1 = _mm256_extract_epi64(plaintext_YMM[1], 0);
			u64 m2 = _mm256_extract_epi64(plaintext_YMM[2], 0);
			u64 m3 = _mm256_extract_epi64(plaintext_YMM[3], 0);
			u64 m4 = _mm256_extract_epi64(plaintext_YMM[4], 0);
			memcpy(&m[progress - 40], &m0, sizeof(u64));
			memcpy(&m[progress - 32], &m1, sizeof(u64));
			memcpy(&m[progress - 24], &m2, sizeof(u64));
			memcpy(&m[progress - 16], &m3, sizeof(u64));
			memcpy(&m[progress - 8], &m4, sizeof(u64));

			//V <= V XOR M[w]10* || 0^c
			int isFractional = clen % 40;
			for (int i = 0; i < 5; i++) {
				if (isFractional) {
					state_V[i][0] = XOR(_mm256_setr_epi64x(_mm256_extract_epi64(plaintext_YMM[i], 0), 0, 0, 0), state_V[i][0]);
				}
				else {
					state_V[i][0] = XOR(_mm256_setr_epi64x(_mm256_extract_epi64(plaintext_YMM[i], 0), 0xFF, 0, 0), state_V[i][0]);
				}
			}
		}


		for (i64 i = paddedClen - 80; i >= 0; i -= 40) {
			i64 progress = i;
			u64 data_u64[5] = { 0 };
			YMM plaintext_YMM[5];

			p1_inv(state_V);

			//Get next 40 bytes of data, unless we are at "the last cipherblock", then we need to use C[0] (which is the rate of state_IV) at this point. 
			if (i == 0) {
				for (int j = 0; j < 5; j++) {
					data_u64[j] = _mm256_extract_epi64(state_IV[j][0], 0);
				}
				progress += 40;
			}
			else {
				progress -= 40;
				load_data_into_u64(c, paddedClen, data_u64, &progress); //will increment progress with 40
				progress += 40;
			}

			for (int i = 0; i < 5; i++) {
				//M[i] <= C[i-1] XOR V_r
				plaintext_YMM[i] = XOR(state_V[i][0], _mm256_setr_epi64x(data_u64[i], 0, 0, 0));

				//V <= C[i-1] || VC
				_mm256_insert_epi64(state_V[i][0], data_u64[i], 0);
			}


			//Extract ciphertext
			u64 m0 = _mm256_extract_epi64(plaintext_YMM[0], 0);
			u64 m1 = _mm256_extract_epi64(plaintext_YMM[1], 0);
			u64 m2 = _mm256_extract_epi64(plaintext_YMM[2], 0);
			u64 m3 = _mm256_extract_epi64(plaintext_YMM[3], 0);
			u64 m4 = _mm256_extract_epi64(plaintext_YMM[4], 0);
			memcpy(&m[progress - 40], &m0, sizeof(u64));
			memcpy(&m[progress - 32], &m1, sizeof(u64));
			memcpy(&m[progress - 24], &m2, sizeof(u64));
			memcpy(&m[progress - 16], &m3, sizeof(u64));
			memcpy(&m[progress - 8], &m4, sizeof(u64));
		}
	}
	else {
		//No msg given... maybe only AD (or no AD), but we still need to verify tag.
		for (int i = 0; i < 5; i++) {
			state_V[i][0] = XOR(key[i][0], _mm256_setr_epi64x(0, tag[i * 6], tag[(i * 6) + 1], tag[(i * 6) + 2]));
			state_V[i][1] = XOR(key[i][1], _mm256_setr_epi64x(tag[(i * 6) + 3], tag[(i * 6) + 4], tag[(i * 6) + 5], 0));
		}
	}

	//Check "tag", IV_c == V_c XOR 0^c-1 || 1
	state_V[4][1] = _mm256_xor_si256(state_V[4][1], _mm256_setr_epi64x(0, 0, 0b1111111100000000000000000000000000000000000000000000000000000000, 0));

	for (int i = 0; i < 5; i++) {
		YMM isEqualSec0 = _mm256_cmpeq_epi64(state_IV[i][0], state_V[i][0]);
		YMM isEqualSec1 = _mm256_cmpeq_epi64(state_IV[i][1], state_V[i][1]);

		//We only compare capacities - not rates, so clean rates. 
		//Also ignore last u64 of second section. The inv_p1 sbox is also applied to this section and turns the 0's into 1's here. Normally shift-rows clear it, but not in the inverse operation 
		//It is cheaper to just clean it here, than to do it everytime in the inverse operation...
		_mm256_insert_epi64(isEqualSec0, 0, 0);
		_mm256_insert_epi64(isEqualSec1, 0, 3);


		if (_mm256_extract_epi64(isEqualSec0, 1) == 0 || _mm256_extract_epi64(isEqualSec0, 2) == 0 || _mm256_extract_epi64(isEqualSec0, 3) == 0 ||
			_mm256_extract_epi64(isEqualSec1, 0) == 0 || _mm256_extract_epi64(isEqualSec1, 1) == 0 || _mm256_extract_epi64(isEqualSec1, 2) == 0) {
			//Not equal. Wipe data and 
			//memset to 0 here
			return 1;
		}
	}
	return 0;
	
}

void create_key_YMM(const u8 *k, YMM(*key)[2]) {
	
	//Expand key
	int key_bits[10] = { 0 };

	//First register section
	//The low 8 bits are kept zero each time, as the rate should be zero and is stored here. 
	key_bits[0] = (k[0] << 8) | (k[1] << 16) | (k[2] << 24);
	key_bits[1] = (k[3] << 8) | (k[4] << 16) | (k[5] << 24);
	key_bits[2] = (k[6] << 8) | (k[7] << 16) | (k[8] << 24);
	key_bits[3] = (k[9] << 8) | (k[10] << 16) | (k[11] << 24);
	key_bits[4] = (k[12] << 8) | (k[13] << 16) | (k[14] << 24);

	//Second register section
	//The high 8 bits are kept zeroed each time, as the size of 8 primate states takes 2240 bits. The first registers uses 1280 bits, so there is 64 bits per register in the second half unused
	//It is more efficient to keep the high bits unused than the lower bits due to unneccesary shifting otherwise.
	key_bits[5] = k[15] | (k[16] << 8) | (k[17] << 16);
	key_bits[6] = k[18] | (k[19] << 8) | (k[20] << 16);
	key_bits[7] = k[21] | (k[22] << 8) | (k[23] << 16);
	key_bits[8] = k[24] | (k[25] << 8) | (k[26] << 16);
	key_bits[9] = k[27] | (k[28] << 8) | (k[29] << 16);

	//broadcast each of the 32 bits (24 if excluding zeroed space) to a 256bit YMM register. This means that each bit gets broadcast to 8 bits, which is ideal here.
	//Key
	key[0][0] = expand_bits_to_bytes(key_bits[0]);
	key[1][0] = expand_bits_to_bytes(key_bits[1]);
	key[2][0] = expand_bits_to_bytes(key_bits[2]);
	key[3][0] = expand_bits_to_bytes(key_bits[3]);
	key[4][0] = expand_bits_to_bytes(key_bits[4]);
	key[0][1] = expand_bits_to_bytes(key_bits[5]);
	key[1][1] = expand_bits_to_bytes(key_bits[6]);
	key[2][1] = expand_bits_to_bytes(key_bits[7]);
	key[3][1] = expand_bits_to_bytes(key_bits[8]);
	key[4][1] = expand_bits_to_bytes(key_bits[9]);
}


/*
Progress = progress in bytes.
Loads 40 bytes into 5x u64. 8 bytes per u64
Increments progress with 40 each time is has loaded 40 bytes (i.e. been called)

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
				rates[i] = 0xFF; //Pad with 1 after the data (we need to pad with 0xFF and not just 1, so that the padding is applied to each state to have a proper separation).
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

