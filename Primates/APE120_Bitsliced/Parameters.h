#pragma once

#define PrimateRounds 1 // p1
#define keyLength 48 //primate elements of 5 bit
#define NonceLength 24 //primate elements of 5 bit
#define RateSize 8 //primate elements of 5 bit
#define CapacitySize 48


#define false 0
#define true 1
#define bool unsigned char

#define XOR(a, b) _mm256_xor_si256(a, b)
#define AND(a, b) _mm256_and_si256(a, b)
#define NEG(a) _mm256_xor_si256(m256iAllOne, a)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR3(a, b, c) _mm256_xor_si256(a, _mm256_xor_si256(b, c))
#define XOR4(a, b, c, d) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, d)))
