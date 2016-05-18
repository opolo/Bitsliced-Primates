#pragma once
#include "Parameters.h"
#include <immintrin.h>
typedef unsigned long long u64;

void transpose_nonce_to_rate_u64(const unsigned char n[4][NonceLength], u64 transposedNonce[5][4][3]);
void transpose_data_to_ratesize_u64(const unsigned char *data[4], u64 dataLen, u64 transposedData[5][4], u64 dataTransposedProgress);
void primates120_decrypt(const unsigned char k[4][KeyLength],
	const unsigned char *ciphertexts[4], u64 cLen,
	const unsigned char *ad[4], u64 adlen,
	const unsigned char npub[4][NonceLength],
	unsigned char *m[4],
	unsigned char tag[4][KeyLength]);

void primate(__m256i *states);
void p1_inv(__m256i *states);

void test_rate_transpose();
void test_capacity_transpose();



void detranspose_capacity_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][CapacitySize]);
void detranspose_rate_to_bytes(__m256i *YMMs, unsigned char detransposedBytes[4][RateSize]);

void transpose_key_to_capacity_u64(const unsigned char k[4][KeyLength], u64 transposedKey[5][4]);

