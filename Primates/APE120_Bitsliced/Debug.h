#include "Primates.h"
#include <immintrin.h>
#include <stdio.h>

void print_keys_hex(const unsigned char k[4][keyLength]);
void print_nonces_hex(const unsigned char npub[4][NonceLength]);
void print_ad_hex(const unsigned char *ad[4], u64 adlen[4]);
void print_YMMs(__m256i *YMMs);
void print_state_as_binary(__m256i *states, int state_no);
void byte_to_binary(unsigned char *binarystr, unsigned char byte);