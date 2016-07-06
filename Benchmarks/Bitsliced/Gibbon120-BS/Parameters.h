#ifndef Parameters

#include <immintrin.h>
#include <stdio.h>

typedef __m256i YMM;
typedef unsigned char u8;
typedef unsigned long long u64;
typedef long long i64;

//For the MSVC compiler, since it do not support extract/insert intrinsics.
#if (_MSC_VER == 1900)
#define _mm256_extract_epi64(a, b) a.m256i_u64[b]
#define _mm256_extract_epi8(a, b) a.m256i_u8[b]
#define __mm256_insert_epi64(a, value, index) a.m256i_u64[index] = value
#define __mm256_insert_epi8(a, value, index) a.m256i_u8[index] = value
#else
#define __mm256_insert_epi64(a, value, index) a = _mm256_insert_epi64(a, value, index)
#define __mm256_insert_epi8(a, value, index) a = _mm256_insert_epi8(a, value, index)
#endif

//Bit-wise operations on AVX registers
#define XOR(a, b) _mm256_xor_si256(a, b)
#define NEG(a) _mm256_xor_si256(m256iAllOne, a)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR3(a, b, c) _mm256_xor_si256(a, _mm256_xor_si256(b, c))
#define XOR4(a, b, c, d) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, d)))
#define XOR5(a, b, c, d, e) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, e))))
#define XOR6(a, b, c, d, e, f) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, f)))))
#define XOR7(a, b, c, d, e, f, g) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, g))))))
#define XOR8(a, b, c, d, e, f, g, h) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, _mm256_xor_si256(g, h)))))))
#define XOR9(a, b, c, d, e, f, g, h, i) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, _mm256_xor_si256(g, _mm256_xor_si256(h, i))))))))
#define XOR10(a, b, c, d, e, f, g, h, i, j) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, _mm256_xor_si256(g, _mm256_xor_si256(h, _mm256_xor_si256(i, j)))))))))
#define AND(a, b) _mm256_and_si256(a, b)
#define AND3(a, b, c) _mm256_and_si256(a, _mm256_and_si256(b, c)
#define AND4(a, b, c, d) _mm256_and_si256(a, _mm256_and_si256(b,  _mm256_and_si256(c, d)))
#define AND5(a, b, c, d, e) _mm256_and_si256(a,  _mm256_and_si256(b,  _mm256_and_si256(c,  _mm256_and_si256(d, e))))
#define AND6(a, b, c, d, e, f) _mm256_and_si256(a,  _mm256_and_si256(b,  _mm256_and_si256(c,  _mm256_and_si256(d, _mm256_and_si256(e, f)))))
#define AND7(a, b, c, d, e, f, g) _mm256_and_si256(a, _mm256_and_si256(b,  _mm256_and_si256(c,  _mm256_and_si256(d, _mm256_and_si256(e, _mm256_and_si256(f, g))))))
#define AND8(a, b, c, d, e, f, g, h) _mm256_and_si256(a, _mm256_and_si256(b,  _mm256_and_si256(c,  _mm256_and_si256(d, _mm256_and_si256(e, _mm256_and_si256(f, _mm256_and_si256(g, h)))))))
#define AND9(a, b, c, d, e, f, g, h, i) _mm256_and_si256(a, _mm256_and_si256(b,  _mm256_and_si256(c,  _mm256_and_si256(d, _mm256_and_si256(e, _mm256_and_si256(f, _mm256_and_si256(g, _mm256_and_si256(h, i))))))))


#define p1_rounds 12
#define p2_rounds 6
#define p3_rounds 6

#define Debug 1
#define Verbose 0
#define Benchmark 1
#define DisablePrimates 0

#endif // !Parameters