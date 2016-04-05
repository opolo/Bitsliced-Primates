//
//  main.c
//  Primates_Bitsliced
//
//  Created by Jonas Bruun Jacobsen on 01/03/16.
//  Copyright © 2016 Jonas Bruun Jacobsen. All rights reserved.
//

#include <stdio.h>
#include <immintrin.h>
#include <string.h>

//Defines
#define STATE_ROWS 7 //7*8
#define STATE_COLS 8 //7*8
#define STATESIZE STATE_ROWS * STATE_COLS
#define AVAILABLE_YMM 16
#define true 1
#define false 0

//Typedefs
typedef unsigned char byte;
typedef unsigned long long int int64;
typedef int bool;

int main(int argc, const char * argv[]) {
    
    //declare vars needed
    //TODO
    
    //initialize testdata
    //TODO
    
    //Decrypt
    //TODO
    
    //Verify
    //TODO
    
    return 0;
}


void sencrypt(){
    //const byte *plaintext, byte *ciphertext, const byte *ad, const byte *key
    //In the AVX2 instruction set, there is a total of 16x 256bit registers (called YMM registers) for the AVX2 instructions.
    //In bitsliced format, we place one bit of each byte in each register, thus in total we can store 5x256 = 1280 bits, by using
    //5 registers
    //The state of APE-120 is 280 bits (7x8 elements of 5 bits), and thus we can ideally encrypt 4,5 (=4) blocks in parallel.
   
    //Transpose data to bitsliced format
    //TODO
    
    //Encrypt
    //Input: (K, Ad, M) + optional nonce
    //Output: C, 	T
    //TODO
    
    //detranspose
    
}

/*
 A max of 4 (=4,5 in theory) blocks = 1280 bits can be processed at a time. This is the equivalent of 5x full YMM registers.
 Returns 0 if no padding was needed, else 1.
 
 Assumes that the length of the YMMs array is atleast 5.
*/
int transpose(byte *m, int mL, __m256i *YMM_regs){
    
    
    int currentBit = 0;
    
    //Allocate space for the partial bitsliced data, we arrange and load up in registers.
    int* YMM_memory = (int *) malloc(160); //1280bits = 160bytes.
    
    //Set all the allocated memory to 0.
    memset(YMM_memory, 0, 160);
    
    //Load bits into memory in a bitsliced arrangement.
    //Load either all the data (if 4x280 bits is given), else "as much as is easy possible" before padding.
    while (currentBit <= mL){
        
    }
    
    
    
    int64 YMMvar[5];
    int currentPrimateBit = 0;
    int indices = 0;
    int blockNumber = 0;
    
    //Are they set to zero by default?
    //TODO: Check up on this, might not be neccessary.
    _mm256_zeroall(); // Zero all YMM registers.
    
    //Process all whole blocks in the given plaintext
    while(currentPrimateBit + 5 <= mL) {
        //load next 5 bits of primates state into registers.
        YMMvar[0] = (YMMvar[0] << 1) & m[currentPrimateBit++];
        YMMvar[1] = (YMMvar[1] << 1) & m[currentPrimateBit++];
        YMMvar[2] = (YMMvar[2] << 1) & m[currentPrimateBit++];
        YMMvar[3] = (YMMvar[3] << 1) & m[currentPrimateBit++];
        YMMvar[4] = (YMMvar[4] << 1) & m[currentPrimateBit++];
        
        if(currentPrimateBit % 320 != 0) //320 comes from 5*64, which is when all YMMVars are full.
        {
            //Load all YMMvars into YMM registers.
            //TODO HERE
            
            //Reset all YMMVars
            memset(YMMvar, 0, sizeof(YMMvar)*4);
            
            //Check if we have 4 full YMM registers now, without adding padding
            if (currentPrimateBit == 1280) {
                return 0;
            }
        }
    }
    
    //If we are here, we did not receive 4 full PRIMATE states, so we need to add padding for the remaining state.
    //First, we find in which of the next 5 bits that the plaintext ends(so we can start the padding with a 1),
    //Then we add 0s for the rest of the state.
    if (currentPrimateBit + 1 == mL) {
        //The current bit is the last.
        YMMvar[0] = (YMMvar[0] << 1) & m[currentPrimateBit++];
        YMMvar[1] = (YMMvar[1] << 1) & 1;
        YMMvar[2] = (YMMvar[2] << 1) & 0;
        YMMvar[3] = (YMMvar[3] << 1) & 0;
        YMMvar[4] = (YMMvar[4] << 1) & 0;
        currentPrimateBit = currentPrimateBit + 4;
    }
    else if (currentPrimateBit + 2 == mL) {
        //The second bit is the last.
        YMMvar[0] = (YMMvar[0] << 1) & m[currentPrimateBit++];
        YMMvar[1] = (YMMvar[1] << 1) & m[currentPrimateBit++];
        YMMvar[2] = (YMMvar[2] << 1) & 1;
        YMMvar[3] = (YMMvar[3] << 1) & 0;
        YMMvar[4] = (YMMvar[4] << 1) & 0;
        currentPrimateBit = currentPrimateBit + 3;
    }
    else if (currentPrimateBit + 3 == mL) {
        //The third bit is the last.
        YMMvar[0] = (YMMvar[0] << 1) & m[currentPrimateBit++];
        YMMvar[1] = (YMMvar[1] << 1) & m[currentPrimateBit++];
        YMMvar[2] = (YMMvar[2] << 1) & m[currentPrimateBit++];
        YMMvar[3] = (YMMvar[3] << 1) & 1;
        YMMvar[4] = (YMMvar[4] << 1) & 0;
        currentPrimateBit = currentPrimateBit + 2;
    }
    else if (currentPrimateBit + 4 == mL) {
        //The fourth bit is the last.
        YMMvar[0] = (YMMvar[0] << 1) & m[currentPrimateBit++];
        YMMvar[1] = (YMMvar[1] << 1) & m[currentPrimateBit++];
        YMMvar[2] = (YMMvar[2] << 1) & m[currentPrimateBit++];
        YMMvar[3] = (YMMvar[3] << 1) & m[currentPrimateBit++];
        YMMvar[4] = (YMMvar[4] << 1) & 1;
        currentPrimateBit++;
    }
    
    
    //After placing the initial 1 that indicates the start of the padding, we now add the trailing 0s (by shifting the vars),
    //so that we have a complete state with padding.
    //How many indices should we shift? We find this by seeing, how many bits that remains and then divide them by 5.
    indices = (1280-currentPrimateBit)/5;
    
    //It truncates anything after ".", so it rounds down.
    blockNumber = indices / 64;
    indices = indices % 64;
    YMMvar[0] = YMMvar[0] << indices;
    YMMvar[1] = YMMvar[1] << indices;
    YMMvar[2] = YMMvar[2] << indices;
    YMMvar[3] = YMMvar[3] << indices;
    YMMvar[4] = YMMvar[4] << indices;
    
    //0: Block should be inserted at end of register; 3: Block should be inserted at the start of register.
    switch (blockNumber) {
        case 0:
            //We had 3 full blocks, and the fourth was not complete. Insert variables in the last part of YMMs.
            //TODO
            break;
        case 1:
            //We had 2 full blocks. This is the third.
            //TODO
            break;
        case 2:
            //We had 1 full block. This is the second
            //TODO
            break;
        case 3:
            //We did not have any full blocks, so insert vars at the start of of YMM registers.
            //TODO
            break;
        default:
            break;
    }
    
    return 1;
}
