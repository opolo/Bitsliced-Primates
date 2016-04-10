#include "Primates.h"


void main() {
	const unsigned char keys[4][keyLength] = { {'a'}, {'b'}, {'c'}, {'d'} }; //max value 255 = 1 byte
	unsigned char msg[4][100] = { {"My"}, {"name"}, {"is"}, {"Jonas"} };
	const unsigned char nonces[4][NonceLength] = { {8}, {16}, {32}, {64} };
	u64 mLengths[4] = { {2}, {4}, {2}, {5} };

	primates120_encrypt(keys, msg, mLengths, 0, 0, nonces);
}