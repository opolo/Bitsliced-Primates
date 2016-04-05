#pragma once

#define PrimateRounds 12
#define keyLength 30 //Bytes = 240 bits
#define NonceLength 15 //Bytes

typedef unsigned long long u64;

void primates120_encrypt(const unsigned char *k[4],
	const unsigned char *m[4], u64 mlen[4],
	const unsigned char *ad[4], u64 adlen[4],
	const unsigned char *npub[4]);