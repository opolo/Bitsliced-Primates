#pragma once

#define PrimateRounds 12 // p1

#define RateSize 8 //primate elements of 5 bit
#define CapacitySize 48
#define KeyLength 48 // In primate elements
#define NonceLength 24 // In primate elements

#define false 0
#define true 1
#define bool unsigned char



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