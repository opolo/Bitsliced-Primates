void Bytes2Element(unsigned char *inE, const unsigned char *in, unsigned long long start, int numElem);
void Element2Bytes(unsigned char *in, unsigned char *inE, unsigned long long start, int numBytes);
void InitializeState(unsigned char *state, unsigned char *key, unsigned char *nonce);

void p_1_80bit(unsigned char *state);
void p_2_80bit(unsigned char *state);
void p_3_80bit(unsigned char *state);
void p_4_80bit(unsigned char *state);

void p_1_inv_80bit(unsigned char *state);
void p_2_inv_80bit(unsigned char *state);
void p_3_inv_80bit(unsigned char *state);
void p_4_inv_80bit(unsigned char *state);

void GenerateRoundConstants();

void DataBytes2Element(const unsigned char *data, unsigned long long i, unsigned char *dataE);
void CipherElement2Bytes(unsigned char *cE, unsigned char *c, unsigned long long i);
void PrintElementsStateDec(unsigned char *state);
void PrintCipherTextDec(unsigned char *c);
void TagElement2Bytes(unsigned char *cE, unsigned char *c, unsigned long long i);