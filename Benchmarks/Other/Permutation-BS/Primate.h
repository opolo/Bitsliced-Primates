#ifndef Primate
#include "Parameters.h"

void p1(YMM(*state)[2]);
void p2(YMM(*state)[2]);
void p3(YMM(*state)[2]);
void p4(YMM(*state)[2]);
void p1_inv(YMM(*state)[2]);
void p2_inv(YMM(*state)[2]);
void p3_inv(YMM(*state)[2]);
void p4_inv(YMM(*state)[2]);

void p1_80bit(YMM(*state)[2]);
void p2_80bit(YMM(*state)[2]);
void p3_80bit(YMM(*state)[2]);
void p4_80bit(YMM(*state)[2]);
void p1_inv_80bit(YMM(*state)[2]);
void p2_inv_80bit(YMM(*state)[2]);
void p3_inv_80bit(YMM(*state)[2]);
void p4_inv_80bit(YMM(*state)[2]);

void Initialize();

void test_primates();

#endif