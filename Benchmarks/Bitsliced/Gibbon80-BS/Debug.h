#ifndef DebugPrint
#include "Parameters.h"

void print_state_as_hex(YMM(*state)[2]);
void print_state_as_hex_with_label(u8 *label, YMM(*state)[2]);
void print_ymm(YMM ymm);

#endif // !Debug