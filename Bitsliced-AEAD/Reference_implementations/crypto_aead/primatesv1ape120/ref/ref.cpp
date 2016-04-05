// ref.cpp : main project file.

#include "api.h"
#include <stdio.h>
#include "crypto_aead.h"

int main()
{
	int cor;

    /* 
     C = Ciphertext
     clen = Ciphertext length
     m = message
     mlen = messagelength
     ad = associated data
     adlen = associated data length
     nsec = secret message number
     npub = public message number
     k = key
     */
     
	const unsigned char k[CRYPTO_KEYBYTES]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
	const unsigned char npub[CRYPTO_NPUBBYTES]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	unsigned char m[50];
	unsigned char ad[256];
	unsigned char c[256+CRYPTO_ABYTES];
	unsigned long long clen;
	unsigned long long mlenT;
	unsigned char mT[256];

    for (int i = 0; i < 50; i++){
        m[i] = 0;
        ad[i] = 0;
    }
    
    /*
	unsigned long long mlen = 0;
	unsigned long long adlen = 0;
	crypto_aead_encrypt(c,&clen,m,mlen,ad,adlen,NULL,npub,k);
	for(unsigned long long i=0; i<clen; i++)
		printf("%.2x ",c[i]);
	printf("\n");	
	
	cor = crypto_aead_decrypt(mT,&mlenT,NULL,c,clen,ad,adlen,npub,k);
	if(cor==0)
	{
		for(unsigned long long i=0; i<mlenT; i++)
			printf("%.2x ",m[i]);
	}
	else
		printf("authentication failed!!");
	*/
	
    crypto_aead_encrypt(c,&clen,m,50,ad,50,NULL,npub,k);
    cor = crypto_aead_decrypt(mT,&mlenT,NULL,c,clen,ad,50,npub,k);
	
    printf("message: %s \n", m);
    printf("associated data: %s \n", ad);
    printf("key: %s \n", k);
    printf("public nonce: %s \n", npub);
    printf("Ciphertext: %s \n", c);
    return 0;
}
