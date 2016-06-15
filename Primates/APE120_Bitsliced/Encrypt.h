#pragma once
#include "Parameters.h"
#include "Debug.h"

void crypto_aead_encrypt(
	u8 *c,
	const u8 *m, u64 mlen,
	const u8 *ad, u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u8 *tag);

int crypto_aead_decrypt(
	u8 *c, u64 clen,
	const u8 *m,
	const u8 *ad, u64 adlen,
	const u8 *nonce,
	const u8 *k,
	u8 *tag);
