//
//  PRIMATE120.h
//  Primates_Bitsliced
//
//  Created by Jonas Bruun Jacobsen on 14/03/16.
//  Copyright © 2016 Jonas Bruun Jacobsen. All rights reserved.
//

#ifndef PRIMATE120_h
#define PRIMATE120_h

#include <stdio.h>

int crypto_aead_encrypt(
                        unsigned char *c,unsigned long long *clen,
                        const unsigned char *m,unsigned long long mlen,
                        const unsigned char *ad,unsigned long long adlen,
                        const unsigned char *nsec,
                        const unsigned char *npub,
                        const unsigned char *k
                        );
int crypto_aead_decrypt(
                        unsigned char *m,unsigned long long *mlen,
                        unsigned char *nsec,
                        const unsigned char *c,unsigned long long clen,
                        const unsigned char *ad,unsigned long long adlen,
                        const unsigned char *npub,
                        const unsigned char *k
                        );

#endif /* PRIMATE120_h */
