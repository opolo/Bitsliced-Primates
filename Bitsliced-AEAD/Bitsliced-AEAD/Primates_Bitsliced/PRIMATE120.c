//
//  PRIMATE120.c
//  Primates_Bitsliced
//
//  Created by Jonas Bruun Jacobsen on 14/03/16.
//  Copyright © 2016 Jonas Bruun Jacobsen. All rights reserved.
//

#include "PRIMATE120.h"
#include <immintrin.h>

typedef unsigned long long ull;

#define nonceLength 15 //ape120, 120 bits = 15 bytes... Actually does not matter since nonce and AD are treated similarly here.
#define keySize 30 //30 bytes = 240 bits.
#define stateSize 280 //280 bits = 35 bytes = 56 primate state elements.
#define primateRounds 12 //P1

void transpose_key_to_ull(const unsigned char *k, ull transposedKey[5]);
void primate_encrypt(const unsigned char *k[4],
                     const unsigned char *m[4], ull mlen[4],
                     const unsigned char *ad[4], ull adlen[4],
                     const unsigned char *npub[4]);

void primate_encrypt(const unsigned char *k[4],
                     const unsigned char *m[4], ull mlen[4],
                     const unsigned char *ad[4], ull adlen[4],
                     const unsigned char *npub[4]) {
    
    //Declarations
    ull transposedKey[4][5];
    __m256i YMM[5];
    
    //Initialize AVX vectors
    _mm256_zeroall();
    
    //Note: Sinze each YMM is 256bits, each state can have 64bits of aligned space in the register (though it only needs 56 bits).
    //This can conveniently be represented with an unsigned long long, where we fill only the first 56 bits of it.
    
    //XOR keys to states and load into YMM registers.
    for (int keyNo = 0; keyNo < 4; keyNo++) {
        transpose_key_to_ull(k[keyNo], transposedKey[keyNo]);
    }
    for (int YMM_no = 0; YMM_no < 5; YMM_no++){
        YMM[YMM_no] = _mm256_set_epi64x(transposedKey[0][YMM_no], transposedKey[1][YMM_no],
                                        transposedKey[2][YMM_no], transposedKey[4][YMM_no]);
    }
    
}

void primate_p1(__m256i states[5]){
    //SE, SR, MC, CA = ConstAdd, MixCol, ShiftRow, SubEle.
    for (int i = 0; i < primateRounds; i++){
        
        //SR
        _mm256_shu
    }
    
        
    
}

void transpose_key_to_ull(const unsigned char *k, ull transposedKey[5]){

    unsigned char singleByte;
    
    //We expect that the key is 240 bits = 30 bytes long.
    for (int index = 0; index < keySize/8; index += 5 ) {
        
        singleByte = k[index];
        transposedKey[0] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
        transposedKey[1] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
        transposedKey[2] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
        transposedKey[3] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
        transposedKey[4] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
        transposedKey[0] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
        transposedKey[1] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
        transposedKey[2] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit
        
        singleByte = k[index+1];
        transposedKey[3] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
        transposedKey[4] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
        transposedKey[0] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
        transposedKey[1] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
        transposedKey[2] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
        transposedKey[3] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
        transposedKey[4] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
        transposedKey[0] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit
        
        singleByte = k[index+2];
        transposedKey[1] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
        transposedKey[2] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
        transposedKey[3] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
        transposedKey[4] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
        transposedKey[0] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
        transposedKey[1] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
        transposedKey[2] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
        transposedKey[3] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit
        
        singleByte = k[index+3];
        transposedKey[4] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
        transposedKey[0] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
        transposedKey[1] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
        transposedKey[2] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
        transposedKey[3] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
        transposedKey[4] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
        transposedKey[0] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
        transposedKey[1] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit
        
        singleByte = k[index+4];
        transposedKey[2] = (transposedKey[0] << 1) | (singleByte & 1); //first bit
        transposedKey[3] = (transposedKey[1] << 1) | ((singleByte >> 1) & 1);
        transposedKey[4] = (transposedKey[2] << 1) | ((singleByte >> 2) & 1);
        transposedKey[0] = (transposedKey[3] << 1) | ((singleByte >> 3) & 1);
        transposedKey[1] = (transposedKey[4] << 1) | ((singleByte >> 4) & 1);
        transposedKey[2] = (transposedKey[0] << 1) | ((singleByte >> 5) & 1);
        transposedKey[3] = (transposedKey[1] << 1) | ((singleByte >> 6) & 1);
        transposedKey[4] = (transposedKey[2] << 1) | ((singleByte >> 7) & 1); //eight bit
    }
}

void xor_keys_to_YMMs(__m256i YMMs[4]) {
    
}


void xor_states_to_YMMs(){

}



__m256i keys[4];
__m256i states[5]; //5 bits per element, thus 5 vector. This allows for 4 APE120 states of data.



int crypto_aead_encrypt(
                        unsigned char *c,unsigned long long *clen,
                        const unsigned char *m,unsigned long long mlen,
                        const unsigned char *ad,unsigned long long adlen,
                        const unsigned char *nsec,
                        const unsigned char *npub,
                        const unsigned char *k
                        ) {
    
    //Transpose key onto V_c. It should be of size 2s in APE-s, so here 240 bits.
    __m256i initialVector = transpose_key_to_vector(k);
    
    //Handle nonce. It should be of size "s" in APE-s, so here 120bits.
    //The ratesize it should be XOR'ed with is 40 bits, so we need to do it 3 times to get the full nonce added to the state.
    for (int i = 0; i < 4, i++){
        //Add nonce to state with P1
    }
    
    //If associated data is present, we treat it the same way as the nonce above.
    if (adlen != 0){
        //Make sure we do the loop as many times as is needed for the full associated data + add padding if needed.
    }
    
    //Add "0^b-1 || 1" padding over the state.
    //TODO
    
    //Get length of last block of M.
    //TODO
    
    //Transpose M to
}


__m256i transpose_key_to_vector(const unsigned char *k){
    return;
}