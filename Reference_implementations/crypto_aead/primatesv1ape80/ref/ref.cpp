// ref.cpp : main project file.

#include "api.h"
#include <stdio.h>
#include "crypto_aead.h"

int main()
{
	int cor;

	const unsigned char k[CRYPTO_KEYBYTES]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
	const unsigned char npub[CRYPTO_NPUBBYTES]={1,2,3,4,5,6,7,8,9,10};
	unsigned char m[256];
	unsigned char ad[256];
	unsigned char c[256+CRYPTO_ABYTES];
	unsigned long long clen;
	unsigned long long mlenT;

	unsigned char mT[256];
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
	
	for (unsigned long long mlen = 0; mlen < 256; mlen++) {
	  m[mlen] = mlen ^ 0x0c;
	  for (unsigned long long adlen = 0; adlen < 256; adlen++) {
	    ad[adlen] = adlen ^ 0x0f;
	  
	    crypto_aead_encrypt(c,&clen,m,mlen+1,ad,adlen+1,NULL,npub,k);
	/*
		for(unsigned long long i=0; i<clen; i++)
			printf("%.2x ",c[i]);
		printf("\n");
		*/
		cor = crypto_aead_decrypt(mT,&mlenT,NULL,c,clen,ad,adlen+1,npub,k);
		if(cor==0)
		{
			if (mlenT!=(mlen+1))
				printf("sizes don't match!");
			for(unsigned long long i=0; i<mlenT; i++)
				if(mT[i]!=m[i]){
					printf("messages don't match"); break;}

		}
		else
			printf("authentication failed!!");
	  }
	}
	
    return 0;
}
