#pragma once

#define PrimateRounds 12
#define keyLength 30 //Bytes = 240 bits
#define NonceLength 15 //Bytes

typedef unsigned long long u64;

void transpose_key_to_u64(const unsigned char *k, u64 transposedKey[5]);