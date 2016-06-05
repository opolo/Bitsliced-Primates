#pragma once
#include <immintrin.h>

typedef __m256i YMM;

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef long long i64;

//Bit-wise operations on AVX registers
#define XOR(a, b) _mm256_xor_si256(a, b)
#define XOR3(a, b, c) _mm256_xor_si256(a, _mm256_xor_si256(b, c))
#define XOR4(a, b, c, d) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, d)))
#define XOR5(a, b, c, d, e) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, e))))
#define XOR7(a, b, c, d, e, f, g) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, g))))))
#define AND(a, b) _mm256_and_si256(a, b)
#define NEG(a) _mm256_xor_si256(m256iAllOne, a)

//Below is for Gibbon-120
#define size_rate_bytes 5
#define size_capacity_bytes 30
#define size_key_bytes 15
#define size_nonce_bytes 15
#define size_key_and_nonce_bytes 15
#define state_row_count 7

//Primate parameters
#define OneBitsAtCol2 0b0000000011111111000000000000000000000000000000000000000000000000ULL
#define p1_rounds 12
#define p2_rounds 6
#define p3_rounds 6

#define Debug 0
#define OutputData 0
#define Benchmark 1
#define DisablePrimates 0