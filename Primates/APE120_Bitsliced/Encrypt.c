#include "Encrypt.h"
#include "Primate.h"
#include <stdio.h>
#include <string.h>

void create_key_YMM(const u8 *k, YMM(*key)[2]);
void load_data_into_u64(u8 *m, u64 mlen, u64 rates[5], u64 *progress);
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

	//XOR key to empty state
	create_key_YMM(k, key);
	for (int i = 0; i < 5; i++) {
		state[i][0] = key[i][0];
		state[i][1] = key[i][1];
	}

	//Add a different constant to each capacity (identically to how primate permutations adds constants) to avoid ECB'esque problems... Constants chosen: 01, 02, 05, 0a, 15, 0b, 17, 0e, 
	state[0][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'1010'1110'00000000, 0), state[0][0]);
	state[1][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0101'0111'00000000, 0), state[1][0]);
	state[2][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0010'1011'00000000, 0), state[2][0]);
	state[3][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0001'0101'00000000, 0), state[3][0]);
	state[4][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0000'1010'00000000, 0), state[4][0]);

	//Handle nonce (40 byte in my implementation)
	{
		u64 nonce_u64[5] = { 0 };
		int progress = 0;
		load_data_into_u64(nonce, 30, nonce_u64, &progress);
		state[0][0] = XOR(state[0][0], _mm256_setr_epi64x(nonce_u64[0], 0, 0, 0));
		state[1][0] = XOR(state[1][0], _mm256_setr_epi64x(nonce_u64[1], 0, 0, 0));
		state[2][0] = XOR(state[2][0], _mm256_setr_epi64x(nonce_u64[2], 0, 0, 0));
		state[3][0] = XOR(state[3][0], _mm256_setr_epi64x(nonce_u64[3], 0, 0, 0));
		state[4][0] = XOR(state[4][0], _mm256_setr_epi64x(nonce_u64[4], 0, 0, 0));
		p1(state);

	}

	//Handle potential AD.
	if (adlen) {
		u64 progress = 0;
		u64 ad_u64[5] = { 0 };
		while (progress < adlen) {

			//Get next 40 bytes of data
			load_data_into_u64(ad, adlen, ad_u64, &progress);

			//Load it into registers and create and do p1
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(_mm256_setr_epi64x(ad_u64[i], 0, 0, 0), state[i][0]);
			}

			p1(state);
		}
	}


	//WHAT IS THE POINT OF THIS? 
	//V <= V XOR (0^(b-1) || 1)
	state[4][1] = _mm256_xor_si256(state[4][1], _mm256_setr_epi64x(0, 0, 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'00000000, 0));
	
	if (mlen) {
		u64 progress = 0;
		u64 data_u64[5] = { 0 };
		while (progress < mlen) {

			//Get next 40 bytes of data
			load_data_into_u64(m, mlen, data_u64, &progress);
			
			//XOR it into registers and do p1
			for (int i = 0; i < 5; i++) {
				state[i][0] = XOR(_mm256_setr_epi64x(data_u64[i], 0, 0, 0), state[i][0]);
			}
			
			p1(state);

			//Extract ciphertext
			memcpy(&c[progress - 40], &state[0][0].m256i_u64[0], sizeof(u64));
			memcpy(&c[progress - 32], &state[1][0].m256i_u64[0], sizeof(u64));
			memcpy(&c[progress - 24], &state[2][0].m256i_u64[0], sizeof(u64));
			memcpy(&c[progress - 16], &state[3][0].m256i_u64[0], sizeof(u64));
			memcpy(&c[progress - 8], &state[4][0].m256i_u64[0], sizeof(u64));
		}
	}


	//Create tag (its size will be 240*8 = 1920 bits = 240 bytes.
	//XOR key to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR(state[i][0], key[i][0]);
		state[i][1] = XOR(state[i][1], key[i][1]);
	}

	//Extract tag  
	memcpy(&tag[0], &state[0][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[1], &state[0][0].m256i_u64[2], sizeof(u64));
	memcpy(&tag[2], &state[0][0].m256i_u64[3], sizeof(u64));
	memcpy(&tag[3], &state[0][1].m256i_u64[0], sizeof(u64));
	memcpy(&tag[4], &state[0][1].m256i_u64[1], sizeof(u64));
	memcpy(&tag[5], &state[0][1].m256i_u64[2], sizeof(u64));

	memcpy(&tag[6], &state[1][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[7], &state[1][0].m256i_u64[2], sizeof(u64));
	memcpy(&tag[8], &state[1][0].m256i_u64[3], sizeof(u64));
	memcpy(&tag[9], &state[1][1].m256i_u64[0], sizeof(u64));
	memcpy(&tag[10], &state[1][1].m256i_u64[1], sizeof(u64));
	memcpy(&tag[11], &state[1][1].m256i_u64[2], sizeof(u64));

	memcpy(&tag[12], &state[2][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[13], &state[2][0].m256i_u64[2], sizeof(u64));
	memcpy(&tag[14], &state[2][0].m256i_u64[3], sizeof(u64));
	memcpy(&tag[15], &state[2][1].m256i_u64[0], sizeof(u64));
	memcpy(&tag[16], &state[2][1].m256i_u64[1], sizeof(u64));
	memcpy(&tag[17], &state[2][1].m256i_u64[2], sizeof(u64));

	memcpy(&tag[18], &state[3][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[19], &state[3][0].m256i_u64[2], sizeof(u64));
	memcpy(&tag[20], &state[3][0].m256i_u64[3], sizeof(u64));
	memcpy(&tag[21], &state[3][1].m256i_u64[0], sizeof(u64));
	memcpy(&tag[22], &state[3][1].m256i_u64[1], sizeof(u64));
	memcpy(&tag[23], &state[3][1].m256i_u64[2], sizeof(u64));

	memcpy(&tag[24], &state[4][0].m256i_u64[1], sizeof(u64));
	memcpy(&tag[25], &state[4][0].m256i_u64[2], sizeof(u64));
	memcpy(&tag[26], &state[4][0].m256i_u64[3], sizeof(u64));
	memcpy(&tag[27], &state[4][1].m256i_u64[0], sizeof(u64));
	memcpy(&tag[28], &state[4][1].m256i_u64[1], sizeof(u64));
	memcpy(&tag[29], &state[4][1].m256i_u64[2], sizeof(u64));
}

int crypto_aead_decrypt(
	u8 *c, const u64 clen,
	const u8 *m,
	const u8 *ad, const u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u64 *tag) {

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

	//Add a different constant to each capacity (identically to how primate permutations adds constants) to avoid ECB'esque problems... Constants chosen: 01, 02, 05, 0a, 15, 0b, 17, 0e, 
	state_IV[0][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'1010'1110'00000000, 0), state_IV[0][0]);
	state_IV[1][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0101'0111'00000000, 0), state_IV[1][0]);
	state_IV[2][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0010'1011'00000000, 0), state_IV[2][0]);
	state_IV[3][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0001'0101'00000000, 0), state_IV[3][0]);
	state_IV[4][0] = XOR(_mm256_set_epi64x(0, 0, 0b00000000'00000000'00000000'00000000'00000000'00000000'0000'1010'00000000, 0), state_IV[4][0]);

	//Handle nonce (40 byte in my implementation)
	{
		u64 nonce_u64[5] = { 0 };
		int progress = 0;
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
			load_data_into_u64(ad, adlen, ad_u64, &progress);

			//Load it into registers and create and do p1
			for (int i = 0; i < 5; i++) {
				state_IV[i][0] = XOR(_mm256_setr_epi64x(ad_u64[i], 0, 0, 0), state_IV[i][0]);
			}

			p1(state_IV);

		}
	}

	//V <= p1^-1(C[w] || K XOR T)
	//M[w] <= v_r XOR C[w-1]
	//V <= V XOR M[w]10* || 0^c
	{
		//V <= p1^-1(C[w] || K XOR T)
		u64 progress = clen >= 40 ? clen - 40 : 0;
		u64 lastMLocation = progress;
		u64 cipherblock[5] = { 0 };
		load_data_into_u64(c, clen, cipherblock, &progress);

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
				load_data_into_u64(c, clen, cipherblock, &lastMLocation);
				plaintext_YMM[i] = XOR(state_V[i][0], _mm256_setr_epi64x(cipherblock[i], 0, 0, 0) );
			}
			
		}

		memcpy(&m[progress - 40], &plaintext_YMM[0].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 32], &plaintext_YMM[1].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 24], &plaintext_YMM[2].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 16], &plaintext_YMM[3].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 8], &plaintext_YMM[4].m256i_u64[0], sizeof(u64));
		
		//V <= V XOR M[w]10* || 0^c
		for (int i = 0; i < 5; i++) {
			state_V[i][0] = XOR(_mm256_setr_epi64x(plaintext_YMM[i].m256i_u64[0], 0, 0, 0), state_V[i][0]);
		}
	}
	
	
	for (int i = clen - 80; i >= 0; i -= 40) {
		i64 progress = i;
		u64 data_u64[5] = { 0 };
		YMM plaintext_YMM[5];
		
		p1_inv(state_V);
		
		//Get next 40 bytes of data, unless we are at "the last cipherblock", then we need to use C[0] (which is the rate of state_IV) at this point. 
		if (i == 0) {
			for (int j = 0; j < 5; j++) {
				data_u64[j] = state_IV[j][0].m256i_u64[0];
			}
			progress += 40;
		}
		else {
			progress -= 40;
			load_data_into_u64(c, clen, data_u64, &progress); //will increment progress with 40
			progress += 40;
		}
		
		for (int i = 0; i < 5; i++) {
			//M[i] <= C[i-1] XOR V_r
			plaintext_YMM[i] = XOR(state_V[i][0], _mm256_setr_epi64x(data_u64[i], 0, 0, 0));
			
			//V <= C[i-1] || VC
			state_V[i][0].m256i_u64[0] = data_u64[i];
		}


		//Extract ciphertext
		memcpy(&m[progress - 40], &plaintext_YMM[0].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 32], &plaintext_YMM[1].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 24], &plaintext_YMM[2].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 16], &plaintext_YMM[3].m256i_u64[0], sizeof(u64));
		memcpy(&m[progress - 8], &plaintext_YMM[4].m256i_u64[0], sizeof(u64));
	}

	//Check "tag", IV_c == V_c XOR 0^c-1 || 1
	state_V[4][1] = _mm256_xor_si256(state_V[4][1], _mm256_setr_epi64x(0, 0, 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'00000000, 0));

	for (int i = 0; i < 5; i++) {
		YMM isEqualSec0 = _mm256_cmpeq_epi64(state_IV[i][0], state_V[i][0]);
		YMM isEqualSec1 = _mm256_cmpeq_epi64(state_IV[i][1], state_V[i][1]);

		//We only compare capacities - not rates, so clean rates. 
		//Also ignore last u64 of second section. The inv_p1 sbox is also applied to this section and turns the 0's into 1's here. Normally shift-rows clear it, but not in the inverse operation 
		//It is cheaper to just clean it here, than to do it everytime in the inverse operation...
		isEqualSec0.m256i_u64[0] = 1;
		isEqualSec1.m256i_u64[3] = 1;

		if (isEqualSec0.m256i_u64[1] == 0 || isEqualSec0.m256i_u64[2] == 0 || isEqualSec0.m256i_u64[3] == 0 ||
			isEqualSec1.m256i_u64[1] == 0 || isEqualSec1.m256i_u64[2] == 0 || isEqualSec1.m256i_u64[3] == 0) {
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
*/
void load_data_into_u64(u8 *m, u64 mlen, u64 rates[5], u64 *progress) {

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
				int available_bytes = mlen - *progress;

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














/*


void debitslice_u8(unsigned char bitsliced[5], unsigned char *data);
void transpose_byte_to_u64(unsigned char byte, u64 transposedData[5][4], int offset);
void detransposeDataFromRegister_capacity(__m256i (*state)[2], unsigned char *data);
void detransposeDataFromRegister_rate(__m256i (*state)[2], unsigned char *data);
void transpose_diff_data8x_to_register_XOR_rate(__m256i (*state)[2], unsigned char *data);
void transpose_same_data8x_to_register_XOR_capacity(__m256i (*state)[2], unsigned char *data);
void transpose_same_data8x_to_register_XOR_rate(__m256i (*state)[2], unsigned char *data);

void aead_encrypt(const unsigned char key[KeyLength],
	const unsigned char *m, u64 mlen,
	const unsigned char *ad, u64 adlen,
	const unsigned char nonce[NonceLength],
	unsigned char *ciphertext,
	unsigned char *tag) {

	__m256i YMM_state[5]; //YMM state registers
	__m256i YMM_key[5]; //YMM state registers

	u64 transposedData[5];
}

//Used for Nonce and AD. Transposes the data to the 8 different states in the regs.
void transpose_same_data8x_to_register_XOR_rate(__m256i (*state)[2], unsigned char *data) {
	
	u64 transposedData[5];

	for (int i = 0; i < RateSize; i++) {
		
		//LSB
		if (data[i] & 1)
			transposedData[0] = (transposedData[0] << 8) ^ 0xFF;
		else 
			transposedData[0] = (transposedData[0] << 8);

		//Bit 2
		if (data[i] & 2)
			transposedData[1] = (transposedData[1] << 8) ^ 0xFF;
		else
			transposedData[1] = (transposedData[1] << 8);

		//Bit 3
		if (data[i] & 4)
			transposedData[2] = (transposedData[2] << 8) ^ 0xFF;
		else
			transposedData[2] = (transposedData[2] << 8);

		//Bit 4
		if (data[i] & 8)
			transposedData[3] = (transposedData[3] << 8) ^ 0xFF;
		else
			transposedData[3] = (transposedData[3] << 8);

		//Bit 5
		if (data[i] & 16)
			transposedData[4] = (transposedData[4] << 8) ^ 0xFF;
		else
			transposedData[4] = (transposedData[4] << 8);
	}

	state[0][0] = XOR(state[0][0], _mm256_set_epi64x(transposedData[0], 0, 0, 0));
	state[1][0] = XOR(state[1][0], _mm256_set_epi64x(transposedData[1], 0, 0, 0));
	state[2][0] = XOR(state[2][0], _mm256_set_epi64x(transposedData[2], 0, 0, 0));
	state[3][0] = XOR(state[3][0], _mm256_set_epi64x(transposedData[3], 0, 0, 0));
	state[4][0] = XOR(state[4][0], _mm256_set_epi64x(transposedData[4], 0, 0, 0));
}

//Used for Nonce and AD. Transposes the data to the 8 different states in the regs.
void transpose_same_data8x_to_register_XOR_capacity(__m256i (*state)[2], unsigned char *data) {

	u64 transposedData[5][4];
	memset(transposedData, 0, transposedData[5][4]);

	//First 3 rows (i.e. first group of registers)
	int offset = 0;
	for (int i = 0; i < CapacitySize-24; i++) {

		if (i % 8 == 0)
			offset++;

		transpose_byte_to_u64(data[i], transposedData, offset);
	}

	//Load first group
	state[0][0] = XOR(state[0][0], _mm256_set_epi64x(0, transposedData[0][1], transposedData[0][2], transposedData[0][3]));
	state[1][0] = XOR(state[1][0], _mm256_set_epi64x(0, transposedData[1][1], transposedData[1][2], transposedData[1][3]));
	state[2][0] = XOR(state[2][0], _mm256_set_epi64x(0, transposedData[2][1], transposedData[2][2], transposedData[2][3]));
	state[3][0] = XOR(state[3][0], _mm256_set_epi64x(0, transposedData[3][1], transposedData[3][2], transposedData[3][3]));
	state[4][0] = XOR(state[4][0], _mm256_set_epi64x(0, transposedData[4][1], transposedData[4][2], transposedData[4][3]));

	//Next 3 rows (i.e. second group of registers).
	memset(transposedData, 0, transposedData[5][4]);
	offset = 0;
	for (int i = CapacitySize - 24; i < CapacitySize; i++) {

		if (i % 8 == 0 && i != 0 )
			offset++;

		transpose_byte_to_u64(data[i], transposedData, offset);
	}

	//Load second group
	state[0][1] = XOR(state[0][1], _mm256_set_epi64x(0, transposedData[0][1], transposedData[0][2], transposedData[0][3]));
	state[1][1] = XOR(state[1][1], _mm256_set_epi64x(0, transposedData[1][1], transposedData[1][2], transposedData[1][3]));
	state[2][1] = XOR(state[2][1], _mm256_set_epi64x(0, transposedData[2][1], transposedData[2][2], transposedData[2][3]));
	state[3][1] = XOR(state[3][1], _mm256_set_epi64x(0, transposedData[3][1], transposedData[3][2], transposedData[3][3]));
	state[4][1] = XOR(state[4][1], _mm256_set_epi64x(0, transposedData[4][1], transposedData[4][2], transposedData[4][3]));
}

void transpose_diff_data8x_to_register_XOR_rate(__m256i (*state)[2], unsigned char *data) {
	u64 transposedData[5];

	//Load 8x bitss from different bytes from the data into registers, per iteration
	for (int i = 0; i < RateSize*8; i += 8) {
		
		//LSB
		transposedData[0] = (transposedData[0] << 8) 
			^ (data[i]   & 1 << 7)
			^ (data[i+1] & 1 << 6)
			^ (data[i+2] & 1 << 5)
			^ (data[i+3] & 1 << 4)
			^ (data[i+4] & 1 << 3)
			^ (data[i+5] & 1 << 2)
			^ (data[i+6] & 1 << 1)
			^ (data[i+7] & 1);

		//Bit 2
		transposedData[1] = (transposedData[1] << 8)
			^ (data[i]	   & 2 << 7)
			^ (data[i + 1] & 2 << 6)
			^ (data[i + 2] & 2 << 5)
			^ (data[i + 3] & 2 << 4)
			^ (data[i + 4] & 2 << 3)
			^ (data[i + 5] & 2 << 2)
			^ (data[i + 6] & 2 << 1)
			^ (data[i + 7] & 2);

		//Bit 3
		transposedData[2] = (transposedData[2] << 8)
			^ (data[i]	   & 4 << 7)
			^ (data[i + 1] & 4 << 6)
			^ (data[i + 2] & 4 << 5)
			^ (data[i + 3] & 4 << 4)
			^ (data[i + 4] & 4 << 3)
			^ (data[i + 5] & 4 << 2)
			^ (data[i + 6] & 4 << 1)
			^ (data[i + 7] & 4);

		//Bit 4
		transposedData[3] = (transposedData[3] << 8)
			^ (data[i]	   & 8 << 7)
			^ (data[i + 1] & 8 << 6)
			^ (data[i + 2] & 8 << 5)
			^ (data[i + 3] & 8 << 4)
			^ (data[i + 4] & 8 << 3)
			^ (data[i + 5] & 8 << 2)
			^ (data[i + 6] & 8 << 1)
			^ (data[i + 7] & 8);

		//Bit 5
		transposedData[4] = (transposedData[4] << 8)
			^ (data[i]     & 16 << 7)
			^ (data[i + 1] & 16 << 6)
			^ (data[i + 2] & 16 << 5)
			^ (data[i + 3] & 16 << 4)
			^ (data[i + 4] & 16 << 3)
			^ (data[i + 5] & 16 << 2)
			^ (data[i + 6] & 16 << 1)
			^ (data[i + 7] & 16);

	}

	state[0][0] = XOR(state[0][0], _mm256_set_epi64x(transposedData[0], 0, 0, 0));
	state[1][0] = XOR(state[1][0], _mm256_set_epi64x(transposedData[1], 0, 0, 0));
	state[2][0] = XOR(state[2][0], _mm256_set_epi64x(transposedData[2], 0, 0, 0));
	state[3][0] = XOR(state[3][0], _mm256_set_epi64x(transposedData[3], 0, 0, 0));
	state[4][0] = XOR(state[4][0], _mm256_set_epi64x(transposedData[4], 0, 0, 0));	
}

void detransposeDataFromRegister_rate(__m256i (*state)[2], unsigned char *data) {
	
	for (int i = 0; i < RateSize; i++) {
		unsigned char bitslice[5] = { state[0][0].m256i_u8[i], state[1][0].m256i_u8[i], state[2][0].m256i_u8[i], state[3][0].m256i_u8[i], state[4][0].m256i_u8[i] };

		debitslice_u8(bitslice, &data[i * 8]);
	}
}

void debitslice_u8(unsigned char bitsliced[5], unsigned char *data) {
	
	int mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	for (int i = 0; i < 8; i++){
		data[i] = (bitsliced[0] & mask[i])
				^ ((bitsliced[1] & mask[i]) << 1)
				^ ((bitsliced[2] & mask[i]) << 2)
				^ ((bitsliced[3] & mask[i]) << 3)
				^ ((bitsliced[4] & mask[i]) << 4);
	}


}

void detransposeDataFromRegister_capacity(__m256i (*state)[2], unsigned char *data) {
	
	//First regs (last 3 rows, since first row contains the rate)
	for (int i = 8; i < 32; i++) {
		unsigned char bitslice[5] = { state[0][0].m256i_u8[i], state[1][0].m256i_u8[i], state[2][0].m256i_u8[i], state[3][0].m256i_u8[i], state[4][0].m256i_u8[i] };
		debitslice_u8(bitslice, &data[i * 8]);
	}

	//second regs (first 3 rows, since last row is not used)
	for (int i = 0; i < 24; i++) {
		unsigned char bitslice[5] = { state[0][0].m256i_u8[i], state[1][0].m256i_u8[i], state[2][0].m256i_u8[i], state[3][0].m256i_u8[i], state[4][0].m256i_u8[i] };
		debitslice_u8(bitslice, &data[i * 8]);
	}
}

void transpose_byte_to_u64(unsigned char byte, u64 transposedData[5][4], int offset) {
	//LSB
	if (byte & 1)
		transposedData[0][offset] = (transposedData[0][offset] << 8) ^ 0xFF;
	else
		transposedData[0][offset] = (transposedData[0][offset] << 8);

	//Bit 2
	if (byte & 2)
		transposedData[1][offset] = (transposedData[1][offset] << 8) ^ 0xFF;
	else
		transposedData[1][offset] = (transposedData[1][offset] << 8);

	//Bit 3
	if (byte & 4)
		transposedData[2][offset] = (transposedData[2][offset] << 8) ^ 0xFF;
	else
		transposedData[2][offset] = (transposedData[2][offset] << 8);

	//Bit 4
	if (byte & 8)
		transposedData[3][offset] = (transposedData[3][offset] << 8) ^ 0xFF;
	else
		transposedData[3][offset] = (transposedData[3][offset] << 8);

	//Bit 5
	if (byte & 16)
		transposedData[4][offset] = (transposedData[4][offset] << 8) ^ 0xFF;
	else
		transposedData[4][offset] = (transposedData[4][offset] << 8);
}







/*
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
	YMM[4] = _mm256_xor_si256(YMM[4], temp);

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
	unsigned char test_capacity_before[4][CapacitySize] = { { test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 },{ test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 },
	{ test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 },{ test_cap_data1 test_cap_data2 test_cap_data3 test_cap_data4 } };

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



We need to transpose chunks of the data of sizes that are equal to the rate-part (40 bits) of the primate-cipher.
Thus we transpose 8 primate elements (stored in 8 bytes) at a time.

void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 data_len, u64 transposed_data[5][4], u64 transpose_progress) {

	for (int state_no = 0; state_no < 4; state_no++) {
		//transpose if there are more bytes left
		if ((data_len - transpose_progress) > 0) {
			for (int element = 0; element < 8; element++) {
				transposed_data[0][state_no] = (transposed_data[0][state_no] << 1) | data[state_no][transpose_progress + element] & 1;
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


This functions accepts 4 nonces of length 120 bits ( = 24 primate elements) and transposes these bits
into an array of size n[YMM_no][stateNo][nonce-section]. Each nonce-section is 40 bits and 8 elements long.

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
			transposed_nonce[1][state_no][nonce_sec] = transposed_nonce[1][state_no][nonce_sec] << 48;
			transposed_nonce[2][state_no][nonce_sec] = transposed_nonce[2][state_no][nonce_sec] << 48;
			transposed_nonce[3][state_no][nonce_sec] = transposed_nonce[3][state_no][nonce_sec] << 48;
			transposed_nonce[4][state_no][nonce_sec] = transposed_nonce[4][state_no][nonce_sec] << 48;
		}
	}
}


This function takes an array with 4 capacities of 48 elements and transposes them into a register with the dimensions:
data[register_no][state_no]
void transpose_key_to_capacity_u64(const unsigned char k[4][KeyLength], u64 transposedKey[5][4]) {

//We expect that the key is 240 bits = 48 primate elements (stored in bytes) long.
	for (int state_no = 0; state_no < 4; state_no++) {
		for (int cap_element = 0; cap_element < KeyLength; cap_element++) {
			transposedKey[0][state_no] = (transposedKey[0][state_no] << 1) | k[state_no][cap_element] & 1;
			transposedKey[1][state_no] = (transposedKey[1][state_no] << 1) | (k[state_no][cap_element] >> 1) & 1;
			transposedKey[2][state_no] = (transposedKey[2][state_no] << 1) | (k[state_no][cap_element] >> 2) & 1;
			transposedKey[3][state_no] = (transposedKey[3][state_no] << 1) | (k[state_no][cap_element] >> 3) & 1;
			transposedKey[4][state_no] = (transposedKey[4][state_no] << 1) | (k[state_no][cap_element] >> 4) & 1;
		}
	}
}


/*
void primates120_decrypt(const unsigned char k[4][KeyLength],
	const unsigned char *ciphertexts[4], u64 cLen,
	const unsigned char *ad[4], u64 adlen,
	const unsigned char npub[4][NonceLength],
	unsigned char *m[4],
	unsigned char tag[4][KeyLength])
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
				int offset = (remaining_cipherblock_count - 1) * 8;
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

*/