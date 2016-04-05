/* groestl-avx2-gather.h     Apr-June 2012
 *
 * Groestl implementation with T-Table approach using the AVX2 vpgatherqq instruction,
 * which allows 4 parallel 64-bit lookups.
 *
 * Author: Martin Pernull
 *
 * This code is placed in the public domain.
 *
 * Known performance losses in favor of better code readability/modularity:
 *  LUT/ROUND_x: before first lookup, output register is cleared. 
 *  for pshufb with EXTR_MASK: for the 0-th byte, a simple "and" with a mask would suffice
 */

#include "hash.h"

#define tostr(a) #a

/* lookup table (T-Table)  */
__attribute__ ((aligned (32))) unsigned char T[8*8*256]; /* 16 KiB */
/* shuffle constant which extracts the i-th byte of each quadword */
/* shuffle could be replaced with right shift and pand */
__attribute__ ((aligned (32))) unsigned char EXTR_MASK[32*8];

/* assign 4 qwords to a 256-bit variable
 * var: destination varible
 * off: offset
 * t0-t3: t0=least significant quadword; t3=most significant quadword
 */
#define assign256(var, t0, t1, t2, t3)\
{\
  ((u64*)var)[0] = t0;\
  ((u64*)var)[1] = t1;\
  ((u64*)var)[2] = t2;\
  ((u64*)var)[3] = t3;\
}

#if (LENGTH <= 256)

/* round constants */
__attribute__ ((aligned (32))) unsigned char ROUND_CONST_P[ROUNDS512*32*2];
__attribute__ ((aligned (32))) unsigned char ROUND_CONST_Q[ROUNDS512*32*2];
/* shift bytes constants */
__attribute__ ((aligned (32))) unsigned char SHIFT_P_0[32];
__attribute__ ((aligned (32))) unsigned char SHIFT_P_1[32];
__attribute__ ((aligned (32))) unsigned char SHIFT_Q_0[32];
__attribute__ ((aligned (32))) unsigned char SHIFT_Q_1[32];

/* initialises constants */
#define SET_CONSTANTS()\
{\
  u64 i, j;\
  INIT_LUT();\
  assign256(SHIFT_P_0, 0x0c030d0407080900ULL, 0x0a010b020e050f06ULL, 0x0e050f060a010b02ULL, 0x0c030d0407080900ULL);\
  assign256(SHIFT_P_1, 0x0d0c050409080100ULL, 0x030f0e07060b0a02ULL, 0x01000d0c05040908ULL, 0x0b03020f0e07060aULL);\
  assign256(SHIFT_Q_0, 0x0e01060a0c030408ULL, 0x0d0005090f02070bULL, 0x0f02070b0d000509ULL, 0x0e01060a0c030408ULL);\
  assign256(SHIFT_Q_1, 0x0d0509010c040800ULL, 0x0f070b03020e060aULL, 0x010d0509000c0408ULL, 0x030f070b0a020e06ULL);\
\
for(i = 0; i < 8; i++)\
  {\
    /*this pshufb mask sets all bytes except the lsb to zero*/\
     u64 temp = 0x8080808080808000ULL;\
    ((u64*)EXTR_MASK)[i*4+0] = temp ^ i;\
    ((u64*)EXTR_MASK)[i*4+1] = temp ^ (i+8);\
    ((u64*)EXTR_MASK)[i*4+2] = temp ^ i;\
    ((u64*)EXTR_MASK)[i*4+3] = temp ^ (i+8);\
  }\
  for(i = 0; i < ROUNDS512; i++)\
  {\
    for(j = 0; j < 8; j++)\
    {\
      ((u64*)ROUND_CONST_P)[i*8+j] = (i * 0x0000000000000001ULL) ^ (0x0000000000000010ULL * j);\
      ((u64*)ROUND_CONST_Q)[i*8+j] = (i * 0x0100000000000000ULL) ^ (0x1000000000000000ULL * j) ^ 0xffffffffffffffffULL;\
    }\
  }\
}

/* saves contents of registers r8 - r15 */
#define Push_All_Regs()\
{\
  asm("push  r8");\
  asm("push  r9");\
  asm("push r10");\
  asm("push r11");\
  asm("push r12");\
  asm("push r13");\
  asm("push r14");\
  asm("push r15");\
}

/* initialises registers r8 - r15 with lookup table 
 * memory addresses for each of the 8 lookups 
 */
#define Init_Regs()\
{\
  asm("lea  r8, T+0*2048");\
  asm("lea  r9, T+1*2048");\
  asm("lea r10, T+2*2048");\
  asm("lea r11, T+3*2048");\
  asm("lea r12, T+4*2048");\
  asm("lea r13, T+5*2048");\
  asm("lea r14, T+6*2048");\
  asm("lea r15, T+7*2048");\
}

/* restores contents of registers r8 - r15 */
#define Pop_All_Regs()\
{\
  asm("pop r15");\
  asm("pop r14");\
  asm("pop r13");\
  asm("pop r12");\
  asm("pop r11");\
  asm("pop r10");\
  asm("pop r9");\
  asm("pop r8");\
}

/* one round of AddRoundConstant, ShiftBytes, SubBytes and MixBytes
 * perm = P or Q (determines round constant and shift constant being used)
 * i = round number
 * a0,a1 = input rows (overwritten with temp values)
 * b0,b1 = output rows
 * t0-t5 = clobbers
 */
#define ROUND(perm, i, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5)\
{\
  /* AddRoundConstant */\
  asm ("vpxor ymm"tostr(a0)", ymm"tostr(a0)", [ROUND_CONST_"tostr(perm)"+"tostr(i)"*64]");\
  asm ("vpxor ymm"tostr(a1)", ymm"tostr(a1)", [ROUND_CONST_"tostr(perm)"+"tostr(i)"*64+32]");\
  /* ShiftBytes */\
  SHIFTBYTES(a0, a1, t0, t1, SHIFT_##perm##_0, SHIFT_##perm##_1);\
  /* SubBytes and MixBytes combined */ \
  SUBMIX(a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
}

/* SubBytes and MixBytes
 * uses a parallelized T-Table approach using vpgatherqq
 */
#define SUBMIX(a0, a1, b0, b1, t0, t1, t2, t3, t4, t5)\
{\
  asm("vpxor ymm"tostr(b0)",  ymm"tostr(b0)",  ymm"tostr(b0)"");\
  asm("vpxor ymm"tostr(b1)",  ymm"tostr(b1)",  ymm"tostr(b1)"");\
  LUT(0,  8, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(1,  9, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(2, 10, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(3, 11, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(4, 12, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(5, 13, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(6, 14, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
  LUT(7, 15, a0, a1, b0, b1, t0, t1, t2, t3, t4, t5);\
}

/* perform the lookup in i-th T-Table
 * i = lookup number
 * n = register number (r8 ... r15) containing the offset for vpgatherqq
 * a0-a1 = input rows
 * b0-b1 = output rows
 * t0,t1,u0,u1: clobbers
 */
#define LUT(i, n, in0, in1, out0, out1, t0, t1, u0, u1, v0, v1)\
{\
  /* extract the i-th byte of each quadword */\
  asm("vmovdqa ymm"tostr(t0)", [EXTR_MASK+"tostr(i)"*32]");\
  asm ("vpshufb    ymm"tostr(t0)", ymm"tostr(in0)", [EXTR_MASK+"tostr(i)"*32]");\
  asm ("vpshufb    ymm"tostr(u0)", ymm"tostr(in1)", [EXTR_MASK+"tostr(i)"*32]");\
  /* restore gather masks (v0, v1) each time, as they are overwritten by vpgatherqq */\
  asm ("vpcmpeqq   ymm"tostr(v0)", ymm"tostr(v0)", ymm"tostr(v0)"");\
  asm ("vpcmpeqq   ymm"tostr(v1)", ymm"tostr(v1)", ymm"tostr(v1)"");\
  /* for vpgatherqq, all ymm-registers have to be distinct */\
  asm ("vpgatherqq ymm"tostr(t1)", [8*ymm"tostr(t0)"+r"tostr(n)"], ymm"tostr(v0)"");\
  asm ("vpgatherqq ymm"tostr(u1)", [8*ymm"tostr(u0)"+r"tostr(n)"], ymm"tostr(v1)"");\
  asm ("vpxor      ymm"tostr(out0)", ymm"tostr(out0)", ymm"tostr(t1)"");\
  asm ("vpxor      ymm"tostr(out1)", ymm"tostr(out1)", ymm"tostr(u1)"");\
}

/* shiftbytes
 * a0, a1: input and output
 * t0, t1: clobbers
 * shift0: shuffle constant 1
 * shift1: shuffle constant 2
 * instead of vpblendvb we could also use unpack (with different shuffle constants).
 */
#define SHIFTBYTES(a0, a1, t0, t1, shift0, shift1)\
{\
  /* pre-shuffle */\
  asm ("vpshufb    ymm"tostr(a0)", ymm"tostr(a0)", ["tostr(shift0)"]");\
  asm ("vpshufb    ymm"tostr(a1)", ymm"tostr(a1)", ["tostr(shift0)"]");\
  /* cross lanes */\
  asm ("vpermq     ymm"tostr(t0)", ymm"tostr(a0)", 0xd8");\
  asm ("vpermq     ymm"tostr(t1)", ymm"tostr(a1)", 0xd8");\
  /* combine registers */\
  asm ("vpblendd   ymm"tostr(a0)", ymm"tostr(t0)", ymm"tostr(t1)", 0xaa");\
  asm ("vpblendd   ymm"tostr(a1)", ymm"tostr(t1)", ymm"tostr(t0)", 0xaa");\
  /* final shuffle */\
  asm ("vpshufb    ymm"tostr(a0)", ymm"tostr(a0)", ["tostr(shift1)"]");\
  asm ("vpshufb    ymm"tostr(a1)", ymm"tostr(a1)", ["tostr(shift1)"]");\
}

/* whole permutation (P or Q) -> 10 rounds
 * perm: which permutation (P or Q)
 * p0, p1: the input values are overwritten with the output
 * t0, t1, u0, u1, u2: clobbers
 */
#define PERMUTATION(perm, p0, p1, t0, t1, u0, u1, u2, u3, u4, u5)\
{\
  ROUND(perm, 0, p0, p1, t0, t1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 1, t0, t1, p0, p1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 2, p0, p1, t0, t1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 3, t0, t1, p0, p1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 4, p0, p1, t0, t1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 5, t0, t1, p0, p1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 6, p0, p1, t0, t1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 7, t0, t1, p0, p1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 8, p0, p1, t0, t1, u0, u1, u2, u3, u4, u5);\
  ROUND(perm, 9, t0, t1, p0, p1, u0, u1, u2, u3, u4, u5);\
}

void INIT(u64* h)
{
  /* __cdecl calling convention: */
  /* chaining value CV in rdi    */

  /* nothing to do */

  asm (".intel_syntax noprefix");
  asm volatile ("emms");

  asm (".att_syntax noprefix");
}

void TF512(u64* h, u64* m)
{
  /* __cdecl calling convention: */
  /* chaining value CV in rdi    */
  /* message M in rsi            */

#ifdef IACA_TRACE
  IACA_START;
#endif
  asm (".intel_syntax noprefix");
  Push_All_Regs();
  Init_Regs();

  /* load message (=input of Q) */
  asm ("vmovdqa ymm4, [rsi+0*32]");
  asm ("vmovdqa ymm5, [rsi+1*32]");

  /* xor message and chaining value (=input of P) */
  asm ("vpxor ymm0, ymm4, [rdi+0*32]");
  asm ("vpxor ymm1, ymm5, [rdi+1*32]");

  /* compute the two permutations P and Q */
  PERMUTATION(P, 0, 1, 8, 9, 10, 11, 12, 13, 14, 15);
  PERMUTATION(Q, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15);

  /* xor output of P and Q */
  /* result: P(CV+M)+Q(M) */
  asm ("vpxor ymm12, ymm0, ymm4");
  asm ("vpxor ymm13, ymm1, ymm5");

  /* xor CV (feed-forward) */
  /* result: P(CV+M)+Q(M)+CV */
  asm ("vpxor ymm12, ymm12, [rdi+0*32]");
  asm ("vpxor ymm13, ymm13, [rdi+1*32]");

  /* store CV */
  asm ("vmovdqa [rdi+0*32], ymm12");
  asm ("vmovdqa [rdi+1*32], ymm13");

  Pop_All_Regs();
  asm (".att_syntax noprefix");

#ifdef IACA_TRACE
  IACA_END;
#endif
  return;
}

void OF512(u64* h)
{
  /* __cdecl calling convention: */
  /* chaining value CV in rdi    */

  asm (".intel_syntax noprefix");
  Push_All_Regs();
  Init_Regs();

  /* load CV as only input of P */
  asm ("vmovdqa ymm0, [rdi+0*32]");
  asm ("vmovdqa ymm1, [rdi+1*32]");

  /* compute the permutation P */
  /* result: the output of P(CV) in ymm0, ymm1 */
  PERMUTATION(P, 0, 1, 8, 9, 10, 11, 12, 13, 14, 15);

  /* xor CV to P output (feed-forward) */
  /* result: P(CV)+CV in ymm0, ymm1 */
  asm ("vpxor ymm0,  ymm0, [rdi+0*32]");
  asm ("vpxor ymm1,  ymm1, [rdi+1*32]");

  /* return state */
  //asm ("vmovdqa [rdi+0*32], ymm0");
  asm ("vmovdqa [rdi+1*32], ymm1"); // we only need to return this half of the state

  Pop_All_Regs();
  asm (".att_syntax noprefix");

  return;
}

#else
  #error The AVX2 Gather implementation is currently only available for Groestl-256
#endif



#define INIT_LUT(){\
    ((u64*)T)[0x000] = 0xc6a597f4a5f432c6ULL;\
    ((u64*)T)[0x001] = 0xf884eb9784976ff8ULL;\
    ((u64*)T)[0x002] = 0xee99c7b099b05eeeULL;\
    ((u64*)T)[0x003] = 0xf68df78c8d8c7af6ULL;\
    ((u64*)T)[0x004] = 0xff0de5170d17e8ffULL;\
    ((u64*)T)[0x005] = 0xd6bdb7dcbddc0ad6ULL;\
    ((u64*)T)[0x006] = 0xdeb1a7c8b1c816deULL;\
    ((u64*)T)[0x007] = 0x915439fc54fc6d91ULL;\
    ((u64*)T)[0x008] = 0x6050c0f050f09060ULL;\
    ((u64*)T)[0x009] = 0x0203040503050702ULL;\
    ((u64*)T)[0x00A] = 0xcea987e0a9e02eceULL;\
    ((u64*)T)[0x00B] = 0x567dac877d87d156ULL;\
    ((u64*)T)[0x00C] = 0xe719d52b192bcce7ULL;\
    ((u64*)T)[0x00D] = 0xb56271a662a613b5ULL;\
    ((u64*)T)[0x00E] = 0x4de69a31e6317c4dULL;\
    ((u64*)T)[0x00F] = 0xec9ac3b59ab559ecULL;\
    ((u64*)T)[0x010] = 0x8f4505cf45cf408fULL;\
    ((u64*)T)[0x011] = 0x1f9d3ebc9dbca31fULL;\
    ((u64*)T)[0x012] = 0x894009c040c04989ULL;\
    ((u64*)T)[0x013] = 0xfa87ef92879268faULL;\
    ((u64*)T)[0x014] = 0xef15c53f153fd0efULL;\
    ((u64*)T)[0x015] = 0xb2eb7f26eb2694b2ULL;\
    ((u64*)T)[0x016] = 0x8ec90740c940ce8eULL;\
    ((u64*)T)[0x017] = 0xfb0bed1d0b1de6fbULL;\
    ((u64*)T)[0x018] = 0x41ec822fec2f6e41ULL;\
    ((u64*)T)[0x019] = 0xb3677da967a91ab3ULL;\
    ((u64*)T)[0x01A] = 0x5ffdbe1cfd1c435fULL;\
    ((u64*)T)[0x01B] = 0x45ea8a25ea256045ULL;\
    ((u64*)T)[0x01C] = 0x23bf46dabfdaf923ULL;\
    ((u64*)T)[0x01D] = 0x53f7a602f7025153ULL;\
    ((u64*)T)[0x01E] = 0xe496d3a196a145e4ULL;\
    ((u64*)T)[0x01F] = 0x9b5b2ded5bed769bULL;\
    ((u64*)T)[0x020] = 0x75c2ea5dc25d2875ULL;\
    ((u64*)T)[0x021] = 0xe11cd9241c24c5e1ULL;\
    ((u64*)T)[0x022] = 0x3dae7ae9aee9d43dULL;\
    ((u64*)T)[0x023] = 0x4c6a98be6abef24cULL;\
    ((u64*)T)[0x024] = 0x6c5ad8ee5aee826cULL;\
    ((u64*)T)[0x025] = 0x7e41fcc341c3bd7eULL;\
    ((u64*)T)[0x026] = 0xf502f1060206f3f5ULL;\
    ((u64*)T)[0x027] = 0x834f1dd14fd15283ULL;\
    ((u64*)T)[0x028] = 0x685cd0e45ce48c68ULL;\
    ((u64*)T)[0x029] = 0x51f4a207f4075651ULL;\
    ((u64*)T)[0x02A] = 0xd134b95c345c8dd1ULL;\
    ((u64*)T)[0x02B] = 0xf908e9180818e1f9ULL;\
    ((u64*)T)[0x02C] = 0xe293dfae93ae4ce2ULL;\
    ((u64*)T)[0x02D] = 0xab734d9573953eabULL;\
    ((u64*)T)[0x02E] = 0x6253c4f553f59762ULL;\
    ((u64*)T)[0x02F] = 0x2a3f54413f416b2aULL;\
    ((u64*)T)[0x030] = 0x080c10140c141c08ULL;\
    ((u64*)T)[0x031] = 0x955231f652f66395ULL;\
    ((u64*)T)[0x032] = 0x46658caf65afe946ULL;\
    ((u64*)T)[0x033] = 0x9d5e21e25ee27f9dULL;\
    ((u64*)T)[0x034] = 0x3028607828784830ULL;\
    ((u64*)T)[0x035] = 0x37a16ef8a1f8cf37ULL;\
    ((u64*)T)[0x036] = 0x0a0f14110f111b0aULL;\
    ((u64*)T)[0x037] = 0x2fb55ec4b5c4eb2fULL;\
    ((u64*)T)[0x038] = 0x0e091c1b091b150eULL;\
    ((u64*)T)[0x039] = 0x2436485a365a7e24ULL;\
    ((u64*)T)[0x03A] = 0x1b9b36b69bb6ad1bULL;\
    ((u64*)T)[0x03B] = 0xdf3da5473d4798dfULL;\
    ((u64*)T)[0x03C] = 0xcd26816a266aa7cdULL;\
    ((u64*)T)[0x03D] = 0x4e699cbb69bbf54eULL;\
    ((u64*)T)[0x03E] = 0x7fcdfe4ccd4c337fULL;\
    ((u64*)T)[0x03F] = 0xea9fcfba9fba50eaULL;\
    ((u64*)T)[0x040] = 0x121b242d1b2d3f12ULL;\
    ((u64*)T)[0x041] = 0x1d9e3ab99eb9a41dULL;\
    ((u64*)T)[0x042] = 0x5874b09c749cc458ULL;\
    ((u64*)T)[0x043] = 0x342e68722e724634ULL;\
    ((u64*)T)[0x044] = 0x362d6c772d774136ULL;\
    ((u64*)T)[0x045] = 0xdcb2a3cdb2cd11dcULL;\
    ((u64*)T)[0x046] = 0xb4ee7329ee299db4ULL;\
    ((u64*)T)[0x047] = 0x5bfbb616fb164d5bULL;\
    ((u64*)T)[0x048] = 0xa4f65301f601a5a4ULL;\
    ((u64*)T)[0x049] = 0x764decd74dd7a176ULL;\
    ((u64*)T)[0x04A] = 0xb76175a361a314b7ULL;\
    ((u64*)T)[0x04B] = 0x7dcefa49ce49347dULL;\
    ((u64*)T)[0x04C] = 0x527ba48d7b8ddf52ULL;\
    ((u64*)T)[0x04D] = 0xdd3ea1423e429fddULL;\
    ((u64*)T)[0x04E] = 0x5e71bc937193cd5eULL;\
    ((u64*)T)[0x04F] = 0x139726a297a2b113ULL;\
    ((u64*)T)[0x050] = 0xa6f55704f504a2a6ULL;\
    ((u64*)T)[0x051] = 0xb96869b868b801b9ULL;\
    ((u64*)T)[0x052] = 0x0000000000000000ULL;\
    ((u64*)T)[0x053] = 0xc12c99742c74b5c1ULL;\
    ((u64*)T)[0x054] = 0x406080a060a0e040ULL;\
    ((u64*)T)[0x055] = 0xe31fdd211f21c2e3ULL;\
    ((u64*)T)[0x056] = 0x79c8f243c8433a79ULL;\
    ((u64*)T)[0x057] = 0xb6ed772ced2c9ab6ULL;\
    ((u64*)T)[0x058] = 0xd4beb3d9bed90dd4ULL;\
    ((u64*)T)[0x059] = 0x8d4601ca46ca478dULL;\
    ((u64*)T)[0x05A] = 0x67d9ce70d9701767ULL;\
    ((u64*)T)[0x05B] = 0x724be4dd4bddaf72ULL;\
    ((u64*)T)[0x05C] = 0x94de3379de79ed94ULL;\
    ((u64*)T)[0x05D] = 0x98d42b67d467ff98ULL;\
    ((u64*)T)[0x05E] = 0xb0e87b23e82393b0ULL;\
    ((u64*)T)[0x05F] = 0x854a11de4ade5b85ULL;\
    ((u64*)T)[0x060] = 0xbb6b6dbd6bbd06bbULL;\
    ((u64*)T)[0x061] = 0xc52a917e2a7ebbc5ULL;\
    ((u64*)T)[0x062] = 0x4fe59e34e5347b4fULL;\
    ((u64*)T)[0x063] = 0xed16c13a163ad7edULL;\
    ((u64*)T)[0x064] = 0x86c51754c554d286ULL;\
    ((u64*)T)[0x065] = 0x9ad72f62d762f89aULL;\
    ((u64*)T)[0x066] = 0x6655ccff55ff9966ULL;\
    ((u64*)T)[0x067] = 0x119422a794a7b611ULL;\
    ((u64*)T)[0x068] = 0x8acf0f4acf4ac08aULL;\
    ((u64*)T)[0x069] = 0xe910c9301030d9e9ULL;\
    ((u64*)T)[0x06A] = 0x0406080a060a0e04ULL;\
    ((u64*)T)[0x06B] = 0xfe81e798819866feULL;\
    ((u64*)T)[0x06C] = 0xa0f05b0bf00baba0ULL;\
    ((u64*)T)[0x06D] = 0x7844f0cc44ccb478ULL;\
    ((u64*)T)[0x06E] = 0x25ba4ad5bad5f025ULL;\
    ((u64*)T)[0x06F] = 0x4be3963ee33e754bULL;\
    ((u64*)T)[0x070] = 0xa2f35f0ef30eaca2ULL;\
    ((u64*)T)[0x071] = 0x5dfeba19fe19445dULL;\
    ((u64*)T)[0x072] = 0x80c01b5bc05bdb80ULL;\
    ((u64*)T)[0x073] = 0x058a0a858a858005ULL;\
    ((u64*)T)[0x074] = 0x3fad7eecadecd33fULL;\
    ((u64*)T)[0x075] = 0x21bc42dfbcdffe21ULL;\
    ((u64*)T)[0x076] = 0x7048e0d848d8a870ULL;\
    ((u64*)T)[0x077] = 0xf104f90c040cfdf1ULL;\
    ((u64*)T)[0x078] = 0x63dfc67adf7a1963ULL;\
    ((u64*)T)[0x079] = 0x77c1ee58c1582f77ULL;\
    ((u64*)T)[0x07A] = 0xaf75459f759f30afULL;\
    ((u64*)T)[0x07B] = 0x426384a563a5e742ULL;\
    ((u64*)T)[0x07C] = 0x2030405030507020ULL;\
    ((u64*)T)[0x07D] = 0xe51ad12e1a2ecbe5ULL;\
    ((u64*)T)[0x07E] = 0xfd0ee1120e12effdULL;\
    ((u64*)T)[0x07F] = 0xbf6d65b76db708bfULL;\
    ((u64*)T)[0x080] = 0x814c19d44cd45581ULL;\
    ((u64*)T)[0x081] = 0x1814303c143c2418ULL;\
    ((u64*)T)[0x082] = 0x26354c5f355f7926ULL;\
    ((u64*)T)[0x083] = 0xc32f9d712f71b2c3ULL;\
    ((u64*)T)[0x084] = 0xbee16738e13886beULL;\
    ((u64*)T)[0x085] = 0x35a26afda2fdc835ULL;\
    ((u64*)T)[0x086] = 0x88cc0b4fcc4fc788ULL;\
    ((u64*)T)[0x087] = 0x2e395c4b394b652eULL;\
    ((u64*)T)[0x088] = 0x93573df957f96a93ULL;\
    ((u64*)T)[0x089] = 0x55f2aa0df20d5855ULL;\
    ((u64*)T)[0x08A] = 0xfc82e39d829d61fcULL;\
    ((u64*)T)[0x08B] = 0x7a47f4c947c9b37aULL;\
    ((u64*)T)[0x08C] = 0xc8ac8befacef27c8ULL;\
    ((u64*)T)[0x08D] = 0xbae76f32e73288baULL;\
    ((u64*)T)[0x08E] = 0x322b647d2b7d4f32ULL;\
    ((u64*)T)[0x08F] = 0xe695d7a495a442e6ULL;\
    ((u64*)T)[0x090] = 0xc0a09bfba0fb3bc0ULL;\
    ((u64*)T)[0x091] = 0x199832b398b3aa19ULL;\
    ((u64*)T)[0x092] = 0x9ed12768d168f69eULL;\
    ((u64*)T)[0x093] = 0xa37f5d817f8122a3ULL;\
    ((u64*)T)[0x094] = 0x446688aa66aaee44ULL;\
    ((u64*)T)[0x095] = 0x547ea8827e82d654ULL;\
    ((u64*)T)[0x096] = 0x3bab76e6abe6dd3bULL;\
    ((u64*)T)[0x097] = 0x0b83169e839e950bULL;\
    ((u64*)T)[0x098] = 0x8cca0345ca45c98cULL;\
    ((u64*)T)[0x099] = 0xc729957b297bbcc7ULL;\
    ((u64*)T)[0x09A] = 0x6bd3d66ed36e056bULL;\
    ((u64*)T)[0x09B] = 0x283c50443c446c28ULL;\
    ((u64*)T)[0x09C] = 0xa779558b798b2ca7ULL;\
    ((u64*)T)[0x09D] = 0xbce2633de23d81bcULL;\
    ((u64*)T)[0x09E] = 0x161d2c271d273116ULL;\
    ((u64*)T)[0x09F] = 0xad76419a769a37adULL;\
    ((u64*)T)[0x0A0] = 0xdb3bad4d3b4d96dbULL;\
    ((u64*)T)[0x0A1] = 0x6456c8fa56fa9e64ULL;\
    ((u64*)T)[0x0A2] = 0x744ee8d24ed2a674ULL;\
    ((u64*)T)[0x0A3] = 0x141e28221e223614ULL;\
    ((u64*)T)[0x0A4] = 0x92db3f76db76e492ULL;\
    ((u64*)T)[0x0A5] = 0x0c0a181e0a1e120cULL;\
    ((u64*)T)[0x0A6] = 0x486c90b46cb4fc48ULL;\
    ((u64*)T)[0x0A7] = 0xb8e46b37e4378fb8ULL;\
    ((u64*)T)[0x0A8] = 0x9f5d25e75de7789fULL;\
    ((u64*)T)[0x0A9] = 0xbd6e61b26eb20fbdULL;\
    ((u64*)T)[0x0AA] = 0x43ef862aef2a6943ULL;\
    ((u64*)T)[0x0AB] = 0xc4a693f1a6f135c4ULL;\
    ((u64*)T)[0x0AC] = 0x39a872e3a8e3da39ULL;\
    ((u64*)T)[0x0AD] = 0x31a462f7a4f7c631ULL;\
    ((u64*)T)[0x0AE] = 0xd337bd5937598ad3ULL;\
    ((u64*)T)[0x0AF] = 0xf28bff868b8674f2ULL;\
    ((u64*)T)[0x0B0] = 0xd532b156325683d5ULL;\
    ((u64*)T)[0x0B1] = 0x8b430dc543c54e8bULL;\
    ((u64*)T)[0x0B2] = 0x6e59dceb59eb856eULL;\
    ((u64*)T)[0x0B3] = 0xdab7afc2b7c218daULL;\
    ((u64*)T)[0x0B4] = 0x018c028f8c8f8e01ULL;\
    ((u64*)T)[0x0B5] = 0xb16479ac64ac1db1ULL;\
    ((u64*)T)[0x0B6] = 0x9cd2236dd26df19cULL;\
    ((u64*)T)[0x0B7] = 0x49e0923be03b7249ULL;\
    ((u64*)T)[0x0B8] = 0xd8b4abc7b4c71fd8ULL;\
    ((u64*)T)[0x0B9] = 0xacfa4315fa15b9acULL;\
    ((u64*)T)[0x0BA] = 0xf307fd090709faf3ULL;\
    ((u64*)T)[0x0BB] = 0xcf25856f256fa0cfULL;\
    ((u64*)T)[0x0BC] = 0xcaaf8feaafea20caULL;\
    ((u64*)T)[0x0BD] = 0xf48ef3898e897df4ULL;\
    ((u64*)T)[0x0BE] = 0x47e98e20e9206747ULL;\
    ((u64*)T)[0x0BF] = 0x1018202818283810ULL;\
    ((u64*)T)[0x0C0] = 0x6fd5de64d5640b6fULL;\
    ((u64*)T)[0x0C1] = 0xf088fb83888373f0ULL;\
    ((u64*)T)[0x0C2] = 0x4a6f94b16fb1fb4aULL;\
    ((u64*)T)[0x0C3] = 0x5c72b8967296ca5cULL;\
    ((u64*)T)[0x0C4] = 0x3824706c246c5438ULL;\
    ((u64*)T)[0x0C5] = 0x57f1ae08f1085f57ULL;\
    ((u64*)T)[0x0C6] = 0x73c7e652c7522173ULL;\
    ((u64*)T)[0x0C7] = 0x975135f351f36497ULL;\
    ((u64*)T)[0x0C8] = 0xcb238d652365aecbULL;\
    ((u64*)T)[0x0C9] = 0xa17c59847c8425a1ULL;\
    ((u64*)T)[0x0CA] = 0xe89ccbbf9cbf57e8ULL;\
    ((u64*)T)[0x0CB] = 0x3e217c6321635d3eULL;\
    ((u64*)T)[0x0CC] = 0x96dd377cdd7cea96ULL;\
    ((u64*)T)[0x0CD] = 0x61dcc27fdc7f1e61ULL;\
    ((u64*)T)[0x0CE] = 0x0d861a9186919c0dULL;\
    ((u64*)T)[0x0CF] = 0x0f851e9485949b0fULL;\
    ((u64*)T)[0x0D0] = 0xe090dbab90ab4be0ULL;\
    ((u64*)T)[0x0D1] = 0x7c42f8c642c6ba7cULL;\
    ((u64*)T)[0x0D2] = 0x71c4e257c4572671ULL;\
    ((u64*)T)[0x0D3] = 0xccaa83e5aae529ccULL;\
    ((u64*)T)[0x0D4] = 0x90d83b73d873e390ULL;\
    ((u64*)T)[0x0D5] = 0x06050c0f050f0906ULL;\
    ((u64*)T)[0x0D6] = 0xf701f5030103f4f7ULL;\
    ((u64*)T)[0x0D7] = 0x1c12383612362a1cULL;\
    ((u64*)T)[0x0D8] = 0xc2a39ffea3fe3cc2ULL;\
    ((u64*)T)[0x0D9] = 0x6a5fd4e15fe18b6aULL;\
    ((u64*)T)[0x0DA] = 0xaef94710f910beaeULL;\
    ((u64*)T)[0x0DB] = 0x69d0d26bd06b0269ULL;\
    ((u64*)T)[0x0DC] = 0x17912ea891a8bf17ULL;\
    ((u64*)T)[0x0DD] = 0x995829e858e87199ULL;\
    ((u64*)T)[0x0DE] = 0x3a2774692769533aULL;\
    ((u64*)T)[0x0DF] = 0x27b94ed0b9d0f727ULL;\
    ((u64*)T)[0x0E0] = 0xd938a948384891d9ULL;\
    ((u64*)T)[0x0E1] = 0xeb13cd351335deebULL;\
    ((u64*)T)[0x0E2] = 0x2bb356ceb3cee52bULL;\
    ((u64*)T)[0x0E3] = 0x2233445533557722ULL;\
    ((u64*)T)[0x0E4] = 0xd2bbbfd6bbd604d2ULL;\
    ((u64*)T)[0x0E5] = 0xa9704990709039a9ULL;\
    ((u64*)T)[0x0E6] = 0x07890e8089808707ULL;\
    ((u64*)T)[0x0E7] = 0x33a766f2a7f2c133ULL;\
    ((u64*)T)[0x0E8] = 0x2db65ac1b6c1ec2dULL;\
    ((u64*)T)[0x0E9] = 0x3c22786622665a3cULL;\
    ((u64*)T)[0x0EA] = 0x15922aad92adb815ULL;\
    ((u64*)T)[0x0EB] = 0xc92089602060a9c9ULL;\
    ((u64*)T)[0x0EC] = 0x874915db49db5c87ULL;\
    ((u64*)T)[0x0ED] = 0xaaff4f1aff1ab0aaULL;\
    ((u64*)T)[0x0EE] = 0x5078a0887888d850ULL;\
    ((u64*)T)[0x0EF] = 0xa57a518e7a8e2ba5ULL;\
    ((u64*)T)[0x0F0] = 0x038f068a8f8a8903ULL;\
    ((u64*)T)[0x0F1] = 0x59f8b213f8134a59ULL;\
    ((u64*)T)[0x0F2] = 0x0980129b809b9209ULL;\
    ((u64*)T)[0x0F3] = 0x1a1734391739231aULL;\
    ((u64*)T)[0x0F4] = 0x65daca75da751065ULL;\
    ((u64*)T)[0x0F5] = 0xd731b553315384d7ULL;\
    ((u64*)T)[0x0F6] = 0x84c61351c651d584ULL;\
    ((u64*)T)[0x0F7] = 0xd0b8bbd3b8d303d0ULL;\
    ((u64*)T)[0x0F8] = 0x82c31f5ec35edc82ULL;\
    ((u64*)T)[0x0F9] = 0x29b052cbb0cbe229ULL;\
    ((u64*)T)[0x0FA] = 0x5a77b4997799c35aULL;\
    ((u64*)T)[0x0FB] = 0x1e113c3311332d1eULL;\
    ((u64*)T)[0x0FC] = 0x7bcbf646cb463d7bULL;\
    ((u64*)T)[0x0FD] = 0xa8fc4b1ffc1fb7a8ULL;\
    ((u64*)T)[0x0FE] = 0x6dd6da61d6610c6dULL;\
    ((u64*)T)[0x0FF] = 0x2c3a584e3a4e622cULL;\
    ((u64*)T)[0x100] = 0xa597f4a5f432c6c6ULL;\
    ((u64*)T)[0x101] = 0x84eb9784976ff8f8ULL;\
    ((u64*)T)[0x102] = 0x99c7b099b05eeeeeULL;\
    ((u64*)T)[0x103] = 0x8df78c8d8c7af6f6ULL;\
    ((u64*)T)[0x104] = 0x0de5170d17e8ffffULL;\
    ((u64*)T)[0x105] = 0xbdb7dcbddc0ad6d6ULL;\
    ((u64*)T)[0x106] = 0xb1a7c8b1c816dedeULL;\
    ((u64*)T)[0x107] = 0x5439fc54fc6d9191ULL;\
    ((u64*)T)[0x108] = 0x50c0f050f0906060ULL;\
    ((u64*)T)[0x109] = 0x0304050305070202ULL;\
    ((u64*)T)[0x10A] = 0xa987e0a9e02ececeULL;\
    ((u64*)T)[0x10B] = 0x7dac877d87d15656ULL;\
    ((u64*)T)[0x10C] = 0x19d52b192bcce7e7ULL;\
    ((u64*)T)[0x10D] = 0x6271a662a613b5b5ULL;\
    ((u64*)T)[0x10E] = 0xe69a31e6317c4d4dULL;\
    ((u64*)T)[0x10F] = 0x9ac3b59ab559ececULL;\
    ((u64*)T)[0x110] = 0x4505cf45cf408f8fULL;\
    ((u64*)T)[0x111] = 0x9d3ebc9dbca31f1fULL;\
    ((u64*)T)[0x112] = 0x4009c040c0498989ULL;\
    ((u64*)T)[0x113] = 0x87ef92879268fafaULL;\
    ((u64*)T)[0x114] = 0x15c53f153fd0efefULL;\
    ((u64*)T)[0x115] = 0xeb7f26eb2694b2b2ULL;\
    ((u64*)T)[0x116] = 0xc90740c940ce8e8eULL;\
    ((u64*)T)[0x117] = 0x0bed1d0b1de6fbfbULL;\
    ((u64*)T)[0x118] = 0xec822fec2f6e4141ULL;\
    ((u64*)T)[0x119] = 0x677da967a91ab3b3ULL;\
    ((u64*)T)[0x11A] = 0xfdbe1cfd1c435f5fULL;\
    ((u64*)T)[0x11B] = 0xea8a25ea25604545ULL;\
    ((u64*)T)[0x11C] = 0xbf46dabfdaf92323ULL;\
    ((u64*)T)[0x11D] = 0xf7a602f702515353ULL;\
    ((u64*)T)[0x11E] = 0x96d3a196a145e4e4ULL;\
    ((u64*)T)[0x11F] = 0x5b2ded5bed769b9bULL;\
    ((u64*)T)[0x120] = 0xc2ea5dc25d287575ULL;\
    ((u64*)T)[0x121] = 0x1cd9241c24c5e1e1ULL;\
    ((u64*)T)[0x122] = 0xae7ae9aee9d43d3dULL;\
    ((u64*)T)[0x123] = 0x6a98be6abef24c4cULL;\
    ((u64*)T)[0x124] = 0x5ad8ee5aee826c6cULL;\
    ((u64*)T)[0x125] = 0x41fcc341c3bd7e7eULL;\
    ((u64*)T)[0x126] = 0x02f1060206f3f5f5ULL;\
    ((u64*)T)[0x127] = 0x4f1dd14fd1528383ULL;\
    ((u64*)T)[0x128] = 0x5cd0e45ce48c6868ULL;\
    ((u64*)T)[0x129] = 0xf4a207f407565151ULL;\
    ((u64*)T)[0x12A] = 0x34b95c345c8dd1d1ULL;\
    ((u64*)T)[0x12B] = 0x08e9180818e1f9f9ULL;\
    ((u64*)T)[0x12C] = 0x93dfae93ae4ce2e2ULL;\
    ((u64*)T)[0x12D] = 0x734d9573953eababULL;\
    ((u64*)T)[0x12E] = 0x53c4f553f5976262ULL;\
    ((u64*)T)[0x12F] = 0x3f54413f416b2a2aULL;\
    ((u64*)T)[0x130] = 0x0c10140c141c0808ULL;\
    ((u64*)T)[0x131] = 0x5231f652f6639595ULL;\
    ((u64*)T)[0x132] = 0x658caf65afe94646ULL;\
    ((u64*)T)[0x133] = 0x5e21e25ee27f9d9dULL;\
    ((u64*)T)[0x134] = 0x2860782878483030ULL;\
    ((u64*)T)[0x135] = 0xa16ef8a1f8cf3737ULL;\
    ((u64*)T)[0x136] = 0x0f14110f111b0a0aULL;\
    ((u64*)T)[0x137] = 0xb55ec4b5c4eb2f2fULL;\
    ((u64*)T)[0x138] = 0x091c1b091b150e0eULL;\
    ((u64*)T)[0x139] = 0x36485a365a7e2424ULL;\
    ((u64*)T)[0x13A] = 0x9b36b69bb6ad1b1bULL;\
    ((u64*)T)[0x13B] = 0x3da5473d4798dfdfULL;\
    ((u64*)T)[0x13C] = 0x26816a266aa7cdcdULL;\
    ((u64*)T)[0x13D] = 0x699cbb69bbf54e4eULL;\
    ((u64*)T)[0x13E] = 0xcdfe4ccd4c337f7fULL;\
    ((u64*)T)[0x13F] = 0x9fcfba9fba50eaeaULL;\
    ((u64*)T)[0x140] = 0x1b242d1b2d3f1212ULL;\
    ((u64*)T)[0x141] = 0x9e3ab99eb9a41d1dULL;\
    ((u64*)T)[0x142] = 0x74b09c749cc45858ULL;\
    ((u64*)T)[0x143] = 0x2e68722e72463434ULL;\
    ((u64*)T)[0x144] = 0x2d6c772d77413636ULL;\
    ((u64*)T)[0x145] = 0xb2a3cdb2cd11dcdcULL;\
    ((u64*)T)[0x146] = 0xee7329ee299db4b4ULL;\
    ((u64*)T)[0x147] = 0xfbb616fb164d5b5bULL;\
    ((u64*)T)[0x148] = 0xf65301f601a5a4a4ULL;\
    ((u64*)T)[0x149] = 0x4decd74dd7a17676ULL;\
    ((u64*)T)[0x14A] = 0x6175a361a314b7b7ULL;\
    ((u64*)T)[0x14B] = 0xcefa49ce49347d7dULL;\
    ((u64*)T)[0x14C] = 0x7ba48d7b8ddf5252ULL;\
    ((u64*)T)[0x14D] = 0x3ea1423e429fddddULL;\
    ((u64*)T)[0x14E] = 0x71bc937193cd5e5eULL;\
    ((u64*)T)[0x14F] = 0x9726a297a2b11313ULL;\
    ((u64*)T)[0x150] = 0xf55704f504a2a6a6ULL;\
    ((u64*)T)[0x151] = 0x6869b868b801b9b9ULL;\
    ((u64*)T)[0x152] = 0x0000000000000000ULL;\
    ((u64*)T)[0x153] = 0x2c99742c74b5c1c1ULL;\
    ((u64*)T)[0x154] = 0x6080a060a0e04040ULL;\
    ((u64*)T)[0x155] = 0x1fdd211f21c2e3e3ULL;\
    ((u64*)T)[0x156] = 0xc8f243c8433a7979ULL;\
    ((u64*)T)[0x157] = 0xed772ced2c9ab6b6ULL;\
    ((u64*)T)[0x158] = 0xbeb3d9bed90dd4d4ULL;\
    ((u64*)T)[0x159] = 0x4601ca46ca478d8dULL;\
    ((u64*)T)[0x15A] = 0xd9ce70d970176767ULL;\
    ((u64*)T)[0x15B] = 0x4be4dd4bddaf7272ULL;\
    ((u64*)T)[0x15C] = 0xde3379de79ed9494ULL;\
    ((u64*)T)[0x15D] = 0xd42b67d467ff9898ULL;\
    ((u64*)T)[0x15E] = 0xe87b23e82393b0b0ULL;\
    ((u64*)T)[0x15F] = 0x4a11de4ade5b8585ULL;\
    ((u64*)T)[0x160] = 0x6b6dbd6bbd06bbbbULL;\
    ((u64*)T)[0x161] = 0x2a917e2a7ebbc5c5ULL;\
    ((u64*)T)[0x162] = 0xe59e34e5347b4f4fULL;\
    ((u64*)T)[0x163] = 0x16c13a163ad7ededULL;\
    ((u64*)T)[0x164] = 0xc51754c554d28686ULL;\
    ((u64*)T)[0x165] = 0xd72f62d762f89a9aULL;\
    ((u64*)T)[0x166] = 0x55ccff55ff996666ULL;\
    ((u64*)T)[0x167] = 0x9422a794a7b61111ULL;\
    ((u64*)T)[0x168] = 0xcf0f4acf4ac08a8aULL;\
    ((u64*)T)[0x169] = 0x10c9301030d9e9e9ULL;\
    ((u64*)T)[0x16A] = 0x06080a060a0e0404ULL;\
    ((u64*)T)[0x16B] = 0x81e798819866fefeULL;\
    ((u64*)T)[0x16C] = 0xf05b0bf00baba0a0ULL;\
    ((u64*)T)[0x16D] = 0x44f0cc44ccb47878ULL;\
    ((u64*)T)[0x16E] = 0xba4ad5bad5f02525ULL;\
    ((u64*)T)[0x16F] = 0xe3963ee33e754b4bULL;\
    ((u64*)T)[0x170] = 0xf35f0ef30eaca2a2ULL;\
    ((u64*)T)[0x171] = 0xfeba19fe19445d5dULL;\
    ((u64*)T)[0x172] = 0xc01b5bc05bdb8080ULL;\
    ((u64*)T)[0x173] = 0x8a0a858a85800505ULL;\
    ((u64*)T)[0x174] = 0xad7eecadecd33f3fULL;\
    ((u64*)T)[0x175] = 0xbc42dfbcdffe2121ULL;\
    ((u64*)T)[0x176] = 0x48e0d848d8a87070ULL;\
    ((u64*)T)[0x177] = 0x04f90c040cfdf1f1ULL;\
    ((u64*)T)[0x178] = 0xdfc67adf7a196363ULL;\
    ((u64*)T)[0x179] = 0xc1ee58c1582f7777ULL;\
    ((u64*)T)[0x17A] = 0x75459f759f30afafULL;\
    ((u64*)T)[0x17B] = 0x6384a563a5e74242ULL;\
    ((u64*)T)[0x17C] = 0x3040503050702020ULL;\
    ((u64*)T)[0x17D] = 0x1ad12e1a2ecbe5e5ULL;\
    ((u64*)T)[0x17E] = 0x0ee1120e12effdfdULL;\
    ((u64*)T)[0x17F] = 0x6d65b76db708bfbfULL;\
    ((u64*)T)[0x180] = 0x4c19d44cd4558181ULL;\
    ((u64*)T)[0x181] = 0x14303c143c241818ULL;\
    ((u64*)T)[0x182] = 0x354c5f355f792626ULL;\
    ((u64*)T)[0x183] = 0x2f9d712f71b2c3c3ULL;\
    ((u64*)T)[0x184] = 0xe16738e13886bebeULL;\
    ((u64*)T)[0x185] = 0xa26afda2fdc83535ULL;\
    ((u64*)T)[0x186] = 0xcc0b4fcc4fc78888ULL;\
    ((u64*)T)[0x187] = 0x395c4b394b652e2eULL;\
    ((u64*)T)[0x188] = 0x573df957f96a9393ULL;\
    ((u64*)T)[0x189] = 0xf2aa0df20d585555ULL;\
    ((u64*)T)[0x18A] = 0x82e39d829d61fcfcULL;\
    ((u64*)T)[0x18B] = 0x47f4c947c9b37a7aULL;\
    ((u64*)T)[0x18C] = 0xac8befacef27c8c8ULL;\
    ((u64*)T)[0x18D] = 0xe76f32e73288babaULL;\
    ((u64*)T)[0x18E] = 0x2b647d2b7d4f3232ULL;\
    ((u64*)T)[0x18F] = 0x95d7a495a442e6e6ULL;\
    ((u64*)T)[0x190] = 0xa09bfba0fb3bc0c0ULL;\
    ((u64*)T)[0x191] = 0x9832b398b3aa1919ULL;\
    ((u64*)T)[0x192] = 0xd12768d168f69e9eULL;\
    ((u64*)T)[0x193] = 0x7f5d817f8122a3a3ULL;\
    ((u64*)T)[0x194] = 0x6688aa66aaee4444ULL;\
    ((u64*)T)[0x195] = 0x7ea8827e82d65454ULL;\
    ((u64*)T)[0x196] = 0xab76e6abe6dd3b3bULL;\
    ((u64*)T)[0x197] = 0x83169e839e950b0bULL;\
    ((u64*)T)[0x198] = 0xca0345ca45c98c8cULL;\
    ((u64*)T)[0x199] = 0x29957b297bbcc7c7ULL;\
    ((u64*)T)[0x19A] = 0xd3d66ed36e056b6bULL;\
    ((u64*)T)[0x19B] = 0x3c50443c446c2828ULL;\
    ((u64*)T)[0x19C] = 0x79558b798b2ca7a7ULL;\
    ((u64*)T)[0x19D] = 0xe2633de23d81bcbcULL;\
    ((u64*)T)[0x19E] = 0x1d2c271d27311616ULL;\
    ((u64*)T)[0x19F] = 0x76419a769a37adadULL;\
    ((u64*)T)[0x1A0] = 0x3bad4d3b4d96dbdbULL;\
    ((u64*)T)[0x1A1] = 0x56c8fa56fa9e6464ULL;\
    ((u64*)T)[0x1A2] = 0x4ee8d24ed2a67474ULL;\
    ((u64*)T)[0x1A3] = 0x1e28221e22361414ULL;\
    ((u64*)T)[0x1A4] = 0xdb3f76db76e49292ULL;\
    ((u64*)T)[0x1A5] = 0x0a181e0a1e120c0cULL;\
    ((u64*)T)[0x1A6] = 0x6c90b46cb4fc4848ULL;\
    ((u64*)T)[0x1A7] = 0xe46b37e4378fb8b8ULL;\
    ((u64*)T)[0x1A8] = 0x5d25e75de7789f9fULL;\
    ((u64*)T)[0x1A9] = 0x6e61b26eb20fbdbdULL;\
    ((u64*)T)[0x1AA] = 0xef862aef2a694343ULL;\
    ((u64*)T)[0x1AB] = 0xa693f1a6f135c4c4ULL;\
    ((u64*)T)[0x1AC] = 0xa872e3a8e3da3939ULL;\
    ((u64*)T)[0x1AD] = 0xa462f7a4f7c63131ULL;\
    ((u64*)T)[0x1AE] = 0x37bd5937598ad3d3ULL;\
    ((u64*)T)[0x1AF] = 0x8bff868b8674f2f2ULL;\
    ((u64*)T)[0x1B0] = 0x32b156325683d5d5ULL;\
    ((u64*)T)[0x1B1] = 0x430dc543c54e8b8bULL;\
    ((u64*)T)[0x1B2] = 0x59dceb59eb856e6eULL;\
    ((u64*)T)[0x1B3] = 0xb7afc2b7c218dadaULL;\
    ((u64*)T)[0x1B4] = 0x8c028f8c8f8e0101ULL;\
    ((u64*)T)[0x1B5] = 0x6479ac64ac1db1b1ULL;\
    ((u64*)T)[0x1B6] = 0xd2236dd26df19c9cULL;\
    ((u64*)T)[0x1B7] = 0xe0923be03b724949ULL;\
    ((u64*)T)[0x1B8] = 0xb4abc7b4c71fd8d8ULL;\
    ((u64*)T)[0x1B9] = 0xfa4315fa15b9acacULL;\
    ((u64*)T)[0x1BA] = 0x07fd090709faf3f3ULL;\
    ((u64*)T)[0x1BB] = 0x25856f256fa0cfcfULL;\
    ((u64*)T)[0x1BC] = 0xaf8feaafea20cacaULL;\
    ((u64*)T)[0x1BD] = 0x8ef3898e897df4f4ULL;\
    ((u64*)T)[0x1BE] = 0xe98e20e920674747ULL;\
    ((u64*)T)[0x1BF] = 0x1820281828381010ULL;\
    ((u64*)T)[0x1C0] = 0xd5de64d5640b6f6fULL;\
    ((u64*)T)[0x1C1] = 0x88fb83888373f0f0ULL;\
    ((u64*)T)[0x1C2] = 0x6f94b16fb1fb4a4aULL;\
    ((u64*)T)[0x1C3] = 0x72b8967296ca5c5cULL;\
    ((u64*)T)[0x1C4] = 0x24706c246c543838ULL;\
    ((u64*)T)[0x1C5] = 0xf1ae08f1085f5757ULL;\
    ((u64*)T)[0x1C6] = 0xc7e652c752217373ULL;\
    ((u64*)T)[0x1C7] = 0x5135f351f3649797ULL;\
    ((u64*)T)[0x1C8] = 0x238d652365aecbcbULL;\
    ((u64*)T)[0x1C9] = 0x7c59847c8425a1a1ULL;\
    ((u64*)T)[0x1CA] = 0x9ccbbf9cbf57e8e8ULL;\
    ((u64*)T)[0x1CB] = 0x217c6321635d3e3eULL;\
    ((u64*)T)[0x1CC] = 0xdd377cdd7cea9696ULL;\
    ((u64*)T)[0x1CD] = 0xdcc27fdc7f1e6161ULL;\
    ((u64*)T)[0x1CE] = 0x861a9186919c0d0dULL;\
    ((u64*)T)[0x1CF] = 0x851e9485949b0f0fULL;\
    ((u64*)T)[0x1D0] = 0x90dbab90ab4be0e0ULL;\
    ((u64*)T)[0x1D1] = 0x42f8c642c6ba7c7cULL;\
    ((u64*)T)[0x1D2] = 0xc4e257c457267171ULL;\
    ((u64*)T)[0x1D3] = 0xaa83e5aae529ccccULL;\
    ((u64*)T)[0x1D4] = 0xd83b73d873e39090ULL;\
    ((u64*)T)[0x1D5] = 0x050c0f050f090606ULL;\
    ((u64*)T)[0x1D6] = 0x01f5030103f4f7f7ULL;\
    ((u64*)T)[0x1D7] = 0x12383612362a1c1cULL;\
    ((u64*)T)[0x1D8] = 0xa39ffea3fe3cc2c2ULL;\
    ((u64*)T)[0x1D9] = 0x5fd4e15fe18b6a6aULL;\
    ((u64*)T)[0x1DA] = 0xf94710f910beaeaeULL;\
    ((u64*)T)[0x1DB] = 0xd0d26bd06b026969ULL;\
    ((u64*)T)[0x1DC] = 0x912ea891a8bf1717ULL;\
    ((u64*)T)[0x1DD] = 0x5829e858e8719999ULL;\
    ((u64*)T)[0x1DE] = 0x2774692769533a3aULL;\
    ((u64*)T)[0x1DF] = 0xb94ed0b9d0f72727ULL;\
    ((u64*)T)[0x1E0] = 0x38a948384891d9d9ULL;\
    ((u64*)T)[0x1E1] = 0x13cd351335deebebULL;\
    ((u64*)T)[0x1E2] = 0xb356ceb3cee52b2bULL;\
    ((u64*)T)[0x1E3] = 0x3344553355772222ULL;\
    ((u64*)T)[0x1E4] = 0xbbbfd6bbd604d2d2ULL;\
    ((u64*)T)[0x1E5] = 0x704990709039a9a9ULL;\
    ((u64*)T)[0x1E6] = 0x890e808980870707ULL;\
    ((u64*)T)[0x1E7] = 0xa766f2a7f2c13333ULL;\
    ((u64*)T)[0x1E8] = 0xb65ac1b6c1ec2d2dULL;\
    ((u64*)T)[0x1E9] = 0x22786622665a3c3cULL;\
    ((u64*)T)[0x1EA] = 0x922aad92adb81515ULL;\
    ((u64*)T)[0x1EB] = 0x2089602060a9c9c9ULL;\
    ((u64*)T)[0x1EC] = 0x4915db49db5c8787ULL;\
    ((u64*)T)[0x1ED] = 0xff4f1aff1ab0aaaaULL;\
    ((u64*)T)[0x1EE] = 0x78a0887888d85050ULL;\
    ((u64*)T)[0x1EF] = 0x7a518e7a8e2ba5a5ULL;\
    ((u64*)T)[0x1F0] = 0x8f068a8f8a890303ULL;\
    ((u64*)T)[0x1F1] = 0xf8b213f8134a5959ULL;\
    ((u64*)T)[0x1F2] = 0x80129b809b920909ULL;\
    ((u64*)T)[0x1F3] = 0x1734391739231a1aULL;\
    ((u64*)T)[0x1F4] = 0xdaca75da75106565ULL;\
    ((u64*)T)[0x1F5] = 0x31b553315384d7d7ULL;\
    ((u64*)T)[0x1F6] = 0xc61351c651d58484ULL;\
    ((u64*)T)[0x1F7] = 0xb8bbd3b8d303d0d0ULL;\
    ((u64*)T)[0x1F8] = 0xc31f5ec35edc8282ULL;\
    ((u64*)T)[0x1F9] = 0xb052cbb0cbe22929ULL;\
    ((u64*)T)[0x1FA] = 0x77b4997799c35a5aULL;\
    ((u64*)T)[0x1FB] = 0x113c3311332d1e1eULL;\
    ((u64*)T)[0x1FC] = 0xcbf646cb463d7b7bULL;\
    ((u64*)T)[0x1FD] = 0xfc4b1ffc1fb7a8a8ULL;\
    ((u64*)T)[0x1FE] = 0xd6da61d6610c6d6dULL;\
    ((u64*)T)[0x1FF] = 0x3a584e3a4e622c2cULL;\
    ((u64*)T)[0x200] = 0x97f4a5f432c6c6a5ULL;\
    ((u64*)T)[0x201] = 0xeb9784976ff8f884ULL;\
    ((u64*)T)[0x202] = 0xc7b099b05eeeee99ULL;\
    ((u64*)T)[0x203] = 0xf78c8d8c7af6f68dULL;\
    ((u64*)T)[0x204] = 0xe5170d17e8ffff0dULL;\
    ((u64*)T)[0x205] = 0xb7dcbddc0ad6d6bdULL;\
    ((u64*)T)[0x206] = 0xa7c8b1c816dedeb1ULL;\
    ((u64*)T)[0x207] = 0x39fc54fc6d919154ULL;\
    ((u64*)T)[0x208] = 0xc0f050f090606050ULL;\
    ((u64*)T)[0x209] = 0x0405030507020203ULL;\
    ((u64*)T)[0x20A] = 0x87e0a9e02ececea9ULL;\
    ((u64*)T)[0x20B] = 0xac877d87d156567dULL;\
    ((u64*)T)[0x20C] = 0xd52b192bcce7e719ULL;\
    ((u64*)T)[0x20D] = 0x71a662a613b5b562ULL;\
    ((u64*)T)[0x20E] = 0x9a31e6317c4d4de6ULL;\
    ((u64*)T)[0x20F] = 0xc3b59ab559ecec9aULL;\
    ((u64*)T)[0x210] = 0x05cf45cf408f8f45ULL;\
    ((u64*)T)[0x211] = 0x3ebc9dbca31f1f9dULL;\
    ((u64*)T)[0x212] = 0x09c040c049898940ULL;\
    ((u64*)T)[0x213] = 0xef92879268fafa87ULL;\
    ((u64*)T)[0x214] = 0xc53f153fd0efef15ULL;\
    ((u64*)T)[0x215] = 0x7f26eb2694b2b2ebULL;\
    ((u64*)T)[0x216] = 0x0740c940ce8e8ec9ULL;\
    ((u64*)T)[0x217] = 0xed1d0b1de6fbfb0bULL;\
    ((u64*)T)[0x218] = 0x822fec2f6e4141ecULL;\
    ((u64*)T)[0x219] = 0x7da967a91ab3b367ULL;\
    ((u64*)T)[0x21A] = 0xbe1cfd1c435f5ffdULL;\
    ((u64*)T)[0x21B] = 0x8a25ea25604545eaULL;\
    ((u64*)T)[0x21C] = 0x46dabfdaf92323bfULL;\
    ((u64*)T)[0x21D] = 0xa602f702515353f7ULL;\
    ((u64*)T)[0x21E] = 0xd3a196a145e4e496ULL;\
    ((u64*)T)[0x21F] = 0x2ded5bed769b9b5bULL;\
    ((u64*)T)[0x220] = 0xea5dc25d287575c2ULL;\
    ((u64*)T)[0x221] = 0xd9241c24c5e1e11cULL;\
    ((u64*)T)[0x222] = 0x7ae9aee9d43d3daeULL;\
    ((u64*)T)[0x223] = 0x98be6abef24c4c6aULL;\
    ((u64*)T)[0x224] = 0xd8ee5aee826c6c5aULL;\
    ((u64*)T)[0x225] = 0xfcc341c3bd7e7e41ULL;\
    ((u64*)T)[0x226] = 0xf1060206f3f5f502ULL;\
    ((u64*)T)[0x227] = 0x1dd14fd15283834fULL;\
    ((u64*)T)[0x228] = 0xd0e45ce48c68685cULL;\
    ((u64*)T)[0x229] = 0xa207f407565151f4ULL;\
    ((u64*)T)[0x22A] = 0xb95c345c8dd1d134ULL;\
    ((u64*)T)[0x22B] = 0xe9180818e1f9f908ULL;\
    ((u64*)T)[0x22C] = 0xdfae93ae4ce2e293ULL;\
    ((u64*)T)[0x22D] = 0x4d9573953eabab73ULL;\
    ((u64*)T)[0x22E] = 0xc4f553f597626253ULL;\
    ((u64*)T)[0x22F] = 0x54413f416b2a2a3fULL;\
    ((u64*)T)[0x230] = 0x10140c141c08080cULL;\
    ((u64*)T)[0x231] = 0x31f652f663959552ULL;\
    ((u64*)T)[0x232] = 0x8caf65afe9464665ULL;\
    ((u64*)T)[0x233] = 0x21e25ee27f9d9d5eULL;\
    ((u64*)T)[0x234] = 0x6078287848303028ULL;\
    ((u64*)T)[0x235] = 0x6ef8a1f8cf3737a1ULL;\
    ((u64*)T)[0x236] = 0x14110f111b0a0a0fULL;\
    ((u64*)T)[0x237] = 0x5ec4b5c4eb2f2fb5ULL;\
    ((u64*)T)[0x238] = 0x1c1b091b150e0e09ULL;\
    ((u64*)T)[0x239] = 0x485a365a7e242436ULL;\
    ((u64*)T)[0x23A] = 0x36b69bb6ad1b1b9bULL;\
    ((u64*)T)[0x23B] = 0xa5473d4798dfdf3dULL;\
    ((u64*)T)[0x23C] = 0x816a266aa7cdcd26ULL;\
    ((u64*)T)[0x23D] = 0x9cbb69bbf54e4e69ULL;\
    ((u64*)T)[0x23E] = 0xfe4ccd4c337f7fcdULL;\
    ((u64*)T)[0x23F] = 0xcfba9fba50eaea9fULL;\
    ((u64*)T)[0x240] = 0x242d1b2d3f12121bULL;\
    ((u64*)T)[0x241] = 0x3ab99eb9a41d1d9eULL;\
    ((u64*)T)[0x242] = 0xb09c749cc4585874ULL;\
    ((u64*)T)[0x243] = 0x68722e724634342eULL;\
    ((u64*)T)[0x244] = 0x6c772d774136362dULL;\
    ((u64*)T)[0x245] = 0xa3cdb2cd11dcdcb2ULL;\
    ((u64*)T)[0x246] = 0x7329ee299db4b4eeULL;\
    ((u64*)T)[0x247] = 0xb616fb164d5b5bfbULL;\
    ((u64*)T)[0x248] = 0x5301f601a5a4a4f6ULL;\
    ((u64*)T)[0x249] = 0xecd74dd7a176764dULL;\
    ((u64*)T)[0x24A] = 0x75a361a314b7b761ULL;\
    ((u64*)T)[0x24B] = 0xfa49ce49347d7dceULL;\
    ((u64*)T)[0x24C] = 0xa48d7b8ddf52527bULL;\
    ((u64*)T)[0x24D] = 0xa1423e429fdddd3eULL;\
    ((u64*)T)[0x24E] = 0xbc937193cd5e5e71ULL;\
    ((u64*)T)[0x24F] = 0x26a297a2b1131397ULL;\
    ((u64*)T)[0x250] = 0x5704f504a2a6a6f5ULL;\
    ((u64*)T)[0x251] = 0x69b868b801b9b968ULL;\
    ((u64*)T)[0x252] = 0x0000000000000000ULL;\
    ((u64*)T)[0x253] = 0x99742c74b5c1c12cULL;\
    ((u64*)T)[0x254] = 0x80a060a0e0404060ULL;\
    ((u64*)T)[0x255] = 0xdd211f21c2e3e31fULL;\
    ((u64*)T)[0x256] = 0xf243c8433a7979c8ULL;\
    ((u64*)T)[0x257] = 0x772ced2c9ab6b6edULL;\
    ((u64*)T)[0x258] = 0xb3d9bed90dd4d4beULL;\
    ((u64*)T)[0x259] = 0x01ca46ca478d8d46ULL;\
    ((u64*)T)[0x25A] = 0xce70d970176767d9ULL;\
    ((u64*)T)[0x25B] = 0xe4dd4bddaf72724bULL;\
    ((u64*)T)[0x25C] = 0x3379de79ed9494deULL;\
    ((u64*)T)[0x25D] = 0x2b67d467ff9898d4ULL;\
    ((u64*)T)[0x25E] = 0x7b23e82393b0b0e8ULL;\
    ((u64*)T)[0x25F] = 0x11de4ade5b85854aULL;\
    ((u64*)T)[0x260] = 0x6dbd6bbd06bbbb6bULL;\
    ((u64*)T)[0x261] = 0x917e2a7ebbc5c52aULL;\
    ((u64*)T)[0x262] = 0x9e34e5347b4f4fe5ULL;\
    ((u64*)T)[0x263] = 0xc13a163ad7eded16ULL;\
    ((u64*)T)[0x264] = 0x1754c554d28686c5ULL;\
    ((u64*)T)[0x265] = 0x2f62d762f89a9ad7ULL;\
    ((u64*)T)[0x266] = 0xccff55ff99666655ULL;\
    ((u64*)T)[0x267] = 0x22a794a7b6111194ULL;\
    ((u64*)T)[0x268] = 0x0f4acf4ac08a8acfULL;\
    ((u64*)T)[0x269] = 0xc9301030d9e9e910ULL;\
    ((u64*)T)[0x26A] = 0x080a060a0e040406ULL;\
    ((u64*)T)[0x26B] = 0xe798819866fefe81ULL;\
    ((u64*)T)[0x26C] = 0x5b0bf00baba0a0f0ULL;\
    ((u64*)T)[0x26D] = 0xf0cc44ccb4787844ULL;\
    ((u64*)T)[0x26E] = 0x4ad5bad5f02525baULL;\
    ((u64*)T)[0x26F] = 0x963ee33e754b4be3ULL;\
    ((u64*)T)[0x270] = 0x5f0ef30eaca2a2f3ULL;\
    ((u64*)T)[0x271] = 0xba19fe19445d5dfeULL;\
    ((u64*)T)[0x272] = 0x1b5bc05bdb8080c0ULL;\
    ((u64*)T)[0x273] = 0x0a858a858005058aULL;\
    ((u64*)T)[0x274] = 0x7eecadecd33f3fadULL;\
    ((u64*)T)[0x275] = 0x42dfbcdffe2121bcULL;\
    ((u64*)T)[0x276] = 0xe0d848d8a8707048ULL;\
    ((u64*)T)[0x277] = 0xf90c040cfdf1f104ULL;\
    ((u64*)T)[0x278] = 0xc67adf7a196363dfULL;\
    ((u64*)T)[0x279] = 0xee58c1582f7777c1ULL;\
    ((u64*)T)[0x27A] = 0x459f759f30afaf75ULL;\
    ((u64*)T)[0x27B] = 0x84a563a5e7424263ULL;\
    ((u64*)T)[0x27C] = 0x4050305070202030ULL;\
    ((u64*)T)[0x27D] = 0xd12e1a2ecbe5e51aULL;\
    ((u64*)T)[0x27E] = 0xe1120e12effdfd0eULL;\
    ((u64*)T)[0x27F] = 0x65b76db708bfbf6dULL;\
    ((u64*)T)[0x280] = 0x19d44cd45581814cULL;\
    ((u64*)T)[0x281] = 0x303c143c24181814ULL;\
    ((u64*)T)[0x282] = 0x4c5f355f79262635ULL;\
    ((u64*)T)[0x283] = 0x9d712f71b2c3c32fULL;\
    ((u64*)T)[0x284] = 0x6738e13886bebee1ULL;\
    ((u64*)T)[0x285] = 0x6afda2fdc83535a2ULL;\
    ((u64*)T)[0x286] = 0x0b4fcc4fc78888ccULL;\
    ((u64*)T)[0x287] = 0x5c4b394b652e2e39ULL;\
    ((u64*)T)[0x288] = 0x3df957f96a939357ULL;\
    ((u64*)T)[0x289] = 0xaa0df20d585555f2ULL;\
    ((u64*)T)[0x28A] = 0xe39d829d61fcfc82ULL;\
    ((u64*)T)[0x28B] = 0xf4c947c9b37a7a47ULL;\
    ((u64*)T)[0x28C] = 0x8befacef27c8c8acULL;\
    ((u64*)T)[0x28D] = 0x6f32e73288babae7ULL;\
    ((u64*)T)[0x28E] = 0x647d2b7d4f32322bULL;\
    ((u64*)T)[0x28F] = 0xd7a495a442e6e695ULL;\
    ((u64*)T)[0x290] = 0x9bfba0fb3bc0c0a0ULL;\
    ((u64*)T)[0x291] = 0x32b398b3aa191998ULL;\
    ((u64*)T)[0x292] = 0x2768d168f69e9ed1ULL;\
    ((u64*)T)[0x293] = 0x5d817f8122a3a37fULL;\
    ((u64*)T)[0x294] = 0x88aa66aaee444466ULL;\
    ((u64*)T)[0x295] = 0xa8827e82d654547eULL;\
    ((u64*)T)[0x296] = 0x76e6abe6dd3b3babULL;\
    ((u64*)T)[0x297] = 0x169e839e950b0b83ULL;\
    ((u64*)T)[0x298] = 0x0345ca45c98c8ccaULL;\
    ((u64*)T)[0x299] = 0x957b297bbcc7c729ULL;\
    ((u64*)T)[0x29A] = 0xd66ed36e056b6bd3ULL;\
    ((u64*)T)[0x29B] = 0x50443c446c28283cULL;\
    ((u64*)T)[0x29C] = 0x558b798b2ca7a779ULL;\
    ((u64*)T)[0x29D] = 0x633de23d81bcbce2ULL;\
    ((u64*)T)[0x29E] = 0x2c271d273116161dULL;\
    ((u64*)T)[0x29F] = 0x419a769a37adad76ULL;\
    ((u64*)T)[0x2A0] = 0xad4d3b4d96dbdb3bULL;\
    ((u64*)T)[0x2A1] = 0xc8fa56fa9e646456ULL;\
    ((u64*)T)[0x2A2] = 0xe8d24ed2a674744eULL;\
    ((u64*)T)[0x2A3] = 0x28221e223614141eULL;\
    ((u64*)T)[0x2A4] = 0x3f76db76e49292dbULL;\
    ((u64*)T)[0x2A5] = 0x181e0a1e120c0c0aULL;\
    ((u64*)T)[0x2A6] = 0x90b46cb4fc48486cULL;\
    ((u64*)T)[0x2A7] = 0x6b37e4378fb8b8e4ULL;\
    ((u64*)T)[0x2A8] = 0x25e75de7789f9f5dULL;\
    ((u64*)T)[0x2A9] = 0x61b26eb20fbdbd6eULL;\
    ((u64*)T)[0x2AA] = 0x862aef2a694343efULL;\
    ((u64*)T)[0x2AB] = 0x93f1a6f135c4c4a6ULL;\
    ((u64*)T)[0x2AC] = 0x72e3a8e3da3939a8ULL;\
    ((u64*)T)[0x2AD] = 0x62f7a4f7c63131a4ULL;\
    ((u64*)T)[0x2AE] = 0xbd5937598ad3d337ULL;\
    ((u64*)T)[0x2AF] = 0xff868b8674f2f28bULL;\
    ((u64*)T)[0x2B0] = 0xb156325683d5d532ULL;\
    ((u64*)T)[0x2B1] = 0x0dc543c54e8b8b43ULL;\
    ((u64*)T)[0x2B2] = 0xdceb59eb856e6e59ULL;\
    ((u64*)T)[0x2B3] = 0xafc2b7c218dadab7ULL;\
    ((u64*)T)[0x2B4] = 0x028f8c8f8e01018cULL;\
    ((u64*)T)[0x2B5] = 0x79ac64ac1db1b164ULL;\
    ((u64*)T)[0x2B6] = 0x236dd26df19c9cd2ULL;\
    ((u64*)T)[0x2B7] = 0x923be03b724949e0ULL;\
    ((u64*)T)[0x2B8] = 0xabc7b4c71fd8d8b4ULL;\
    ((u64*)T)[0x2B9] = 0x4315fa15b9acacfaULL;\
    ((u64*)T)[0x2BA] = 0xfd090709faf3f307ULL;\
    ((u64*)T)[0x2BB] = 0x856f256fa0cfcf25ULL;\
    ((u64*)T)[0x2BC] = 0x8feaafea20cacaafULL;\
    ((u64*)T)[0x2BD] = 0xf3898e897df4f48eULL;\
    ((u64*)T)[0x2BE] = 0x8e20e920674747e9ULL;\
    ((u64*)T)[0x2BF] = 0x2028182838101018ULL;\
    ((u64*)T)[0x2C0] = 0xde64d5640b6f6fd5ULL;\
    ((u64*)T)[0x2C1] = 0xfb83888373f0f088ULL;\
    ((u64*)T)[0x2C2] = 0x94b16fb1fb4a4a6fULL;\
    ((u64*)T)[0x2C3] = 0xb8967296ca5c5c72ULL;\
    ((u64*)T)[0x2C4] = 0x706c246c54383824ULL;\
    ((u64*)T)[0x2C5] = 0xae08f1085f5757f1ULL;\
    ((u64*)T)[0x2C6] = 0xe652c752217373c7ULL;\
    ((u64*)T)[0x2C7] = 0x35f351f364979751ULL;\
    ((u64*)T)[0x2C8] = 0x8d652365aecbcb23ULL;\
    ((u64*)T)[0x2C9] = 0x59847c8425a1a17cULL;\
    ((u64*)T)[0x2CA] = 0xcbbf9cbf57e8e89cULL;\
    ((u64*)T)[0x2CB] = 0x7c6321635d3e3e21ULL;\
    ((u64*)T)[0x2CC] = 0x377cdd7cea9696ddULL;\
    ((u64*)T)[0x2CD] = 0xc27fdc7f1e6161dcULL;\
    ((u64*)T)[0x2CE] = 0x1a9186919c0d0d86ULL;\
    ((u64*)T)[0x2CF] = 0x1e9485949b0f0f85ULL;\
    ((u64*)T)[0x2D0] = 0xdbab90ab4be0e090ULL;\
    ((u64*)T)[0x2D1] = 0xf8c642c6ba7c7c42ULL;\
    ((u64*)T)[0x2D2] = 0xe257c457267171c4ULL;\
    ((u64*)T)[0x2D3] = 0x83e5aae529ccccaaULL;\
    ((u64*)T)[0x2D4] = 0x3b73d873e39090d8ULL;\
    ((u64*)T)[0x2D5] = 0x0c0f050f09060605ULL;\
    ((u64*)T)[0x2D6] = 0xf5030103f4f7f701ULL;\
    ((u64*)T)[0x2D7] = 0x383612362a1c1c12ULL;\
    ((u64*)T)[0x2D8] = 0x9ffea3fe3cc2c2a3ULL;\
    ((u64*)T)[0x2D9] = 0xd4e15fe18b6a6a5fULL;\
    ((u64*)T)[0x2DA] = 0x4710f910beaeaef9ULL;\
    ((u64*)T)[0x2DB] = 0xd26bd06b026969d0ULL;\
    ((u64*)T)[0x2DC] = 0x2ea891a8bf171791ULL;\
    ((u64*)T)[0x2DD] = 0x29e858e871999958ULL;\
    ((u64*)T)[0x2DE] = 0x74692769533a3a27ULL;\
    ((u64*)T)[0x2DF] = 0x4ed0b9d0f72727b9ULL;\
    ((u64*)T)[0x2E0] = 0xa948384891d9d938ULL;\
    ((u64*)T)[0x2E1] = 0xcd351335deebeb13ULL;\
    ((u64*)T)[0x2E2] = 0x56ceb3cee52b2bb3ULL;\
    ((u64*)T)[0x2E3] = 0x4455335577222233ULL;\
    ((u64*)T)[0x2E4] = 0xbfd6bbd604d2d2bbULL;\
    ((u64*)T)[0x2E5] = 0x4990709039a9a970ULL;\
    ((u64*)T)[0x2E6] = 0x0e80898087070789ULL;\
    ((u64*)T)[0x2E7] = 0x66f2a7f2c13333a7ULL;\
    ((u64*)T)[0x2E8] = 0x5ac1b6c1ec2d2db6ULL;\
    ((u64*)T)[0x2E9] = 0x786622665a3c3c22ULL;\
    ((u64*)T)[0x2EA] = 0x2aad92adb8151592ULL;\
    ((u64*)T)[0x2EB] = 0x89602060a9c9c920ULL;\
    ((u64*)T)[0x2EC] = 0x15db49db5c878749ULL;\
    ((u64*)T)[0x2ED] = 0x4f1aff1ab0aaaaffULL;\
    ((u64*)T)[0x2EE] = 0xa0887888d8505078ULL;\
    ((u64*)T)[0x2EF] = 0x518e7a8e2ba5a57aULL;\
    ((u64*)T)[0x2F0] = 0x068a8f8a8903038fULL;\
    ((u64*)T)[0x2F1] = 0xb213f8134a5959f8ULL;\
    ((u64*)T)[0x2F2] = 0x129b809b92090980ULL;\
    ((u64*)T)[0x2F3] = 0x34391739231a1a17ULL;\
    ((u64*)T)[0x2F4] = 0xca75da75106565daULL;\
    ((u64*)T)[0x2F5] = 0xb553315384d7d731ULL;\
    ((u64*)T)[0x2F6] = 0x1351c651d58484c6ULL;\
    ((u64*)T)[0x2F7] = 0xbbd3b8d303d0d0b8ULL;\
    ((u64*)T)[0x2F8] = 0x1f5ec35edc8282c3ULL;\
    ((u64*)T)[0x2F9] = 0x52cbb0cbe22929b0ULL;\
    ((u64*)T)[0x2FA] = 0xb4997799c35a5a77ULL;\
    ((u64*)T)[0x2FB] = 0x3c3311332d1e1e11ULL;\
    ((u64*)T)[0x2FC] = 0xf646cb463d7b7bcbULL;\
    ((u64*)T)[0x2FD] = 0x4b1ffc1fb7a8a8fcULL;\
    ((u64*)T)[0x2FE] = 0xda61d6610c6d6dd6ULL;\
    ((u64*)T)[0x2FF] = 0x584e3a4e622c2c3aULL;\
    ((u64*)T)[0x300] = 0xf4a5f432c6c6a597ULL;\
    ((u64*)T)[0x301] = 0x9784976ff8f884ebULL;\
    ((u64*)T)[0x302] = 0xb099b05eeeee99c7ULL;\
    ((u64*)T)[0x303] = 0x8c8d8c7af6f68df7ULL;\
    ((u64*)T)[0x304] = 0x170d17e8ffff0de5ULL;\
    ((u64*)T)[0x305] = 0xdcbddc0ad6d6bdb7ULL;\
    ((u64*)T)[0x306] = 0xc8b1c816dedeb1a7ULL;\
    ((u64*)T)[0x307] = 0xfc54fc6d91915439ULL;\
    ((u64*)T)[0x308] = 0xf050f090606050c0ULL;\
    ((u64*)T)[0x309] = 0x0503050702020304ULL;\
    ((u64*)T)[0x30A] = 0xe0a9e02ececea987ULL;\
    ((u64*)T)[0x30B] = 0x877d87d156567dacULL;\
    ((u64*)T)[0x30C] = 0x2b192bcce7e719d5ULL;\
    ((u64*)T)[0x30D] = 0xa662a613b5b56271ULL;\
    ((u64*)T)[0x30E] = 0x31e6317c4d4de69aULL;\
    ((u64*)T)[0x30F] = 0xb59ab559ecec9ac3ULL;\
    ((u64*)T)[0x310] = 0xcf45cf408f8f4505ULL;\
    ((u64*)T)[0x311] = 0xbc9dbca31f1f9d3eULL;\
    ((u64*)T)[0x312] = 0xc040c04989894009ULL;\
    ((u64*)T)[0x313] = 0x92879268fafa87efULL;\
    ((u64*)T)[0x314] = 0x3f153fd0efef15c5ULL;\
    ((u64*)T)[0x315] = 0x26eb2694b2b2eb7fULL;\
    ((u64*)T)[0x316] = 0x40c940ce8e8ec907ULL;\
    ((u64*)T)[0x317] = 0x1d0b1de6fbfb0bedULL;\
    ((u64*)T)[0x318] = 0x2fec2f6e4141ec82ULL;\
    ((u64*)T)[0x319] = 0xa967a91ab3b3677dULL;\
    ((u64*)T)[0x31A] = 0x1cfd1c435f5ffdbeULL;\
    ((u64*)T)[0x31B] = 0x25ea25604545ea8aULL;\
    ((u64*)T)[0x31C] = 0xdabfdaf92323bf46ULL;\
    ((u64*)T)[0x31D] = 0x02f702515353f7a6ULL;\
    ((u64*)T)[0x31E] = 0xa196a145e4e496d3ULL;\
    ((u64*)T)[0x31F] = 0xed5bed769b9b5b2dULL;\
    ((u64*)T)[0x320] = 0x5dc25d287575c2eaULL;\
    ((u64*)T)[0x321] = 0x241c24c5e1e11cd9ULL;\
    ((u64*)T)[0x322] = 0xe9aee9d43d3dae7aULL;\
    ((u64*)T)[0x323] = 0xbe6abef24c4c6a98ULL;\
    ((u64*)T)[0x324] = 0xee5aee826c6c5ad8ULL;\
    ((u64*)T)[0x325] = 0xc341c3bd7e7e41fcULL;\
    ((u64*)T)[0x326] = 0x060206f3f5f502f1ULL;\
    ((u64*)T)[0x327] = 0xd14fd15283834f1dULL;\
    ((u64*)T)[0x328] = 0xe45ce48c68685cd0ULL;\
    ((u64*)T)[0x329] = 0x07f407565151f4a2ULL;\
    ((u64*)T)[0x32A] = 0x5c345c8dd1d134b9ULL;\
    ((u64*)T)[0x32B] = 0x180818e1f9f908e9ULL;\
    ((u64*)T)[0x32C] = 0xae93ae4ce2e293dfULL;\
    ((u64*)T)[0x32D] = 0x9573953eabab734dULL;\
    ((u64*)T)[0x32E] = 0xf553f597626253c4ULL;\
    ((u64*)T)[0x32F] = 0x413f416b2a2a3f54ULL;\
    ((u64*)T)[0x330] = 0x140c141c08080c10ULL;\
    ((u64*)T)[0x331] = 0xf652f66395955231ULL;\
    ((u64*)T)[0x332] = 0xaf65afe94646658cULL;\
    ((u64*)T)[0x333] = 0xe25ee27f9d9d5e21ULL;\
    ((u64*)T)[0x334] = 0x7828784830302860ULL;\
    ((u64*)T)[0x335] = 0xf8a1f8cf3737a16eULL;\
    ((u64*)T)[0x336] = 0x110f111b0a0a0f14ULL;\
    ((u64*)T)[0x337] = 0xc4b5c4eb2f2fb55eULL;\
    ((u64*)T)[0x338] = 0x1b091b150e0e091cULL;\
    ((u64*)T)[0x339] = 0x5a365a7e24243648ULL;\
    ((u64*)T)[0x33A] = 0xb69bb6ad1b1b9b36ULL;\
    ((u64*)T)[0x33B] = 0x473d4798dfdf3da5ULL;\
    ((u64*)T)[0x33C] = 0x6a266aa7cdcd2681ULL;\
    ((u64*)T)[0x33D] = 0xbb69bbf54e4e699cULL;\
    ((u64*)T)[0x33E] = 0x4ccd4c337f7fcdfeULL;\
    ((u64*)T)[0x33F] = 0xba9fba50eaea9fcfULL;\
    ((u64*)T)[0x340] = 0x2d1b2d3f12121b24ULL;\
    ((u64*)T)[0x341] = 0xb99eb9a41d1d9e3aULL;\
    ((u64*)T)[0x342] = 0x9c749cc4585874b0ULL;\
    ((u64*)T)[0x343] = 0x722e724634342e68ULL;\
    ((u64*)T)[0x344] = 0x772d774136362d6cULL;\
    ((u64*)T)[0x345] = 0xcdb2cd11dcdcb2a3ULL;\
    ((u64*)T)[0x346] = 0x29ee299db4b4ee73ULL;\
    ((u64*)T)[0x347] = 0x16fb164d5b5bfbb6ULL;\
    ((u64*)T)[0x348] = 0x01f601a5a4a4f653ULL;\
    ((u64*)T)[0x349] = 0xd74dd7a176764decULL;\
    ((u64*)T)[0x34A] = 0xa361a314b7b76175ULL;\
    ((u64*)T)[0x34B] = 0x49ce49347d7dcefaULL;\
    ((u64*)T)[0x34C] = 0x8d7b8ddf52527ba4ULL;\
    ((u64*)T)[0x34D] = 0x423e429fdddd3ea1ULL;\
    ((u64*)T)[0x34E] = 0x937193cd5e5e71bcULL;\
    ((u64*)T)[0x34F] = 0xa297a2b113139726ULL;\
    ((u64*)T)[0x350] = 0x04f504a2a6a6f557ULL;\
    ((u64*)T)[0x351] = 0xb868b801b9b96869ULL;\
    ((u64*)T)[0x352] = 0x0000000000000000ULL;\
    ((u64*)T)[0x353] = 0x742c74b5c1c12c99ULL;\
    ((u64*)T)[0x354] = 0xa060a0e040406080ULL;\
    ((u64*)T)[0x355] = 0x211f21c2e3e31fddULL;\
    ((u64*)T)[0x356] = 0x43c8433a7979c8f2ULL;\
    ((u64*)T)[0x357] = 0x2ced2c9ab6b6ed77ULL;\
    ((u64*)T)[0x358] = 0xd9bed90dd4d4beb3ULL;\
    ((u64*)T)[0x359] = 0xca46ca478d8d4601ULL;\
    ((u64*)T)[0x35A] = 0x70d970176767d9ceULL;\
    ((u64*)T)[0x35B] = 0xdd4bddaf72724be4ULL;\
    ((u64*)T)[0x35C] = 0x79de79ed9494de33ULL;\
    ((u64*)T)[0x35D] = 0x67d467ff9898d42bULL;\
    ((u64*)T)[0x35E] = 0x23e82393b0b0e87bULL;\
    ((u64*)T)[0x35F] = 0xde4ade5b85854a11ULL;\
    ((u64*)T)[0x360] = 0xbd6bbd06bbbb6b6dULL;\
    ((u64*)T)[0x361] = 0x7e2a7ebbc5c52a91ULL;\
    ((u64*)T)[0x362] = 0x34e5347b4f4fe59eULL;\
    ((u64*)T)[0x363] = 0x3a163ad7eded16c1ULL;\
    ((u64*)T)[0x364] = 0x54c554d28686c517ULL;\
    ((u64*)T)[0x365] = 0x62d762f89a9ad72fULL;\
    ((u64*)T)[0x366] = 0xff55ff99666655ccULL;\
    ((u64*)T)[0x367] = 0xa794a7b611119422ULL;\
    ((u64*)T)[0x368] = 0x4acf4ac08a8acf0fULL;\
    ((u64*)T)[0x369] = 0x301030d9e9e910c9ULL;\
    ((u64*)T)[0x36A] = 0x0a060a0e04040608ULL;\
    ((u64*)T)[0x36B] = 0x98819866fefe81e7ULL;\
    ((u64*)T)[0x36C] = 0x0bf00baba0a0f05bULL;\
    ((u64*)T)[0x36D] = 0xcc44ccb4787844f0ULL;\
    ((u64*)T)[0x36E] = 0xd5bad5f02525ba4aULL;\
    ((u64*)T)[0x36F] = 0x3ee33e754b4be396ULL;\
    ((u64*)T)[0x370] = 0x0ef30eaca2a2f35fULL;\
    ((u64*)T)[0x371] = 0x19fe19445d5dfebaULL;\
    ((u64*)T)[0x372] = 0x5bc05bdb8080c01bULL;\
    ((u64*)T)[0x373] = 0x858a858005058a0aULL;\
    ((u64*)T)[0x374] = 0xecadecd33f3fad7eULL;\
    ((u64*)T)[0x375] = 0xdfbcdffe2121bc42ULL;\
    ((u64*)T)[0x376] = 0xd848d8a8707048e0ULL;\
    ((u64*)T)[0x377] = 0x0c040cfdf1f104f9ULL;\
    ((u64*)T)[0x378] = 0x7adf7a196363dfc6ULL;\
    ((u64*)T)[0x379] = 0x58c1582f7777c1eeULL;\
    ((u64*)T)[0x37A] = 0x9f759f30afaf7545ULL;\
    ((u64*)T)[0x37B] = 0xa563a5e742426384ULL;\
    ((u64*)T)[0x37C] = 0x5030507020203040ULL;\
    ((u64*)T)[0x37D] = 0x2e1a2ecbe5e51ad1ULL;\
    ((u64*)T)[0x37E] = 0x120e12effdfd0ee1ULL;\
    ((u64*)T)[0x37F] = 0xb76db708bfbf6d65ULL;\
    ((u64*)T)[0x380] = 0xd44cd45581814c19ULL;\
    ((u64*)T)[0x381] = 0x3c143c2418181430ULL;\
    ((u64*)T)[0x382] = 0x5f355f792626354cULL;\
    ((u64*)T)[0x383] = 0x712f71b2c3c32f9dULL;\
    ((u64*)T)[0x384] = 0x38e13886bebee167ULL;\
    ((u64*)T)[0x385] = 0xfda2fdc83535a26aULL;\
    ((u64*)T)[0x386] = 0x4fcc4fc78888cc0bULL;\
    ((u64*)T)[0x387] = 0x4b394b652e2e395cULL;\
    ((u64*)T)[0x388] = 0xf957f96a9393573dULL;\
    ((u64*)T)[0x389] = 0x0df20d585555f2aaULL;\
    ((u64*)T)[0x38A] = 0x9d829d61fcfc82e3ULL;\
    ((u64*)T)[0x38B] = 0xc947c9b37a7a47f4ULL;\
    ((u64*)T)[0x38C] = 0xefacef27c8c8ac8bULL;\
    ((u64*)T)[0x38D] = 0x32e73288babae76fULL;\
    ((u64*)T)[0x38E] = 0x7d2b7d4f32322b64ULL;\
    ((u64*)T)[0x38F] = 0xa495a442e6e695d7ULL;\
    ((u64*)T)[0x390] = 0xfba0fb3bc0c0a09bULL;\
    ((u64*)T)[0x391] = 0xb398b3aa19199832ULL;\
    ((u64*)T)[0x392] = 0x68d168f69e9ed127ULL;\
    ((u64*)T)[0x393] = 0x817f8122a3a37f5dULL;\
    ((u64*)T)[0x394] = 0xaa66aaee44446688ULL;\
    ((u64*)T)[0x395] = 0x827e82d654547ea8ULL;\
    ((u64*)T)[0x396] = 0xe6abe6dd3b3bab76ULL;\
    ((u64*)T)[0x397] = 0x9e839e950b0b8316ULL;\
    ((u64*)T)[0x398] = 0x45ca45c98c8cca03ULL;\
    ((u64*)T)[0x399] = 0x7b297bbcc7c72995ULL;\
    ((u64*)T)[0x39A] = 0x6ed36e056b6bd3d6ULL;\
    ((u64*)T)[0x39B] = 0x443c446c28283c50ULL;\
    ((u64*)T)[0x39C] = 0x8b798b2ca7a77955ULL;\
    ((u64*)T)[0x39D] = 0x3de23d81bcbce263ULL;\
    ((u64*)T)[0x39E] = 0x271d273116161d2cULL;\
    ((u64*)T)[0x39F] = 0x9a769a37adad7641ULL;\
    ((u64*)T)[0x3A0] = 0x4d3b4d96dbdb3badULL;\
    ((u64*)T)[0x3A1] = 0xfa56fa9e646456c8ULL;\
    ((u64*)T)[0x3A2] = 0xd24ed2a674744ee8ULL;\
    ((u64*)T)[0x3A3] = 0x221e223614141e28ULL;\
    ((u64*)T)[0x3A4] = 0x76db76e49292db3fULL;\
    ((u64*)T)[0x3A5] = 0x1e0a1e120c0c0a18ULL;\
    ((u64*)T)[0x3A6] = 0xb46cb4fc48486c90ULL;\
    ((u64*)T)[0x3A7] = 0x37e4378fb8b8e46bULL;\
    ((u64*)T)[0x3A8] = 0xe75de7789f9f5d25ULL;\
    ((u64*)T)[0x3A9] = 0xb26eb20fbdbd6e61ULL;\
    ((u64*)T)[0x3AA] = 0x2aef2a694343ef86ULL;\
    ((u64*)T)[0x3AB] = 0xf1a6f135c4c4a693ULL;\
    ((u64*)T)[0x3AC] = 0xe3a8e3da3939a872ULL;\
    ((u64*)T)[0x3AD] = 0xf7a4f7c63131a462ULL;\
    ((u64*)T)[0x3AE] = 0x5937598ad3d337bdULL;\
    ((u64*)T)[0x3AF] = 0x868b8674f2f28bffULL;\
    ((u64*)T)[0x3B0] = 0x56325683d5d532b1ULL;\
    ((u64*)T)[0x3B1] = 0xc543c54e8b8b430dULL;\
    ((u64*)T)[0x3B2] = 0xeb59eb856e6e59dcULL;\
    ((u64*)T)[0x3B3] = 0xc2b7c218dadab7afULL;\
    ((u64*)T)[0x3B4] = 0x8f8c8f8e01018c02ULL;\
    ((u64*)T)[0x3B5] = 0xac64ac1db1b16479ULL;\
    ((u64*)T)[0x3B6] = 0x6dd26df19c9cd223ULL;\
    ((u64*)T)[0x3B7] = 0x3be03b724949e092ULL;\
    ((u64*)T)[0x3B8] = 0xc7b4c71fd8d8b4abULL;\
    ((u64*)T)[0x3B9] = 0x15fa15b9acacfa43ULL;\
    ((u64*)T)[0x3BA] = 0x090709faf3f307fdULL;\
    ((u64*)T)[0x3BB] = 0x6f256fa0cfcf2585ULL;\
    ((u64*)T)[0x3BC] = 0xeaafea20cacaaf8fULL;\
    ((u64*)T)[0x3BD] = 0x898e897df4f48ef3ULL;\
    ((u64*)T)[0x3BE] = 0x20e920674747e98eULL;\
    ((u64*)T)[0x3BF] = 0x2818283810101820ULL;\
    ((u64*)T)[0x3C0] = 0x64d5640b6f6fd5deULL;\
    ((u64*)T)[0x3C1] = 0x83888373f0f088fbULL;\
    ((u64*)T)[0x3C2] = 0xb16fb1fb4a4a6f94ULL;\
    ((u64*)T)[0x3C3] = 0x967296ca5c5c72b8ULL;\
    ((u64*)T)[0x3C4] = 0x6c246c5438382470ULL;\
    ((u64*)T)[0x3C5] = 0x08f1085f5757f1aeULL;\
    ((u64*)T)[0x3C6] = 0x52c752217373c7e6ULL;\
    ((u64*)T)[0x3C7] = 0xf351f36497975135ULL;\
    ((u64*)T)[0x3C8] = 0x652365aecbcb238dULL;\
    ((u64*)T)[0x3C9] = 0x847c8425a1a17c59ULL;\
    ((u64*)T)[0x3CA] = 0xbf9cbf57e8e89ccbULL;\
    ((u64*)T)[0x3CB] = 0x6321635d3e3e217cULL;\
    ((u64*)T)[0x3CC] = 0x7cdd7cea9696dd37ULL;\
    ((u64*)T)[0x3CD] = 0x7fdc7f1e6161dcc2ULL;\
    ((u64*)T)[0x3CE] = 0x9186919c0d0d861aULL;\
    ((u64*)T)[0x3CF] = 0x9485949b0f0f851eULL;\
    ((u64*)T)[0x3D0] = 0xab90ab4be0e090dbULL;\
    ((u64*)T)[0x3D1] = 0xc642c6ba7c7c42f8ULL;\
    ((u64*)T)[0x3D2] = 0x57c457267171c4e2ULL;\
    ((u64*)T)[0x3D3] = 0xe5aae529ccccaa83ULL;\
    ((u64*)T)[0x3D4] = 0x73d873e39090d83bULL;\
    ((u64*)T)[0x3D5] = 0x0f050f090606050cULL;\
    ((u64*)T)[0x3D6] = 0x030103f4f7f701f5ULL;\
    ((u64*)T)[0x3D7] = 0x3612362a1c1c1238ULL;\
    ((u64*)T)[0x3D8] = 0xfea3fe3cc2c2a39fULL;\
    ((u64*)T)[0x3D9] = 0xe15fe18b6a6a5fd4ULL;\
    ((u64*)T)[0x3DA] = 0x10f910beaeaef947ULL;\
    ((u64*)T)[0x3DB] = 0x6bd06b026969d0d2ULL;\
    ((u64*)T)[0x3DC] = 0xa891a8bf1717912eULL;\
    ((u64*)T)[0x3DD] = 0xe858e87199995829ULL;\
    ((u64*)T)[0x3DE] = 0x692769533a3a2774ULL;\
    ((u64*)T)[0x3DF] = 0xd0b9d0f72727b94eULL;\
    ((u64*)T)[0x3E0] = 0x48384891d9d938a9ULL;\
    ((u64*)T)[0x3E1] = 0x351335deebeb13cdULL;\
    ((u64*)T)[0x3E2] = 0xceb3cee52b2bb356ULL;\
    ((u64*)T)[0x3E3] = 0x5533557722223344ULL;\
    ((u64*)T)[0x3E4] = 0xd6bbd604d2d2bbbfULL;\
    ((u64*)T)[0x3E5] = 0x90709039a9a97049ULL;\
    ((u64*)T)[0x3E6] = 0x808980870707890eULL;\
    ((u64*)T)[0x3E7] = 0xf2a7f2c13333a766ULL;\
    ((u64*)T)[0x3E8] = 0xc1b6c1ec2d2db65aULL;\
    ((u64*)T)[0x3E9] = 0x6622665a3c3c2278ULL;\
    ((u64*)T)[0x3EA] = 0xad92adb81515922aULL;\
    ((u64*)T)[0x3EB] = 0x602060a9c9c92089ULL;\
    ((u64*)T)[0x3EC] = 0xdb49db5c87874915ULL;\
    ((u64*)T)[0x3ED] = 0x1aff1ab0aaaaff4fULL;\
    ((u64*)T)[0x3EE] = 0x887888d8505078a0ULL;\
    ((u64*)T)[0x3EF] = 0x8e7a8e2ba5a57a51ULL;\
    ((u64*)T)[0x3F0] = 0x8a8f8a8903038f06ULL;\
    ((u64*)T)[0x3F1] = 0x13f8134a5959f8b2ULL;\
    ((u64*)T)[0x3F2] = 0x9b809b9209098012ULL;\
    ((u64*)T)[0x3F3] = 0x391739231a1a1734ULL;\
    ((u64*)T)[0x3F4] = 0x75da75106565dacaULL;\
    ((u64*)T)[0x3F5] = 0x53315384d7d731b5ULL;\
    ((u64*)T)[0x3F6] = 0x51c651d58484c613ULL;\
    ((u64*)T)[0x3F7] = 0xd3b8d303d0d0b8bbULL;\
    ((u64*)T)[0x3F8] = 0x5ec35edc8282c31fULL;\
    ((u64*)T)[0x3F9] = 0xcbb0cbe22929b052ULL;\
    ((u64*)T)[0x3FA] = 0x997799c35a5a77b4ULL;\
    ((u64*)T)[0x3FB] = 0x3311332d1e1e113cULL;\
    ((u64*)T)[0x3FC] = 0x46cb463d7b7bcbf6ULL;\
    ((u64*)T)[0x3FD] = 0x1ffc1fb7a8a8fc4bULL;\
    ((u64*)T)[0x3FE] = 0x61d6610c6d6dd6daULL;\
    ((u64*)T)[0x3FF] = 0x4e3a4e622c2c3a58ULL;\
    ((u64*)T)[0x400] = 0xa5f432c6c6a597f4ULL;\
    ((u64*)T)[0x401] = 0x84976ff8f884eb97ULL;\
    ((u64*)T)[0x402] = 0x99b05eeeee99c7b0ULL;\
    ((u64*)T)[0x403] = 0x8d8c7af6f68df78cULL;\
    ((u64*)T)[0x404] = 0x0d17e8ffff0de517ULL;\
    ((u64*)T)[0x405] = 0xbddc0ad6d6bdb7dcULL;\
    ((u64*)T)[0x406] = 0xb1c816dedeb1a7c8ULL;\
    ((u64*)T)[0x407] = 0x54fc6d91915439fcULL;\
    ((u64*)T)[0x408] = 0x50f090606050c0f0ULL;\
    ((u64*)T)[0x409] = 0x0305070202030405ULL;\
    ((u64*)T)[0x40A] = 0xa9e02ececea987e0ULL;\
    ((u64*)T)[0x40B] = 0x7d87d156567dac87ULL;\
    ((u64*)T)[0x40C] = 0x192bcce7e719d52bULL;\
    ((u64*)T)[0x40D] = 0x62a613b5b56271a6ULL;\
    ((u64*)T)[0x40E] = 0xe6317c4d4de69a31ULL;\
    ((u64*)T)[0x40F] = 0x9ab559ecec9ac3b5ULL;\
    ((u64*)T)[0x410] = 0x45cf408f8f4505cfULL;\
    ((u64*)T)[0x411] = 0x9dbca31f1f9d3ebcULL;\
    ((u64*)T)[0x412] = 0x40c04989894009c0ULL;\
    ((u64*)T)[0x413] = 0x879268fafa87ef92ULL;\
    ((u64*)T)[0x414] = 0x153fd0efef15c53fULL;\
    ((u64*)T)[0x415] = 0xeb2694b2b2eb7f26ULL;\
    ((u64*)T)[0x416] = 0xc940ce8e8ec90740ULL;\
    ((u64*)T)[0x417] = 0x0b1de6fbfb0bed1dULL;\
    ((u64*)T)[0x418] = 0xec2f6e4141ec822fULL;\
    ((u64*)T)[0x419] = 0x67a91ab3b3677da9ULL;\
    ((u64*)T)[0x41A] = 0xfd1c435f5ffdbe1cULL;\
    ((u64*)T)[0x41B] = 0xea25604545ea8a25ULL;\
    ((u64*)T)[0x41C] = 0xbfdaf92323bf46daULL;\
    ((u64*)T)[0x41D] = 0xf702515353f7a602ULL;\
    ((u64*)T)[0x41E] = 0x96a145e4e496d3a1ULL;\
    ((u64*)T)[0x41F] = 0x5bed769b9b5b2dedULL;\
    ((u64*)T)[0x420] = 0xc25d287575c2ea5dULL;\
    ((u64*)T)[0x421] = 0x1c24c5e1e11cd924ULL;\
    ((u64*)T)[0x422] = 0xaee9d43d3dae7ae9ULL;\
    ((u64*)T)[0x423] = 0x6abef24c4c6a98beULL;\
    ((u64*)T)[0x424] = 0x5aee826c6c5ad8eeULL;\
    ((u64*)T)[0x425] = 0x41c3bd7e7e41fcc3ULL;\
    ((u64*)T)[0x426] = 0x0206f3f5f502f106ULL;\
    ((u64*)T)[0x427] = 0x4fd15283834f1dd1ULL;\
    ((u64*)T)[0x428] = 0x5ce48c68685cd0e4ULL;\
    ((u64*)T)[0x429] = 0xf407565151f4a207ULL;\
    ((u64*)T)[0x42A] = 0x345c8dd1d134b95cULL;\
    ((u64*)T)[0x42B] = 0x0818e1f9f908e918ULL;\
    ((u64*)T)[0x42C] = 0x93ae4ce2e293dfaeULL;\
    ((u64*)T)[0x42D] = 0x73953eabab734d95ULL;\
    ((u64*)T)[0x42E] = 0x53f597626253c4f5ULL;\
    ((u64*)T)[0x42F] = 0x3f416b2a2a3f5441ULL;\
    ((u64*)T)[0x430] = 0x0c141c08080c1014ULL;\
    ((u64*)T)[0x431] = 0x52f66395955231f6ULL;\
    ((u64*)T)[0x432] = 0x65afe94646658cafULL;\
    ((u64*)T)[0x433] = 0x5ee27f9d9d5e21e2ULL;\
    ((u64*)T)[0x434] = 0x2878483030286078ULL;\
    ((u64*)T)[0x435] = 0xa1f8cf3737a16ef8ULL;\
    ((u64*)T)[0x436] = 0x0f111b0a0a0f1411ULL;\
    ((u64*)T)[0x437] = 0xb5c4eb2f2fb55ec4ULL;\
    ((u64*)T)[0x438] = 0x091b150e0e091c1bULL;\
    ((u64*)T)[0x439] = 0x365a7e242436485aULL;\
    ((u64*)T)[0x43A] = 0x9bb6ad1b1b9b36b6ULL;\
    ((u64*)T)[0x43B] = 0x3d4798dfdf3da547ULL;\
    ((u64*)T)[0x43C] = 0x266aa7cdcd26816aULL;\
    ((u64*)T)[0x43D] = 0x69bbf54e4e699cbbULL;\
    ((u64*)T)[0x43E] = 0xcd4c337f7fcdfe4cULL;\
    ((u64*)T)[0x43F] = 0x9fba50eaea9fcfbaULL;\
    ((u64*)T)[0x440] = 0x1b2d3f12121b242dULL;\
    ((u64*)T)[0x441] = 0x9eb9a41d1d9e3ab9ULL;\
    ((u64*)T)[0x442] = 0x749cc4585874b09cULL;\
    ((u64*)T)[0x443] = 0x2e724634342e6872ULL;\
    ((u64*)T)[0x444] = 0x2d774136362d6c77ULL;\
    ((u64*)T)[0x445] = 0xb2cd11dcdcb2a3cdULL;\
    ((u64*)T)[0x446] = 0xee299db4b4ee7329ULL;\
    ((u64*)T)[0x447] = 0xfb164d5b5bfbb616ULL;\
    ((u64*)T)[0x448] = 0xf601a5a4a4f65301ULL;\
    ((u64*)T)[0x449] = 0x4dd7a176764decd7ULL;\
    ((u64*)T)[0x44A] = 0x61a314b7b76175a3ULL;\
    ((u64*)T)[0x44B] = 0xce49347d7dcefa49ULL;\
    ((u64*)T)[0x44C] = 0x7b8ddf52527ba48dULL;\
    ((u64*)T)[0x44D] = 0x3e429fdddd3ea142ULL;\
    ((u64*)T)[0x44E] = 0x7193cd5e5e71bc93ULL;\
    ((u64*)T)[0x44F] = 0x97a2b113139726a2ULL;\
    ((u64*)T)[0x450] = 0xf504a2a6a6f55704ULL;\
    ((u64*)T)[0x451] = 0x68b801b9b96869b8ULL;\
    ((u64*)T)[0x452] = 0x0000000000000000ULL;\
    ((u64*)T)[0x453] = 0x2c74b5c1c12c9974ULL;\
    ((u64*)T)[0x454] = 0x60a0e040406080a0ULL;\
    ((u64*)T)[0x455] = 0x1f21c2e3e31fdd21ULL;\
    ((u64*)T)[0x456] = 0xc8433a7979c8f243ULL;\
    ((u64*)T)[0x457] = 0xed2c9ab6b6ed772cULL;\
    ((u64*)T)[0x458] = 0xbed90dd4d4beb3d9ULL;\
    ((u64*)T)[0x459] = 0x46ca478d8d4601caULL;\
    ((u64*)T)[0x45A] = 0xd970176767d9ce70ULL;\
    ((u64*)T)[0x45B] = 0x4bddaf72724be4ddULL;\
    ((u64*)T)[0x45C] = 0xde79ed9494de3379ULL;\
    ((u64*)T)[0x45D] = 0xd467ff9898d42b67ULL;\
    ((u64*)T)[0x45E] = 0xe82393b0b0e87b23ULL;\
    ((u64*)T)[0x45F] = 0x4ade5b85854a11deULL;\
    ((u64*)T)[0x460] = 0x6bbd06bbbb6b6dbdULL;\
    ((u64*)T)[0x461] = 0x2a7ebbc5c52a917eULL;\
    ((u64*)T)[0x462] = 0xe5347b4f4fe59e34ULL;\
    ((u64*)T)[0x463] = 0x163ad7eded16c13aULL;\
    ((u64*)T)[0x464] = 0xc554d28686c51754ULL;\
    ((u64*)T)[0x465] = 0xd762f89a9ad72f62ULL;\
    ((u64*)T)[0x466] = 0x55ff99666655ccffULL;\
    ((u64*)T)[0x467] = 0x94a7b611119422a7ULL;\
    ((u64*)T)[0x468] = 0xcf4ac08a8acf0f4aULL;\
    ((u64*)T)[0x469] = 0x1030d9e9e910c930ULL;\
    ((u64*)T)[0x46A] = 0x060a0e040406080aULL;\
    ((u64*)T)[0x46B] = 0x819866fefe81e798ULL;\
    ((u64*)T)[0x46C] = 0xf00baba0a0f05b0bULL;\
    ((u64*)T)[0x46D] = 0x44ccb4787844f0ccULL;\
    ((u64*)T)[0x46E] = 0xbad5f02525ba4ad5ULL;\
    ((u64*)T)[0x46F] = 0xe33e754b4be3963eULL;\
    ((u64*)T)[0x470] = 0xf30eaca2a2f35f0eULL;\
    ((u64*)T)[0x471] = 0xfe19445d5dfeba19ULL;\
    ((u64*)T)[0x472] = 0xc05bdb8080c01b5bULL;\
    ((u64*)T)[0x473] = 0x8a858005058a0a85ULL;\
    ((u64*)T)[0x474] = 0xadecd33f3fad7eecULL;\
    ((u64*)T)[0x475] = 0xbcdffe2121bc42dfULL;\
    ((u64*)T)[0x476] = 0x48d8a8707048e0d8ULL;\
    ((u64*)T)[0x477] = 0x040cfdf1f104f90cULL;\
    ((u64*)T)[0x478] = 0xdf7a196363dfc67aULL;\
    ((u64*)T)[0x479] = 0xc1582f7777c1ee58ULL;\
    ((u64*)T)[0x47A] = 0x759f30afaf75459fULL;\
    ((u64*)T)[0x47B] = 0x63a5e742426384a5ULL;\
    ((u64*)T)[0x47C] = 0x3050702020304050ULL;\
    ((u64*)T)[0x47D] = 0x1a2ecbe5e51ad12eULL;\
    ((u64*)T)[0x47E] = 0x0e12effdfd0ee112ULL;\
    ((u64*)T)[0x47F] = 0x6db708bfbf6d65b7ULL;\
    ((u64*)T)[0x480] = 0x4cd45581814c19d4ULL;\
    ((u64*)T)[0x481] = 0x143c24181814303cULL;\
    ((u64*)T)[0x482] = 0x355f792626354c5fULL;\
    ((u64*)T)[0x483] = 0x2f71b2c3c32f9d71ULL;\
    ((u64*)T)[0x484] = 0xe13886bebee16738ULL;\
    ((u64*)T)[0x485] = 0xa2fdc83535a26afdULL;\
    ((u64*)T)[0x486] = 0xcc4fc78888cc0b4fULL;\
    ((u64*)T)[0x487] = 0x394b652e2e395c4bULL;\
    ((u64*)T)[0x488] = 0x57f96a9393573df9ULL;\
    ((u64*)T)[0x489] = 0xf20d585555f2aa0dULL;\
    ((u64*)T)[0x48A] = 0x829d61fcfc82e39dULL;\
    ((u64*)T)[0x48B] = 0x47c9b37a7a47f4c9ULL;\
    ((u64*)T)[0x48C] = 0xacef27c8c8ac8befULL;\
    ((u64*)T)[0x48D] = 0xe73288babae76f32ULL;\
    ((u64*)T)[0x48E] = 0x2b7d4f32322b647dULL;\
    ((u64*)T)[0x48F] = 0x95a442e6e695d7a4ULL;\
    ((u64*)T)[0x490] = 0xa0fb3bc0c0a09bfbULL;\
    ((u64*)T)[0x491] = 0x98b3aa19199832b3ULL;\
    ((u64*)T)[0x492] = 0xd168f69e9ed12768ULL;\
    ((u64*)T)[0x493] = 0x7f8122a3a37f5d81ULL;\
    ((u64*)T)[0x494] = 0x66aaee44446688aaULL;\
    ((u64*)T)[0x495] = 0x7e82d654547ea882ULL;\
    ((u64*)T)[0x496] = 0xabe6dd3b3bab76e6ULL;\
    ((u64*)T)[0x497] = 0x839e950b0b83169eULL;\
    ((u64*)T)[0x498] = 0xca45c98c8cca0345ULL;\
    ((u64*)T)[0x499] = 0x297bbcc7c729957bULL;\
    ((u64*)T)[0x49A] = 0xd36e056b6bd3d66eULL;\
    ((u64*)T)[0x49B] = 0x3c446c28283c5044ULL;\
    ((u64*)T)[0x49C] = 0x798b2ca7a779558bULL;\
    ((u64*)T)[0x49D] = 0xe23d81bcbce2633dULL;\
    ((u64*)T)[0x49E] = 0x1d273116161d2c27ULL;\
    ((u64*)T)[0x49F] = 0x769a37adad76419aULL;\
    ((u64*)T)[0x4A0] = 0x3b4d96dbdb3bad4dULL;\
    ((u64*)T)[0x4A1] = 0x56fa9e646456c8faULL;\
    ((u64*)T)[0x4A2] = 0x4ed2a674744ee8d2ULL;\
    ((u64*)T)[0x4A3] = 0x1e223614141e2822ULL;\
    ((u64*)T)[0x4A4] = 0xdb76e49292db3f76ULL;\
    ((u64*)T)[0x4A5] = 0x0a1e120c0c0a181eULL;\
    ((u64*)T)[0x4A6] = 0x6cb4fc48486c90b4ULL;\
    ((u64*)T)[0x4A7] = 0xe4378fb8b8e46b37ULL;\
    ((u64*)T)[0x4A8] = 0x5de7789f9f5d25e7ULL;\
    ((u64*)T)[0x4A9] = 0x6eb20fbdbd6e61b2ULL;\
    ((u64*)T)[0x4AA] = 0xef2a694343ef862aULL;\
    ((u64*)T)[0x4AB] = 0xa6f135c4c4a693f1ULL;\
    ((u64*)T)[0x4AC] = 0xa8e3da3939a872e3ULL;\
    ((u64*)T)[0x4AD] = 0xa4f7c63131a462f7ULL;\
    ((u64*)T)[0x4AE] = 0x37598ad3d337bd59ULL;\
    ((u64*)T)[0x4AF] = 0x8b8674f2f28bff86ULL;\
    ((u64*)T)[0x4B0] = 0x325683d5d532b156ULL;\
    ((u64*)T)[0x4B1] = 0x43c54e8b8b430dc5ULL;\
    ((u64*)T)[0x4B2] = 0x59eb856e6e59dcebULL;\
    ((u64*)T)[0x4B3] = 0xb7c218dadab7afc2ULL;\
    ((u64*)T)[0x4B4] = 0x8c8f8e01018c028fULL;\
    ((u64*)T)[0x4B5] = 0x64ac1db1b16479acULL;\
    ((u64*)T)[0x4B6] = 0xd26df19c9cd2236dULL;\
    ((u64*)T)[0x4B7] = 0xe03b724949e0923bULL;\
    ((u64*)T)[0x4B8] = 0xb4c71fd8d8b4abc7ULL;\
    ((u64*)T)[0x4B9] = 0xfa15b9acacfa4315ULL;\
    ((u64*)T)[0x4BA] = 0x0709faf3f307fd09ULL;\
    ((u64*)T)[0x4BB] = 0x256fa0cfcf25856fULL;\
    ((u64*)T)[0x4BC] = 0xafea20cacaaf8feaULL;\
    ((u64*)T)[0x4BD] = 0x8e897df4f48ef389ULL;\
    ((u64*)T)[0x4BE] = 0xe920674747e98e20ULL;\
    ((u64*)T)[0x4BF] = 0x1828381010182028ULL;\
    ((u64*)T)[0x4C0] = 0xd5640b6f6fd5de64ULL;\
    ((u64*)T)[0x4C1] = 0x888373f0f088fb83ULL;\
    ((u64*)T)[0x4C2] = 0x6fb1fb4a4a6f94b1ULL;\
    ((u64*)T)[0x4C3] = 0x7296ca5c5c72b896ULL;\
    ((u64*)T)[0x4C4] = 0x246c54383824706cULL;\
    ((u64*)T)[0x4C5] = 0xf1085f5757f1ae08ULL;\
    ((u64*)T)[0x4C6] = 0xc752217373c7e652ULL;\
    ((u64*)T)[0x4C7] = 0x51f36497975135f3ULL;\
    ((u64*)T)[0x4C8] = 0x2365aecbcb238d65ULL;\
    ((u64*)T)[0x4C9] = 0x7c8425a1a17c5984ULL;\
    ((u64*)T)[0x4CA] = 0x9cbf57e8e89ccbbfULL;\
    ((u64*)T)[0x4CB] = 0x21635d3e3e217c63ULL;\
    ((u64*)T)[0x4CC] = 0xdd7cea9696dd377cULL;\
    ((u64*)T)[0x4CD] = 0xdc7f1e6161dcc27fULL;\
    ((u64*)T)[0x4CE] = 0x86919c0d0d861a91ULL;\
    ((u64*)T)[0x4CF] = 0x85949b0f0f851e94ULL;\
    ((u64*)T)[0x4D0] = 0x90ab4be0e090dbabULL;\
    ((u64*)T)[0x4D1] = 0x42c6ba7c7c42f8c6ULL;\
    ((u64*)T)[0x4D2] = 0xc457267171c4e257ULL;\
    ((u64*)T)[0x4D3] = 0xaae529ccccaa83e5ULL;\
    ((u64*)T)[0x4D4] = 0xd873e39090d83b73ULL;\
    ((u64*)T)[0x4D5] = 0x050f090606050c0fULL;\
    ((u64*)T)[0x4D6] = 0x0103f4f7f701f503ULL;\
    ((u64*)T)[0x4D7] = 0x12362a1c1c123836ULL;\
    ((u64*)T)[0x4D8] = 0xa3fe3cc2c2a39ffeULL;\
    ((u64*)T)[0x4D9] = 0x5fe18b6a6a5fd4e1ULL;\
    ((u64*)T)[0x4DA] = 0xf910beaeaef94710ULL;\
    ((u64*)T)[0x4DB] = 0xd06b026969d0d26bULL;\
    ((u64*)T)[0x4DC] = 0x91a8bf1717912ea8ULL;\
    ((u64*)T)[0x4DD] = 0x58e87199995829e8ULL;\
    ((u64*)T)[0x4DE] = 0x2769533a3a277469ULL;\
    ((u64*)T)[0x4DF] = 0xb9d0f72727b94ed0ULL;\
    ((u64*)T)[0x4E0] = 0x384891d9d938a948ULL;\
    ((u64*)T)[0x4E1] = 0x1335deebeb13cd35ULL;\
    ((u64*)T)[0x4E2] = 0xb3cee52b2bb356ceULL;\
    ((u64*)T)[0x4E3] = 0x3355772222334455ULL;\
    ((u64*)T)[0x4E4] = 0xbbd604d2d2bbbfd6ULL;\
    ((u64*)T)[0x4E5] = 0x709039a9a9704990ULL;\
    ((u64*)T)[0x4E6] = 0x8980870707890e80ULL;\
    ((u64*)T)[0x4E7] = 0xa7f2c13333a766f2ULL;\
    ((u64*)T)[0x4E8] = 0xb6c1ec2d2db65ac1ULL;\
    ((u64*)T)[0x4E9] = 0x22665a3c3c227866ULL;\
    ((u64*)T)[0x4EA] = 0x92adb81515922aadULL;\
    ((u64*)T)[0x4EB] = 0x2060a9c9c9208960ULL;\
    ((u64*)T)[0x4EC] = 0x49db5c87874915dbULL;\
    ((u64*)T)[0x4ED] = 0xff1ab0aaaaff4f1aULL;\
    ((u64*)T)[0x4EE] = 0x7888d8505078a088ULL;\
    ((u64*)T)[0x4EF] = 0x7a8e2ba5a57a518eULL;\
    ((u64*)T)[0x4F0] = 0x8f8a8903038f068aULL;\
    ((u64*)T)[0x4F1] = 0xf8134a5959f8b213ULL;\
    ((u64*)T)[0x4F2] = 0x809b92090980129bULL;\
    ((u64*)T)[0x4F3] = 0x1739231a1a173439ULL;\
    ((u64*)T)[0x4F4] = 0xda75106565daca75ULL;\
    ((u64*)T)[0x4F5] = 0x315384d7d731b553ULL;\
    ((u64*)T)[0x4F6] = 0xc651d58484c61351ULL;\
    ((u64*)T)[0x4F7] = 0xb8d303d0d0b8bbd3ULL;\
    ((u64*)T)[0x4F8] = 0xc35edc8282c31f5eULL;\
    ((u64*)T)[0x4F9] = 0xb0cbe22929b052cbULL;\
    ((u64*)T)[0x4FA] = 0x7799c35a5a77b499ULL;\
    ((u64*)T)[0x4FB] = 0x11332d1e1e113c33ULL;\
    ((u64*)T)[0x4FC] = 0xcb463d7b7bcbf646ULL;\
    ((u64*)T)[0x4FD] = 0xfc1fb7a8a8fc4b1fULL;\
    ((u64*)T)[0x4FE] = 0xd6610c6d6dd6da61ULL;\
    ((u64*)T)[0x4FF] = 0x3a4e622c2c3a584eULL;\
    ((u64*)T)[0x500] = 0xf432c6c6a597f4a5ULL;\
    ((u64*)T)[0x501] = 0x976ff8f884eb9784ULL;\
    ((u64*)T)[0x502] = 0xb05eeeee99c7b099ULL;\
    ((u64*)T)[0x503] = 0x8c7af6f68df78c8dULL;\
    ((u64*)T)[0x504] = 0x17e8ffff0de5170dULL;\
    ((u64*)T)[0x505] = 0xdc0ad6d6bdb7dcbdULL;\
    ((u64*)T)[0x506] = 0xc816dedeb1a7c8b1ULL;\
    ((u64*)T)[0x507] = 0xfc6d91915439fc54ULL;\
    ((u64*)T)[0x508] = 0xf090606050c0f050ULL;\
    ((u64*)T)[0x509] = 0x0507020203040503ULL;\
    ((u64*)T)[0x50A] = 0xe02ececea987e0a9ULL;\
    ((u64*)T)[0x50B] = 0x87d156567dac877dULL;\
    ((u64*)T)[0x50C] = 0x2bcce7e719d52b19ULL;\
    ((u64*)T)[0x50D] = 0xa613b5b56271a662ULL;\
    ((u64*)T)[0x50E] = 0x317c4d4de69a31e6ULL;\
    ((u64*)T)[0x50F] = 0xb559ecec9ac3b59aULL;\
    ((u64*)T)[0x510] = 0xcf408f8f4505cf45ULL;\
    ((u64*)T)[0x511] = 0xbca31f1f9d3ebc9dULL;\
    ((u64*)T)[0x512] = 0xc04989894009c040ULL;\
    ((u64*)T)[0x513] = 0x9268fafa87ef9287ULL;\
    ((u64*)T)[0x514] = 0x3fd0efef15c53f15ULL;\
    ((u64*)T)[0x515] = 0x2694b2b2eb7f26ebULL;\
    ((u64*)T)[0x516] = 0x40ce8e8ec90740c9ULL;\
    ((u64*)T)[0x517] = 0x1de6fbfb0bed1d0bULL;\
    ((u64*)T)[0x518] = 0x2f6e4141ec822fecULL;\
    ((u64*)T)[0x519] = 0xa91ab3b3677da967ULL;\
    ((u64*)T)[0x51A] = 0x1c435f5ffdbe1cfdULL;\
    ((u64*)T)[0x51B] = 0x25604545ea8a25eaULL;\
    ((u64*)T)[0x51C] = 0xdaf92323bf46dabfULL;\
    ((u64*)T)[0x51D] = 0x02515353f7a602f7ULL;\
    ((u64*)T)[0x51E] = 0xa145e4e496d3a196ULL;\
    ((u64*)T)[0x51F] = 0xed769b9b5b2ded5bULL;\
    ((u64*)T)[0x520] = 0x5d287575c2ea5dc2ULL;\
    ((u64*)T)[0x521] = 0x24c5e1e11cd9241cULL;\
    ((u64*)T)[0x522] = 0xe9d43d3dae7ae9aeULL;\
    ((u64*)T)[0x523] = 0xbef24c4c6a98be6aULL;\
    ((u64*)T)[0x524] = 0xee826c6c5ad8ee5aULL;\
    ((u64*)T)[0x525] = 0xc3bd7e7e41fcc341ULL;\
    ((u64*)T)[0x526] = 0x06f3f5f502f10602ULL;\
    ((u64*)T)[0x527] = 0xd15283834f1dd14fULL;\
    ((u64*)T)[0x528] = 0xe48c68685cd0e45cULL;\
    ((u64*)T)[0x529] = 0x07565151f4a207f4ULL;\
    ((u64*)T)[0x52A] = 0x5c8dd1d134b95c34ULL;\
    ((u64*)T)[0x52B] = 0x18e1f9f908e91808ULL;\
    ((u64*)T)[0x52C] = 0xae4ce2e293dfae93ULL;\
    ((u64*)T)[0x52D] = 0x953eabab734d9573ULL;\
    ((u64*)T)[0x52E] = 0xf597626253c4f553ULL;\
    ((u64*)T)[0x52F] = 0x416b2a2a3f54413fULL;\
    ((u64*)T)[0x530] = 0x141c08080c10140cULL;\
    ((u64*)T)[0x531] = 0xf66395955231f652ULL;\
    ((u64*)T)[0x532] = 0xafe94646658caf65ULL;\
    ((u64*)T)[0x533] = 0xe27f9d9d5e21e25eULL;\
    ((u64*)T)[0x534] = 0x7848303028607828ULL;\
    ((u64*)T)[0x535] = 0xf8cf3737a16ef8a1ULL;\
    ((u64*)T)[0x536] = 0x111b0a0a0f14110fULL;\
    ((u64*)T)[0x537] = 0xc4eb2f2fb55ec4b5ULL;\
    ((u64*)T)[0x538] = 0x1b150e0e091c1b09ULL;\
    ((u64*)T)[0x539] = 0x5a7e242436485a36ULL;\
    ((u64*)T)[0x53A] = 0xb6ad1b1b9b36b69bULL;\
    ((u64*)T)[0x53B] = 0x4798dfdf3da5473dULL;\
    ((u64*)T)[0x53C] = 0x6aa7cdcd26816a26ULL;\
    ((u64*)T)[0x53D] = 0xbbf54e4e699cbb69ULL;\
    ((u64*)T)[0x53E] = 0x4c337f7fcdfe4ccdULL;\
    ((u64*)T)[0x53F] = 0xba50eaea9fcfba9fULL;\
    ((u64*)T)[0x540] = 0x2d3f12121b242d1bULL;\
    ((u64*)T)[0x541] = 0xb9a41d1d9e3ab99eULL;\
    ((u64*)T)[0x542] = 0x9cc4585874b09c74ULL;\
    ((u64*)T)[0x543] = 0x724634342e68722eULL;\
    ((u64*)T)[0x544] = 0x774136362d6c772dULL;\
    ((u64*)T)[0x545] = 0xcd11dcdcb2a3cdb2ULL;\
    ((u64*)T)[0x546] = 0x299db4b4ee7329eeULL;\
    ((u64*)T)[0x547] = 0x164d5b5bfbb616fbULL;\
    ((u64*)T)[0x548] = 0x01a5a4a4f65301f6ULL;\
    ((u64*)T)[0x549] = 0xd7a176764decd74dULL;\
    ((u64*)T)[0x54A] = 0xa314b7b76175a361ULL;\
    ((u64*)T)[0x54B] = 0x49347d7dcefa49ceULL;\
    ((u64*)T)[0x54C] = 0x8ddf52527ba48d7bULL;\
    ((u64*)T)[0x54D] = 0x429fdddd3ea1423eULL;\
    ((u64*)T)[0x54E] = 0x93cd5e5e71bc9371ULL;\
    ((u64*)T)[0x54F] = 0xa2b113139726a297ULL;\
    ((u64*)T)[0x550] = 0x04a2a6a6f55704f5ULL;\
    ((u64*)T)[0x551] = 0xb801b9b96869b868ULL;\
    ((u64*)T)[0x552] = 0x0000000000000000ULL;\
    ((u64*)T)[0x553] = 0x74b5c1c12c99742cULL;\
    ((u64*)T)[0x554] = 0xa0e040406080a060ULL;\
    ((u64*)T)[0x555] = 0x21c2e3e31fdd211fULL;\
    ((u64*)T)[0x556] = 0x433a7979c8f243c8ULL;\
    ((u64*)T)[0x557] = 0x2c9ab6b6ed772cedULL;\
    ((u64*)T)[0x558] = 0xd90dd4d4beb3d9beULL;\
    ((u64*)T)[0x559] = 0xca478d8d4601ca46ULL;\
    ((u64*)T)[0x55A] = 0x70176767d9ce70d9ULL;\
    ((u64*)T)[0x55B] = 0xddaf72724be4dd4bULL;\
    ((u64*)T)[0x55C] = 0x79ed9494de3379deULL;\
    ((u64*)T)[0x55D] = 0x67ff9898d42b67d4ULL;\
    ((u64*)T)[0x55E] = 0x2393b0b0e87b23e8ULL;\
    ((u64*)T)[0x55F] = 0xde5b85854a11de4aULL;\
    ((u64*)T)[0x560] = 0xbd06bbbb6b6dbd6bULL;\
    ((u64*)T)[0x561] = 0x7ebbc5c52a917e2aULL;\
    ((u64*)T)[0x562] = 0x347b4f4fe59e34e5ULL;\
    ((u64*)T)[0x563] = 0x3ad7eded16c13a16ULL;\
    ((u64*)T)[0x564] = 0x54d28686c51754c5ULL;\
    ((u64*)T)[0x565] = 0x62f89a9ad72f62d7ULL;\
    ((u64*)T)[0x566] = 0xff99666655ccff55ULL;\
    ((u64*)T)[0x567] = 0xa7b611119422a794ULL;\
    ((u64*)T)[0x568] = 0x4ac08a8acf0f4acfULL;\
    ((u64*)T)[0x569] = 0x30d9e9e910c93010ULL;\
    ((u64*)T)[0x56A] = 0x0a0e040406080a06ULL;\
    ((u64*)T)[0x56B] = 0x9866fefe81e79881ULL;\
    ((u64*)T)[0x56C] = 0x0baba0a0f05b0bf0ULL;\
    ((u64*)T)[0x56D] = 0xccb4787844f0cc44ULL;\
    ((u64*)T)[0x56E] = 0xd5f02525ba4ad5baULL;\
    ((u64*)T)[0x56F] = 0x3e754b4be3963ee3ULL;\
    ((u64*)T)[0x570] = 0x0eaca2a2f35f0ef3ULL;\
    ((u64*)T)[0x571] = 0x19445d5dfeba19feULL;\
    ((u64*)T)[0x572] = 0x5bdb8080c01b5bc0ULL;\
    ((u64*)T)[0x573] = 0x858005058a0a858aULL;\
    ((u64*)T)[0x574] = 0xecd33f3fad7eecadULL;\
    ((u64*)T)[0x575] = 0xdffe2121bc42dfbcULL;\
    ((u64*)T)[0x576] = 0xd8a8707048e0d848ULL;\
    ((u64*)T)[0x577] = 0x0cfdf1f104f90c04ULL;\
    ((u64*)T)[0x578] = 0x7a196363dfc67adfULL;\
    ((u64*)T)[0x579] = 0x582f7777c1ee58c1ULL;\
    ((u64*)T)[0x57A] = 0x9f30afaf75459f75ULL;\
    ((u64*)T)[0x57B] = 0xa5e742426384a563ULL;\
    ((u64*)T)[0x57C] = 0x5070202030405030ULL;\
    ((u64*)T)[0x57D] = 0x2ecbe5e51ad12e1aULL;\
    ((u64*)T)[0x57E] = 0x12effdfd0ee1120eULL;\
    ((u64*)T)[0x57F] = 0xb708bfbf6d65b76dULL;\
    ((u64*)T)[0x580] = 0xd45581814c19d44cULL;\
    ((u64*)T)[0x581] = 0x3c24181814303c14ULL;\
    ((u64*)T)[0x582] = 0x5f792626354c5f35ULL;\
    ((u64*)T)[0x583] = 0x71b2c3c32f9d712fULL;\
    ((u64*)T)[0x584] = 0x3886bebee16738e1ULL;\
    ((u64*)T)[0x585] = 0xfdc83535a26afda2ULL;\
    ((u64*)T)[0x586] = 0x4fc78888cc0b4fccULL;\
    ((u64*)T)[0x587] = 0x4b652e2e395c4b39ULL;\
    ((u64*)T)[0x588] = 0xf96a9393573df957ULL;\
    ((u64*)T)[0x589] = 0x0d585555f2aa0df2ULL;\
    ((u64*)T)[0x58A] = 0x9d61fcfc82e39d82ULL;\
    ((u64*)T)[0x58B] = 0xc9b37a7a47f4c947ULL;\
    ((u64*)T)[0x58C] = 0xef27c8c8ac8befacULL;\
    ((u64*)T)[0x58D] = 0x3288babae76f32e7ULL;\
    ((u64*)T)[0x58E] = 0x7d4f32322b647d2bULL;\
    ((u64*)T)[0x58F] = 0xa442e6e695d7a495ULL;\
    ((u64*)T)[0x590] = 0xfb3bc0c0a09bfba0ULL;\
    ((u64*)T)[0x591] = 0xb3aa19199832b398ULL;\
    ((u64*)T)[0x592] = 0x68f69e9ed12768d1ULL;\
    ((u64*)T)[0x593] = 0x8122a3a37f5d817fULL;\
    ((u64*)T)[0x594] = 0xaaee44446688aa66ULL;\
    ((u64*)T)[0x595] = 0x82d654547ea8827eULL;\
    ((u64*)T)[0x596] = 0xe6dd3b3bab76e6abULL;\
    ((u64*)T)[0x597] = 0x9e950b0b83169e83ULL;\
    ((u64*)T)[0x598] = 0x45c98c8cca0345caULL;\
    ((u64*)T)[0x599] = 0x7bbcc7c729957b29ULL;\
    ((u64*)T)[0x59A] = 0x6e056b6bd3d66ed3ULL;\
    ((u64*)T)[0x59B] = 0x446c28283c50443cULL;\
    ((u64*)T)[0x59C] = 0x8b2ca7a779558b79ULL;\
    ((u64*)T)[0x59D] = 0x3d81bcbce2633de2ULL;\
    ((u64*)T)[0x59E] = 0x273116161d2c271dULL;\
    ((u64*)T)[0x59F] = 0x9a37adad76419a76ULL;\
    ((u64*)T)[0x5A0] = 0x4d96dbdb3bad4d3bULL;\
    ((u64*)T)[0x5A1] = 0xfa9e646456c8fa56ULL;\
    ((u64*)T)[0x5A2] = 0xd2a674744ee8d24eULL;\
    ((u64*)T)[0x5A3] = 0x223614141e28221eULL;\
    ((u64*)T)[0x5A4] = 0x76e49292db3f76dbULL;\
    ((u64*)T)[0x5A5] = 0x1e120c0c0a181e0aULL;\
    ((u64*)T)[0x5A6] = 0xb4fc48486c90b46cULL;\
    ((u64*)T)[0x5A7] = 0x378fb8b8e46b37e4ULL;\
    ((u64*)T)[0x5A8] = 0xe7789f9f5d25e75dULL;\
    ((u64*)T)[0x5A9] = 0xb20fbdbd6e61b26eULL;\
    ((u64*)T)[0x5AA] = 0x2a694343ef862aefULL;\
    ((u64*)T)[0x5AB] = 0xf135c4c4a693f1a6ULL;\
    ((u64*)T)[0x5AC] = 0xe3da3939a872e3a8ULL;\
    ((u64*)T)[0x5AD] = 0xf7c63131a462f7a4ULL;\
    ((u64*)T)[0x5AE] = 0x598ad3d337bd5937ULL;\
    ((u64*)T)[0x5AF] = 0x8674f2f28bff868bULL;\
    ((u64*)T)[0x5B0] = 0x5683d5d532b15632ULL;\
    ((u64*)T)[0x5B1] = 0xc54e8b8b430dc543ULL;\
    ((u64*)T)[0x5B2] = 0xeb856e6e59dceb59ULL;\
    ((u64*)T)[0x5B3] = 0xc218dadab7afc2b7ULL;\
    ((u64*)T)[0x5B4] = 0x8f8e01018c028f8cULL;\
    ((u64*)T)[0x5B5] = 0xac1db1b16479ac64ULL;\
    ((u64*)T)[0x5B6] = 0x6df19c9cd2236dd2ULL;\
    ((u64*)T)[0x5B7] = 0x3b724949e0923be0ULL;\
    ((u64*)T)[0x5B8] = 0xc71fd8d8b4abc7b4ULL;\
    ((u64*)T)[0x5B9] = 0x15b9acacfa4315faULL;\
    ((u64*)T)[0x5BA] = 0x09faf3f307fd0907ULL;\
    ((u64*)T)[0x5BB] = 0x6fa0cfcf25856f25ULL;\
    ((u64*)T)[0x5BC] = 0xea20cacaaf8feaafULL;\
    ((u64*)T)[0x5BD] = 0x897df4f48ef3898eULL;\
    ((u64*)T)[0x5BE] = 0x20674747e98e20e9ULL;\
    ((u64*)T)[0x5BF] = 0x2838101018202818ULL;\
    ((u64*)T)[0x5C0] = 0x640b6f6fd5de64d5ULL;\
    ((u64*)T)[0x5C1] = 0x8373f0f088fb8388ULL;\
    ((u64*)T)[0x5C2] = 0xb1fb4a4a6f94b16fULL;\
    ((u64*)T)[0x5C3] = 0x96ca5c5c72b89672ULL;\
    ((u64*)T)[0x5C4] = 0x6c54383824706c24ULL;\
    ((u64*)T)[0x5C5] = 0x085f5757f1ae08f1ULL;\
    ((u64*)T)[0x5C6] = 0x52217373c7e652c7ULL;\
    ((u64*)T)[0x5C7] = 0xf36497975135f351ULL;\
    ((u64*)T)[0x5C8] = 0x65aecbcb238d6523ULL;\
    ((u64*)T)[0x5C9] = 0x8425a1a17c59847cULL;\
    ((u64*)T)[0x5CA] = 0xbf57e8e89ccbbf9cULL;\
    ((u64*)T)[0x5CB] = 0x635d3e3e217c6321ULL;\
    ((u64*)T)[0x5CC] = 0x7cea9696dd377cddULL;\
    ((u64*)T)[0x5CD] = 0x7f1e6161dcc27fdcULL;\
    ((u64*)T)[0x5CE] = 0x919c0d0d861a9186ULL;\
    ((u64*)T)[0x5CF] = 0x949b0f0f851e9485ULL;\
    ((u64*)T)[0x5D0] = 0xab4be0e090dbab90ULL;\
    ((u64*)T)[0x5D1] = 0xc6ba7c7c42f8c642ULL;\
    ((u64*)T)[0x5D2] = 0x57267171c4e257c4ULL;\
    ((u64*)T)[0x5D3] = 0xe529ccccaa83e5aaULL;\
    ((u64*)T)[0x5D4] = 0x73e39090d83b73d8ULL;\
    ((u64*)T)[0x5D5] = 0x0f090606050c0f05ULL;\
    ((u64*)T)[0x5D6] = 0x03f4f7f701f50301ULL;\
    ((u64*)T)[0x5D7] = 0x362a1c1c12383612ULL;\
    ((u64*)T)[0x5D8] = 0xfe3cc2c2a39ffea3ULL;\
    ((u64*)T)[0x5D9] = 0xe18b6a6a5fd4e15fULL;\
    ((u64*)T)[0x5DA] = 0x10beaeaef94710f9ULL;\
    ((u64*)T)[0x5DB] = 0x6b026969d0d26bd0ULL;\
    ((u64*)T)[0x5DC] = 0xa8bf1717912ea891ULL;\
    ((u64*)T)[0x5DD] = 0xe87199995829e858ULL;\
    ((u64*)T)[0x5DE] = 0x69533a3a27746927ULL;\
    ((u64*)T)[0x5DF] = 0xd0f72727b94ed0b9ULL;\
    ((u64*)T)[0x5E0] = 0x4891d9d938a94838ULL;\
    ((u64*)T)[0x5E1] = 0x35deebeb13cd3513ULL;\
    ((u64*)T)[0x5E2] = 0xcee52b2bb356ceb3ULL;\
    ((u64*)T)[0x5E3] = 0x5577222233445533ULL;\
    ((u64*)T)[0x5E4] = 0xd604d2d2bbbfd6bbULL;\
    ((u64*)T)[0x5E5] = 0x9039a9a970499070ULL;\
    ((u64*)T)[0x5E6] = 0x80870707890e8089ULL;\
    ((u64*)T)[0x5E7] = 0xf2c13333a766f2a7ULL;\
    ((u64*)T)[0x5E8] = 0xc1ec2d2db65ac1b6ULL;\
    ((u64*)T)[0x5E9] = 0x665a3c3c22786622ULL;\
    ((u64*)T)[0x5EA] = 0xadb81515922aad92ULL;\
    ((u64*)T)[0x5EB] = 0x60a9c9c920896020ULL;\
    ((u64*)T)[0x5EC] = 0xdb5c87874915db49ULL;\
    ((u64*)T)[0x5ED] = 0x1ab0aaaaff4f1affULL;\
    ((u64*)T)[0x5EE] = 0x88d8505078a08878ULL;\
    ((u64*)T)[0x5EF] = 0x8e2ba5a57a518e7aULL;\
    ((u64*)T)[0x5F0] = 0x8a8903038f068a8fULL;\
    ((u64*)T)[0x5F1] = 0x134a5959f8b213f8ULL;\
    ((u64*)T)[0x5F2] = 0x9b92090980129b80ULL;\
    ((u64*)T)[0x5F3] = 0x39231a1a17343917ULL;\
    ((u64*)T)[0x5F4] = 0x75106565daca75daULL;\
    ((u64*)T)[0x5F5] = 0x5384d7d731b55331ULL;\
    ((u64*)T)[0x5F6] = 0x51d58484c61351c6ULL;\
    ((u64*)T)[0x5F7] = 0xd303d0d0b8bbd3b8ULL;\
    ((u64*)T)[0x5F8] = 0x5edc8282c31f5ec3ULL;\
    ((u64*)T)[0x5F9] = 0xcbe22929b052cbb0ULL;\
    ((u64*)T)[0x5FA] = 0x99c35a5a77b49977ULL;\
    ((u64*)T)[0x5FB] = 0x332d1e1e113c3311ULL;\
    ((u64*)T)[0x5FC] = 0x463d7b7bcbf646cbULL;\
    ((u64*)T)[0x5FD] = 0x1fb7a8a8fc4b1ffcULL;\
    ((u64*)T)[0x5FE] = 0x610c6d6dd6da61d6ULL;\
    ((u64*)T)[0x5FF] = 0x4e622c2c3a584e3aULL;\
    ((u64*)T)[0x600] = 0x32c6c6a597f4a5f4ULL;\
    ((u64*)T)[0x601] = 0x6ff8f884eb978497ULL;\
    ((u64*)T)[0x602] = 0x5eeeee99c7b099b0ULL;\
    ((u64*)T)[0x603] = 0x7af6f68df78c8d8cULL;\
    ((u64*)T)[0x604] = 0xe8ffff0de5170d17ULL;\
    ((u64*)T)[0x605] = 0x0ad6d6bdb7dcbddcULL;\
    ((u64*)T)[0x606] = 0x16dedeb1a7c8b1c8ULL;\
    ((u64*)T)[0x607] = 0x6d91915439fc54fcULL;\
    ((u64*)T)[0x608] = 0x90606050c0f050f0ULL;\
    ((u64*)T)[0x609] = 0x0702020304050305ULL;\
    ((u64*)T)[0x60A] = 0x2ececea987e0a9e0ULL;\
    ((u64*)T)[0x60B] = 0xd156567dac877d87ULL;\
    ((u64*)T)[0x60C] = 0xcce7e719d52b192bULL;\
    ((u64*)T)[0x60D] = 0x13b5b56271a662a6ULL;\
    ((u64*)T)[0x60E] = 0x7c4d4de69a31e631ULL;\
    ((u64*)T)[0x60F] = 0x59ecec9ac3b59ab5ULL;\
    ((u64*)T)[0x610] = 0x408f8f4505cf45cfULL;\
    ((u64*)T)[0x611] = 0xa31f1f9d3ebc9dbcULL;\
    ((u64*)T)[0x612] = 0x4989894009c040c0ULL;\
    ((u64*)T)[0x613] = 0x68fafa87ef928792ULL;\
    ((u64*)T)[0x614] = 0xd0efef15c53f153fULL;\
    ((u64*)T)[0x615] = 0x94b2b2eb7f26eb26ULL;\
    ((u64*)T)[0x616] = 0xce8e8ec90740c940ULL;\
    ((u64*)T)[0x617] = 0xe6fbfb0bed1d0b1dULL;\
    ((u64*)T)[0x618] = 0x6e4141ec822fec2fULL;\
    ((u64*)T)[0x619] = 0x1ab3b3677da967a9ULL;\
    ((u64*)T)[0x61A] = 0x435f5ffdbe1cfd1cULL;\
    ((u64*)T)[0x61B] = 0x604545ea8a25ea25ULL;\
    ((u64*)T)[0x61C] = 0xf92323bf46dabfdaULL;\
    ((u64*)T)[0x61D] = 0x515353f7a602f702ULL;\
    ((u64*)T)[0x61E] = 0x45e4e496d3a196a1ULL;\
    ((u64*)T)[0x61F] = 0x769b9b5b2ded5bedULL;\
    ((u64*)T)[0x620] = 0x287575c2ea5dc25dULL;\
    ((u64*)T)[0x621] = 0xc5e1e11cd9241c24ULL;\
    ((u64*)T)[0x622] = 0xd43d3dae7ae9aee9ULL;\
    ((u64*)T)[0x623] = 0xf24c4c6a98be6abeULL;\
    ((u64*)T)[0x624] = 0x826c6c5ad8ee5aeeULL;\
    ((u64*)T)[0x625] = 0xbd7e7e41fcc341c3ULL;\
    ((u64*)T)[0x626] = 0xf3f5f502f1060206ULL;\
    ((u64*)T)[0x627] = 0x5283834f1dd14fd1ULL;\
    ((u64*)T)[0x628] = 0x8c68685cd0e45ce4ULL;\
    ((u64*)T)[0x629] = 0x565151f4a207f407ULL;\
    ((u64*)T)[0x62A] = 0x8dd1d134b95c345cULL;\
    ((u64*)T)[0x62B] = 0xe1f9f908e9180818ULL;\
    ((u64*)T)[0x62C] = 0x4ce2e293dfae93aeULL;\
    ((u64*)T)[0x62D] = 0x3eabab734d957395ULL;\
    ((u64*)T)[0x62E] = 0x97626253c4f553f5ULL;\
    ((u64*)T)[0x62F] = 0x6b2a2a3f54413f41ULL;\
    ((u64*)T)[0x630] = 0x1c08080c10140c14ULL;\
    ((u64*)T)[0x631] = 0x6395955231f652f6ULL;\
    ((u64*)T)[0x632] = 0xe94646658caf65afULL;\
    ((u64*)T)[0x633] = 0x7f9d9d5e21e25ee2ULL;\
    ((u64*)T)[0x634] = 0x4830302860782878ULL;\
    ((u64*)T)[0x635] = 0xcf3737a16ef8a1f8ULL;\
    ((u64*)T)[0x636] = 0x1b0a0a0f14110f11ULL;\
    ((u64*)T)[0x637] = 0xeb2f2fb55ec4b5c4ULL;\
    ((u64*)T)[0x638] = 0x150e0e091c1b091bULL;\
    ((u64*)T)[0x639] = 0x7e242436485a365aULL;\
    ((u64*)T)[0x63A] = 0xad1b1b9b36b69bb6ULL;\
    ((u64*)T)[0x63B] = 0x98dfdf3da5473d47ULL;\
    ((u64*)T)[0x63C] = 0xa7cdcd26816a266aULL;\
    ((u64*)T)[0x63D] = 0xf54e4e699cbb69bbULL;\
    ((u64*)T)[0x63E] = 0x337f7fcdfe4ccd4cULL;\
    ((u64*)T)[0x63F] = 0x50eaea9fcfba9fbaULL;\
    ((u64*)T)[0x640] = 0x3f12121b242d1b2dULL;\
    ((u64*)T)[0x641] = 0xa41d1d9e3ab99eb9ULL;\
    ((u64*)T)[0x642] = 0xc4585874b09c749cULL;\
    ((u64*)T)[0x643] = 0x4634342e68722e72ULL;\
    ((u64*)T)[0x644] = 0x4136362d6c772d77ULL;\
    ((u64*)T)[0x645] = 0x11dcdcb2a3cdb2cdULL;\
    ((u64*)T)[0x646] = 0x9db4b4ee7329ee29ULL;\
    ((u64*)T)[0x647] = 0x4d5b5bfbb616fb16ULL;\
    ((u64*)T)[0x648] = 0xa5a4a4f65301f601ULL;\
    ((u64*)T)[0x649] = 0xa176764decd74dd7ULL;\
    ((u64*)T)[0x64A] = 0x14b7b76175a361a3ULL;\
    ((u64*)T)[0x64B] = 0x347d7dcefa49ce49ULL;\
    ((u64*)T)[0x64C] = 0xdf52527ba48d7b8dULL;\
    ((u64*)T)[0x64D] = 0x9fdddd3ea1423e42ULL;\
    ((u64*)T)[0x64E] = 0xcd5e5e71bc937193ULL;\
    ((u64*)T)[0x64F] = 0xb113139726a297a2ULL;\
    ((u64*)T)[0x650] = 0xa2a6a6f55704f504ULL;\
    ((u64*)T)[0x651] = 0x01b9b96869b868b8ULL;\
    ((u64*)T)[0x652] = 0x0000000000000000ULL;\
    ((u64*)T)[0x653] = 0xb5c1c12c99742c74ULL;\
    ((u64*)T)[0x654] = 0xe040406080a060a0ULL;\
    ((u64*)T)[0x655] = 0xc2e3e31fdd211f21ULL;\
    ((u64*)T)[0x656] = 0x3a7979c8f243c843ULL;\
    ((u64*)T)[0x657] = 0x9ab6b6ed772ced2cULL;\
    ((u64*)T)[0x658] = 0x0dd4d4beb3d9bed9ULL;\
    ((u64*)T)[0x659] = 0x478d8d4601ca46caULL;\
    ((u64*)T)[0x65A] = 0x176767d9ce70d970ULL;\
    ((u64*)T)[0x65B] = 0xaf72724be4dd4bddULL;\
    ((u64*)T)[0x65C] = 0xed9494de3379de79ULL;\
    ((u64*)T)[0x65D] = 0xff9898d42b67d467ULL;\
    ((u64*)T)[0x65E] = 0x93b0b0e87b23e823ULL;\
    ((u64*)T)[0x65F] = 0x5b85854a11de4adeULL;\
    ((u64*)T)[0x660] = 0x06bbbb6b6dbd6bbdULL;\
    ((u64*)T)[0x661] = 0xbbc5c52a917e2a7eULL;\
    ((u64*)T)[0x662] = 0x7b4f4fe59e34e534ULL;\
    ((u64*)T)[0x663] = 0xd7eded16c13a163aULL;\
    ((u64*)T)[0x664] = 0xd28686c51754c554ULL;\
    ((u64*)T)[0x665] = 0xf89a9ad72f62d762ULL;\
    ((u64*)T)[0x666] = 0x99666655ccff55ffULL;\
    ((u64*)T)[0x667] = 0xb611119422a794a7ULL;\
    ((u64*)T)[0x668] = 0xc08a8acf0f4acf4aULL;\
    ((u64*)T)[0x669] = 0xd9e9e910c9301030ULL;\
    ((u64*)T)[0x66A] = 0x0e040406080a060aULL;\
    ((u64*)T)[0x66B] = 0x66fefe81e7988198ULL;\
    ((u64*)T)[0x66C] = 0xaba0a0f05b0bf00bULL;\
    ((u64*)T)[0x66D] = 0xb4787844f0cc44ccULL;\
    ((u64*)T)[0x66E] = 0xf02525ba4ad5bad5ULL;\
    ((u64*)T)[0x66F] = 0x754b4be3963ee33eULL;\
    ((u64*)T)[0x670] = 0xaca2a2f35f0ef30eULL;\
    ((u64*)T)[0x671] = 0x445d5dfeba19fe19ULL;\
    ((u64*)T)[0x672] = 0xdb8080c01b5bc05bULL;\
    ((u64*)T)[0x673] = 0x8005058a0a858a85ULL;\
    ((u64*)T)[0x674] = 0xd33f3fad7eecadecULL;\
    ((u64*)T)[0x675] = 0xfe2121bc42dfbcdfULL;\
    ((u64*)T)[0x676] = 0xa8707048e0d848d8ULL;\
    ((u64*)T)[0x677] = 0xfdf1f104f90c040cULL;\
    ((u64*)T)[0x678] = 0x196363dfc67adf7aULL;\
    ((u64*)T)[0x679] = 0x2f7777c1ee58c158ULL;\
    ((u64*)T)[0x67A] = 0x30afaf75459f759fULL;\
    ((u64*)T)[0x67B] = 0xe742426384a563a5ULL;\
    ((u64*)T)[0x67C] = 0x7020203040503050ULL;\
    ((u64*)T)[0x67D] = 0xcbe5e51ad12e1a2eULL;\
    ((u64*)T)[0x67E] = 0xeffdfd0ee1120e12ULL;\
    ((u64*)T)[0x67F] = 0x08bfbf6d65b76db7ULL;\
    ((u64*)T)[0x680] = 0x5581814c19d44cd4ULL;\
    ((u64*)T)[0x681] = 0x24181814303c143cULL;\
    ((u64*)T)[0x682] = 0x792626354c5f355fULL;\
    ((u64*)T)[0x683] = 0xb2c3c32f9d712f71ULL;\
    ((u64*)T)[0x684] = 0x86bebee16738e138ULL;\
    ((u64*)T)[0x685] = 0xc83535a26afda2fdULL;\
    ((u64*)T)[0x686] = 0xc78888cc0b4fcc4fULL;\
    ((u64*)T)[0x687] = 0x652e2e395c4b394bULL;\
    ((u64*)T)[0x688] = 0x6a9393573df957f9ULL;\
    ((u64*)T)[0x689] = 0x585555f2aa0df20dULL;\
    ((u64*)T)[0x68A] = 0x61fcfc82e39d829dULL;\
    ((u64*)T)[0x68B] = 0xb37a7a47f4c947c9ULL;\
    ((u64*)T)[0x68C] = 0x27c8c8ac8befacefULL;\
    ((u64*)T)[0x68D] = 0x88babae76f32e732ULL;\
    ((u64*)T)[0x68E] = 0x4f32322b647d2b7dULL;\
    ((u64*)T)[0x68F] = 0x42e6e695d7a495a4ULL;\
    ((u64*)T)[0x690] = 0x3bc0c0a09bfba0fbULL;\
    ((u64*)T)[0x691] = 0xaa19199832b398b3ULL;\
    ((u64*)T)[0x692] = 0xf69e9ed12768d168ULL;\
    ((u64*)T)[0x693] = 0x22a3a37f5d817f81ULL;\
    ((u64*)T)[0x694] = 0xee44446688aa66aaULL;\
    ((u64*)T)[0x695] = 0xd654547ea8827e82ULL;\
    ((u64*)T)[0x696] = 0xdd3b3bab76e6abe6ULL;\
    ((u64*)T)[0x697] = 0x950b0b83169e839eULL;\
    ((u64*)T)[0x698] = 0xc98c8cca0345ca45ULL;\
    ((u64*)T)[0x699] = 0xbcc7c729957b297bULL;\
    ((u64*)T)[0x69A] = 0x056b6bd3d66ed36eULL;\
    ((u64*)T)[0x69B] = 0x6c28283c50443c44ULL;\
    ((u64*)T)[0x69C] = 0x2ca7a779558b798bULL;\
    ((u64*)T)[0x69D] = 0x81bcbce2633de23dULL;\
    ((u64*)T)[0x69E] = 0x3116161d2c271d27ULL;\
    ((u64*)T)[0x69F] = 0x37adad76419a769aULL;\
    ((u64*)T)[0x6A0] = 0x96dbdb3bad4d3b4dULL;\
    ((u64*)T)[0x6A1] = 0x9e646456c8fa56faULL;\
    ((u64*)T)[0x6A2] = 0xa674744ee8d24ed2ULL;\
    ((u64*)T)[0x6A3] = 0x3614141e28221e22ULL;\
    ((u64*)T)[0x6A4] = 0xe49292db3f76db76ULL;\
    ((u64*)T)[0x6A5] = 0x120c0c0a181e0a1eULL;\
    ((u64*)T)[0x6A6] = 0xfc48486c90b46cb4ULL;\
    ((u64*)T)[0x6A7] = 0x8fb8b8e46b37e437ULL;\
    ((u64*)T)[0x6A8] = 0x789f9f5d25e75de7ULL;\
    ((u64*)T)[0x6A9] = 0x0fbdbd6e61b26eb2ULL;\
    ((u64*)T)[0x6AA] = 0x694343ef862aef2aULL;\
    ((u64*)T)[0x6AB] = 0x35c4c4a693f1a6f1ULL;\
    ((u64*)T)[0x6AC] = 0xda3939a872e3a8e3ULL;\
    ((u64*)T)[0x6AD] = 0xc63131a462f7a4f7ULL;\
    ((u64*)T)[0x6AE] = 0x8ad3d337bd593759ULL;\
    ((u64*)T)[0x6AF] = 0x74f2f28bff868b86ULL;\
    ((u64*)T)[0x6B0] = 0x83d5d532b1563256ULL;\
    ((u64*)T)[0x6B1] = 0x4e8b8b430dc543c5ULL;\
    ((u64*)T)[0x6B2] = 0x856e6e59dceb59ebULL;\
    ((u64*)T)[0x6B3] = 0x18dadab7afc2b7c2ULL;\
    ((u64*)T)[0x6B4] = 0x8e01018c028f8c8fULL;\
    ((u64*)T)[0x6B5] = 0x1db1b16479ac64acULL;\
    ((u64*)T)[0x6B6] = 0xf19c9cd2236dd26dULL;\
    ((u64*)T)[0x6B7] = 0x724949e0923be03bULL;\
    ((u64*)T)[0x6B8] = 0x1fd8d8b4abc7b4c7ULL;\
    ((u64*)T)[0x6B9] = 0xb9acacfa4315fa15ULL;\
    ((u64*)T)[0x6BA] = 0xfaf3f307fd090709ULL;\
    ((u64*)T)[0x6BB] = 0xa0cfcf25856f256fULL;\
    ((u64*)T)[0x6BC] = 0x20cacaaf8feaafeaULL;\
    ((u64*)T)[0x6BD] = 0x7df4f48ef3898e89ULL;\
    ((u64*)T)[0x6BE] = 0x674747e98e20e920ULL;\
    ((u64*)T)[0x6BF] = 0x3810101820281828ULL;\
    ((u64*)T)[0x6C0] = 0x0b6f6fd5de64d564ULL;\
    ((u64*)T)[0x6C1] = 0x73f0f088fb838883ULL;\
    ((u64*)T)[0x6C2] = 0xfb4a4a6f94b16fb1ULL;\
    ((u64*)T)[0x6C3] = 0xca5c5c72b8967296ULL;\
    ((u64*)T)[0x6C4] = 0x54383824706c246cULL;\
    ((u64*)T)[0x6C5] = 0x5f5757f1ae08f108ULL;\
    ((u64*)T)[0x6C6] = 0x217373c7e652c752ULL;\
    ((u64*)T)[0x6C7] = 0x6497975135f351f3ULL;\
    ((u64*)T)[0x6C8] = 0xaecbcb238d652365ULL;\
    ((u64*)T)[0x6C9] = 0x25a1a17c59847c84ULL;\
    ((u64*)T)[0x6CA] = 0x57e8e89ccbbf9cbfULL;\
    ((u64*)T)[0x6CB] = 0x5d3e3e217c632163ULL;\
    ((u64*)T)[0x6CC] = 0xea9696dd377cdd7cULL;\
    ((u64*)T)[0x6CD] = 0x1e6161dcc27fdc7fULL;\
    ((u64*)T)[0x6CE] = 0x9c0d0d861a918691ULL;\
    ((u64*)T)[0x6CF] = 0x9b0f0f851e948594ULL;\
    ((u64*)T)[0x6D0] = 0x4be0e090dbab90abULL;\
    ((u64*)T)[0x6D1] = 0xba7c7c42f8c642c6ULL;\
    ((u64*)T)[0x6D2] = 0x267171c4e257c457ULL;\
    ((u64*)T)[0x6D3] = 0x29ccccaa83e5aae5ULL;\
    ((u64*)T)[0x6D4] = 0xe39090d83b73d873ULL;\
    ((u64*)T)[0x6D5] = 0x090606050c0f050fULL;\
    ((u64*)T)[0x6D6] = 0xf4f7f701f5030103ULL;\
    ((u64*)T)[0x6D7] = 0x2a1c1c1238361236ULL;\
    ((u64*)T)[0x6D8] = 0x3cc2c2a39ffea3feULL;\
    ((u64*)T)[0x6D9] = 0x8b6a6a5fd4e15fe1ULL;\
    ((u64*)T)[0x6DA] = 0xbeaeaef94710f910ULL;\
    ((u64*)T)[0x6DB] = 0x026969d0d26bd06bULL;\
    ((u64*)T)[0x6DC] = 0xbf1717912ea891a8ULL;\
    ((u64*)T)[0x6DD] = 0x7199995829e858e8ULL;\
    ((u64*)T)[0x6DE] = 0x533a3a2774692769ULL;\
    ((u64*)T)[0x6DF] = 0xf72727b94ed0b9d0ULL;\
    ((u64*)T)[0x6E0] = 0x91d9d938a9483848ULL;\
    ((u64*)T)[0x6E1] = 0xdeebeb13cd351335ULL;\
    ((u64*)T)[0x6E2] = 0xe52b2bb356ceb3ceULL;\
    ((u64*)T)[0x6E3] = 0x7722223344553355ULL;\
    ((u64*)T)[0x6E4] = 0x04d2d2bbbfd6bbd6ULL;\
    ((u64*)T)[0x6E5] = 0x39a9a97049907090ULL;\
    ((u64*)T)[0x6E6] = 0x870707890e808980ULL;\
    ((u64*)T)[0x6E7] = 0xc13333a766f2a7f2ULL;\
    ((u64*)T)[0x6E8] = 0xec2d2db65ac1b6c1ULL;\
    ((u64*)T)[0x6E9] = 0x5a3c3c2278662266ULL;\
    ((u64*)T)[0x6EA] = 0xb81515922aad92adULL;\
    ((u64*)T)[0x6EB] = 0xa9c9c92089602060ULL;\
    ((u64*)T)[0x6EC] = 0x5c87874915db49dbULL;\
    ((u64*)T)[0x6ED] = 0xb0aaaaff4f1aff1aULL;\
    ((u64*)T)[0x6EE] = 0xd8505078a0887888ULL;\
    ((u64*)T)[0x6EF] = 0x2ba5a57a518e7a8eULL;\
    ((u64*)T)[0x6F0] = 0x8903038f068a8f8aULL;\
    ((u64*)T)[0x6F1] = 0x4a5959f8b213f813ULL;\
    ((u64*)T)[0x6F2] = 0x92090980129b809bULL;\
    ((u64*)T)[0x6F3] = 0x231a1a1734391739ULL;\
    ((u64*)T)[0x6F4] = 0x106565daca75da75ULL;\
    ((u64*)T)[0x6F5] = 0x84d7d731b5533153ULL;\
    ((u64*)T)[0x6F6] = 0xd58484c61351c651ULL;\
    ((u64*)T)[0x6F7] = 0x03d0d0b8bbd3b8d3ULL;\
    ((u64*)T)[0x6F8] = 0xdc8282c31f5ec35eULL;\
    ((u64*)T)[0x6F9] = 0xe22929b052cbb0cbULL;\
    ((u64*)T)[0x6FA] = 0xc35a5a77b4997799ULL;\
    ((u64*)T)[0x6FB] = 0x2d1e1e113c331133ULL;\
    ((u64*)T)[0x6FC] = 0x3d7b7bcbf646cb46ULL;\
    ((u64*)T)[0x6FD] = 0xb7a8a8fc4b1ffc1fULL;\
    ((u64*)T)[0x6FE] = 0x0c6d6dd6da61d661ULL;\
    ((u64*)T)[0x6FF] = 0x622c2c3a584e3a4eULL;\
    ((u64*)T)[0x700] = 0xc6c6a597f4a5f432ULL;\
    ((u64*)T)[0x701] = 0xf8f884eb9784976fULL;\
    ((u64*)T)[0x702] = 0xeeee99c7b099b05eULL;\
    ((u64*)T)[0x703] = 0xf6f68df78c8d8c7aULL;\
    ((u64*)T)[0x704] = 0xffff0de5170d17e8ULL;\
    ((u64*)T)[0x705] = 0xd6d6bdb7dcbddc0aULL;\
    ((u64*)T)[0x706] = 0xdedeb1a7c8b1c816ULL;\
    ((u64*)T)[0x707] = 0x91915439fc54fc6dULL;\
    ((u64*)T)[0x708] = 0x606050c0f050f090ULL;\
    ((u64*)T)[0x709] = 0x0202030405030507ULL;\
    ((u64*)T)[0x70A] = 0xcecea987e0a9e02eULL;\
    ((u64*)T)[0x70B] = 0x56567dac877d87d1ULL;\
    ((u64*)T)[0x70C] = 0xe7e719d52b192bccULL;\
    ((u64*)T)[0x70D] = 0xb5b56271a662a613ULL;\
    ((u64*)T)[0x70E] = 0x4d4de69a31e6317cULL;\
    ((u64*)T)[0x70F] = 0xecec9ac3b59ab559ULL;\
    ((u64*)T)[0x710] = 0x8f8f4505cf45cf40ULL;\
    ((u64*)T)[0x711] = 0x1f1f9d3ebc9dbca3ULL;\
    ((u64*)T)[0x712] = 0x89894009c040c049ULL;\
    ((u64*)T)[0x713] = 0xfafa87ef92879268ULL;\
    ((u64*)T)[0x714] = 0xefef15c53f153fd0ULL;\
    ((u64*)T)[0x715] = 0xb2b2eb7f26eb2694ULL;\
    ((u64*)T)[0x716] = 0x8e8ec90740c940ceULL;\
    ((u64*)T)[0x717] = 0xfbfb0bed1d0b1de6ULL;\
    ((u64*)T)[0x718] = 0x4141ec822fec2f6eULL;\
    ((u64*)T)[0x719] = 0xb3b3677da967a91aULL;\
    ((u64*)T)[0x71A] = 0x5f5ffdbe1cfd1c43ULL;\
    ((u64*)T)[0x71B] = 0x4545ea8a25ea2560ULL;\
    ((u64*)T)[0x71C] = 0x2323bf46dabfdaf9ULL;\
    ((u64*)T)[0x71D] = 0x5353f7a602f70251ULL;\
    ((u64*)T)[0x71E] = 0xe4e496d3a196a145ULL;\
    ((u64*)T)[0x71F] = 0x9b9b5b2ded5bed76ULL;\
    ((u64*)T)[0x720] = 0x7575c2ea5dc25d28ULL;\
    ((u64*)T)[0x721] = 0xe1e11cd9241c24c5ULL;\
    ((u64*)T)[0x722] = 0x3d3dae7ae9aee9d4ULL;\
    ((u64*)T)[0x723] = 0x4c4c6a98be6abef2ULL;\
    ((u64*)T)[0x724] = 0x6c6c5ad8ee5aee82ULL;\
    ((u64*)T)[0x725] = 0x7e7e41fcc341c3bdULL;\
    ((u64*)T)[0x726] = 0xf5f502f1060206f3ULL;\
    ((u64*)T)[0x727] = 0x83834f1dd14fd152ULL;\
    ((u64*)T)[0x728] = 0x68685cd0e45ce48cULL;\
    ((u64*)T)[0x729] = 0x5151f4a207f40756ULL;\
    ((u64*)T)[0x72A] = 0xd1d134b95c345c8dULL;\
    ((u64*)T)[0x72B] = 0xf9f908e9180818e1ULL;\
    ((u64*)T)[0x72C] = 0xe2e293dfae93ae4cULL;\
    ((u64*)T)[0x72D] = 0xabab734d9573953eULL;\
    ((u64*)T)[0x72E] = 0x626253c4f553f597ULL;\
    ((u64*)T)[0x72F] = 0x2a2a3f54413f416bULL;\
    ((u64*)T)[0x730] = 0x08080c10140c141cULL;\
    ((u64*)T)[0x731] = 0x95955231f652f663ULL;\
    ((u64*)T)[0x732] = 0x4646658caf65afe9ULL;\
    ((u64*)T)[0x733] = 0x9d9d5e21e25ee27fULL;\
    ((u64*)T)[0x734] = 0x3030286078287848ULL;\
    ((u64*)T)[0x735] = 0x3737a16ef8a1f8cfULL;\
    ((u64*)T)[0x736] = 0x0a0a0f14110f111bULL;\
    ((u64*)T)[0x737] = 0x2f2fb55ec4b5c4ebULL;\
    ((u64*)T)[0x738] = 0x0e0e091c1b091b15ULL;\
    ((u64*)T)[0x739] = 0x242436485a365a7eULL;\
    ((u64*)T)[0x73A] = 0x1b1b9b36b69bb6adULL;\
    ((u64*)T)[0x73B] = 0xdfdf3da5473d4798ULL;\
    ((u64*)T)[0x73C] = 0xcdcd26816a266aa7ULL;\
    ((u64*)T)[0x73D] = 0x4e4e699cbb69bbf5ULL;\
    ((u64*)T)[0x73E] = 0x7f7fcdfe4ccd4c33ULL;\
    ((u64*)T)[0x73F] = 0xeaea9fcfba9fba50ULL;\
    ((u64*)T)[0x740] = 0x12121b242d1b2d3fULL;\
    ((u64*)T)[0x741] = 0x1d1d9e3ab99eb9a4ULL;\
    ((u64*)T)[0x742] = 0x585874b09c749cc4ULL;\
    ((u64*)T)[0x743] = 0x34342e68722e7246ULL;\
    ((u64*)T)[0x744] = 0x36362d6c772d7741ULL;\
    ((u64*)T)[0x745] = 0xdcdcb2a3cdb2cd11ULL;\
    ((u64*)T)[0x746] = 0xb4b4ee7329ee299dULL;\
    ((u64*)T)[0x747] = 0x5b5bfbb616fb164dULL;\
    ((u64*)T)[0x748] = 0xa4a4f65301f601a5ULL;\
    ((u64*)T)[0x749] = 0x76764decd74dd7a1ULL;\
    ((u64*)T)[0x74A] = 0xb7b76175a361a314ULL;\
    ((u64*)T)[0x74B] = 0x7d7dcefa49ce4934ULL;\
    ((u64*)T)[0x74C] = 0x52527ba48d7b8ddfULL;\
    ((u64*)T)[0x74D] = 0xdddd3ea1423e429fULL;\
    ((u64*)T)[0x74E] = 0x5e5e71bc937193cdULL;\
    ((u64*)T)[0x74F] = 0x13139726a297a2b1ULL;\
    ((u64*)T)[0x750] = 0xa6a6f55704f504a2ULL;\
    ((u64*)T)[0x751] = 0xb9b96869b868b801ULL;\
    ((u64*)T)[0x752] = 0x0000000000000000ULL;\
    ((u64*)T)[0x753] = 0xc1c12c99742c74b5ULL;\
    ((u64*)T)[0x754] = 0x40406080a060a0e0ULL;\
    ((u64*)T)[0x755] = 0xe3e31fdd211f21c2ULL;\
    ((u64*)T)[0x756] = 0x7979c8f243c8433aULL;\
    ((u64*)T)[0x757] = 0xb6b6ed772ced2c9aULL;\
    ((u64*)T)[0x758] = 0xd4d4beb3d9bed90dULL;\
    ((u64*)T)[0x759] = 0x8d8d4601ca46ca47ULL;\
    ((u64*)T)[0x75A] = 0x6767d9ce70d97017ULL;\
    ((u64*)T)[0x75B] = 0x72724be4dd4bddafULL;\
    ((u64*)T)[0x75C] = 0x9494de3379de79edULL;\
    ((u64*)T)[0x75D] = 0x9898d42b67d467ffULL;\
    ((u64*)T)[0x75E] = 0xb0b0e87b23e82393ULL;\
    ((u64*)T)[0x75F] = 0x85854a11de4ade5bULL;\
    ((u64*)T)[0x760] = 0xbbbb6b6dbd6bbd06ULL;\
    ((u64*)T)[0x761] = 0xc5c52a917e2a7ebbULL;\
    ((u64*)T)[0x762] = 0x4f4fe59e34e5347bULL;\
    ((u64*)T)[0x763] = 0xeded16c13a163ad7ULL;\
    ((u64*)T)[0x764] = 0x8686c51754c554d2ULL;\
    ((u64*)T)[0x765] = 0x9a9ad72f62d762f8ULL;\
    ((u64*)T)[0x766] = 0x666655ccff55ff99ULL;\
    ((u64*)T)[0x767] = 0x11119422a794a7b6ULL;\
    ((u64*)T)[0x768] = 0x8a8acf0f4acf4ac0ULL;\
    ((u64*)T)[0x769] = 0xe9e910c9301030d9ULL;\
    ((u64*)T)[0x76A] = 0x040406080a060a0eULL;\
    ((u64*)T)[0x76B] = 0xfefe81e798819866ULL;\
    ((u64*)T)[0x76C] = 0xa0a0f05b0bf00babULL;\
    ((u64*)T)[0x76D] = 0x787844f0cc44ccb4ULL;\
    ((u64*)T)[0x76E] = 0x2525ba4ad5bad5f0ULL;\
    ((u64*)T)[0x76F] = 0x4b4be3963ee33e75ULL;\
    ((u64*)T)[0x770] = 0xa2a2f35f0ef30eacULL;\
    ((u64*)T)[0x771] = 0x5d5dfeba19fe1944ULL;\
    ((u64*)T)[0x772] = 0x8080c01b5bc05bdbULL;\
    ((u64*)T)[0x773] = 0x05058a0a858a8580ULL;\
    ((u64*)T)[0x774] = 0x3f3fad7eecadecd3ULL;\
    ((u64*)T)[0x775] = 0x2121bc42dfbcdffeULL;\
    ((u64*)T)[0x776] = 0x707048e0d848d8a8ULL;\
    ((u64*)T)[0x777] = 0xf1f104f90c040cfdULL;\
    ((u64*)T)[0x778] = 0x6363dfc67adf7a19ULL;\
    ((u64*)T)[0x779] = 0x7777c1ee58c1582fULL;\
    ((u64*)T)[0x77A] = 0xafaf75459f759f30ULL;\
    ((u64*)T)[0x77B] = 0x42426384a563a5e7ULL;\
    ((u64*)T)[0x77C] = 0x2020304050305070ULL;\
    ((u64*)T)[0x77D] = 0xe5e51ad12e1a2ecbULL;\
    ((u64*)T)[0x77E] = 0xfdfd0ee1120e12efULL;\
    ((u64*)T)[0x77F] = 0xbfbf6d65b76db708ULL;\
    ((u64*)T)[0x780] = 0x81814c19d44cd455ULL;\
    ((u64*)T)[0x781] = 0x181814303c143c24ULL;\
    ((u64*)T)[0x782] = 0x2626354c5f355f79ULL;\
    ((u64*)T)[0x783] = 0xc3c32f9d712f71b2ULL;\
    ((u64*)T)[0x784] = 0xbebee16738e13886ULL;\
    ((u64*)T)[0x785] = 0x3535a26afda2fdc8ULL;\
    ((u64*)T)[0x786] = 0x8888cc0b4fcc4fc7ULL;\
    ((u64*)T)[0x787] = 0x2e2e395c4b394b65ULL;\
    ((u64*)T)[0x788] = 0x9393573df957f96aULL;\
    ((u64*)T)[0x789] = 0x5555f2aa0df20d58ULL;\
    ((u64*)T)[0x78A] = 0xfcfc82e39d829d61ULL;\
    ((u64*)T)[0x78B] = 0x7a7a47f4c947c9b3ULL;\
    ((u64*)T)[0x78C] = 0xc8c8ac8befacef27ULL;\
    ((u64*)T)[0x78D] = 0xbabae76f32e73288ULL;\
    ((u64*)T)[0x78E] = 0x32322b647d2b7d4fULL;\
    ((u64*)T)[0x78F] = 0xe6e695d7a495a442ULL;\
    ((u64*)T)[0x790] = 0xc0c0a09bfba0fb3bULL;\
    ((u64*)T)[0x791] = 0x19199832b398b3aaULL;\
    ((u64*)T)[0x792] = 0x9e9ed12768d168f6ULL;\
    ((u64*)T)[0x793] = 0xa3a37f5d817f8122ULL;\
    ((u64*)T)[0x794] = 0x44446688aa66aaeeULL;\
    ((u64*)T)[0x795] = 0x54547ea8827e82d6ULL;\
    ((u64*)T)[0x796] = 0x3b3bab76e6abe6ddULL;\
    ((u64*)T)[0x797] = 0x0b0b83169e839e95ULL;\
    ((u64*)T)[0x798] = 0x8c8cca0345ca45c9ULL;\
    ((u64*)T)[0x799] = 0xc7c729957b297bbcULL;\
    ((u64*)T)[0x79A] = 0x6b6bd3d66ed36e05ULL;\
    ((u64*)T)[0x79B] = 0x28283c50443c446cULL;\
    ((u64*)T)[0x79C] = 0xa7a779558b798b2cULL;\
    ((u64*)T)[0x79D] = 0xbcbce2633de23d81ULL;\
    ((u64*)T)[0x79E] = 0x16161d2c271d2731ULL;\
    ((u64*)T)[0x79F] = 0xadad76419a769a37ULL;\
    ((u64*)T)[0x7A0] = 0xdbdb3bad4d3b4d96ULL;\
    ((u64*)T)[0x7A1] = 0x646456c8fa56fa9eULL;\
    ((u64*)T)[0x7A2] = 0x74744ee8d24ed2a6ULL;\
    ((u64*)T)[0x7A3] = 0x14141e28221e2236ULL;\
    ((u64*)T)[0x7A4] = 0x9292db3f76db76e4ULL;\
    ((u64*)T)[0x7A5] = 0x0c0c0a181e0a1e12ULL;\
    ((u64*)T)[0x7A6] = 0x48486c90b46cb4fcULL;\
    ((u64*)T)[0x7A7] = 0xb8b8e46b37e4378fULL;\
    ((u64*)T)[0x7A8] = 0x9f9f5d25e75de778ULL;\
    ((u64*)T)[0x7A9] = 0xbdbd6e61b26eb20fULL;\
    ((u64*)T)[0x7AA] = 0x4343ef862aef2a69ULL;\
    ((u64*)T)[0x7AB] = 0xc4c4a693f1a6f135ULL;\
    ((u64*)T)[0x7AC] = 0x3939a872e3a8e3daULL;\
    ((u64*)T)[0x7AD] = 0x3131a462f7a4f7c6ULL;\
    ((u64*)T)[0x7AE] = 0xd3d337bd5937598aULL;\
    ((u64*)T)[0x7AF] = 0xf2f28bff868b8674ULL;\
    ((u64*)T)[0x7B0] = 0xd5d532b156325683ULL;\
    ((u64*)T)[0x7B1] = 0x8b8b430dc543c54eULL;\
    ((u64*)T)[0x7B2] = 0x6e6e59dceb59eb85ULL;\
    ((u64*)T)[0x7B3] = 0xdadab7afc2b7c218ULL;\
    ((u64*)T)[0x7B4] = 0x01018c028f8c8f8eULL;\
    ((u64*)T)[0x7B5] = 0xb1b16479ac64ac1dULL;\
    ((u64*)T)[0x7B6] = 0x9c9cd2236dd26df1ULL;\
    ((u64*)T)[0x7B7] = 0x4949e0923be03b72ULL;\
    ((u64*)T)[0x7B8] = 0xd8d8b4abc7b4c71fULL;\
    ((u64*)T)[0x7B9] = 0xacacfa4315fa15b9ULL;\
    ((u64*)T)[0x7BA] = 0xf3f307fd090709faULL;\
    ((u64*)T)[0x7BB] = 0xcfcf25856f256fa0ULL;\
    ((u64*)T)[0x7BC] = 0xcacaaf8feaafea20ULL;\
    ((u64*)T)[0x7BD] = 0xf4f48ef3898e897dULL;\
    ((u64*)T)[0x7BE] = 0x4747e98e20e92067ULL;\
    ((u64*)T)[0x7BF] = 0x1010182028182838ULL;\
    ((u64*)T)[0x7C0] = 0x6f6fd5de64d5640bULL;\
    ((u64*)T)[0x7C1] = 0xf0f088fb83888373ULL;\
    ((u64*)T)[0x7C2] = 0x4a4a6f94b16fb1fbULL;\
    ((u64*)T)[0x7C3] = 0x5c5c72b8967296caULL;\
    ((u64*)T)[0x7C4] = 0x383824706c246c54ULL;\
    ((u64*)T)[0x7C5] = 0x5757f1ae08f1085fULL;\
    ((u64*)T)[0x7C6] = 0x7373c7e652c75221ULL;\
    ((u64*)T)[0x7C7] = 0x97975135f351f364ULL;\
    ((u64*)T)[0x7C8] = 0xcbcb238d652365aeULL;\
    ((u64*)T)[0x7C9] = 0xa1a17c59847c8425ULL;\
    ((u64*)T)[0x7CA] = 0xe8e89ccbbf9cbf57ULL;\
    ((u64*)T)[0x7CB] = 0x3e3e217c6321635dULL;\
    ((u64*)T)[0x7CC] = 0x9696dd377cdd7ceaULL;\
    ((u64*)T)[0x7CD] = 0x6161dcc27fdc7f1eULL;\
    ((u64*)T)[0x7CE] = 0x0d0d861a9186919cULL;\
    ((u64*)T)[0x7CF] = 0x0f0f851e9485949bULL;\
    ((u64*)T)[0x7D0] = 0xe0e090dbab90ab4bULL;\
    ((u64*)T)[0x7D1] = 0x7c7c42f8c642c6baULL;\
    ((u64*)T)[0x7D2] = 0x7171c4e257c45726ULL;\
    ((u64*)T)[0x7D3] = 0xccccaa83e5aae529ULL;\
    ((u64*)T)[0x7D4] = 0x9090d83b73d873e3ULL;\
    ((u64*)T)[0x7D5] = 0x0606050c0f050f09ULL;\
    ((u64*)T)[0x7D6] = 0xf7f701f5030103f4ULL;\
    ((u64*)T)[0x7D7] = 0x1c1c12383612362aULL;\
    ((u64*)T)[0x7D8] = 0xc2c2a39ffea3fe3cULL;\
    ((u64*)T)[0x7D9] = 0x6a6a5fd4e15fe18bULL;\
    ((u64*)T)[0x7DA] = 0xaeaef94710f910beULL;\
    ((u64*)T)[0x7DB] = 0x6969d0d26bd06b02ULL;\
    ((u64*)T)[0x7DC] = 0x1717912ea891a8bfULL;\
    ((u64*)T)[0x7DD] = 0x99995829e858e871ULL;\
    ((u64*)T)[0x7DE] = 0x3a3a277469276953ULL;\
    ((u64*)T)[0x7DF] = 0x2727b94ed0b9d0f7ULL;\
    ((u64*)T)[0x7E0] = 0xd9d938a948384891ULL;\
    ((u64*)T)[0x7E1] = 0xebeb13cd351335deULL;\
    ((u64*)T)[0x7E2] = 0x2b2bb356ceb3cee5ULL;\
    ((u64*)T)[0x7E3] = 0x2222334455335577ULL;\
    ((u64*)T)[0x7E4] = 0xd2d2bbbfd6bbd604ULL;\
    ((u64*)T)[0x7E5] = 0xa9a9704990709039ULL;\
    ((u64*)T)[0x7E6] = 0x0707890e80898087ULL;\
    ((u64*)T)[0x7E7] = 0x3333a766f2a7f2c1ULL;\
    ((u64*)T)[0x7E8] = 0x2d2db65ac1b6c1ecULL;\
    ((u64*)T)[0x7E9] = 0x3c3c22786622665aULL;\
    ((u64*)T)[0x7EA] = 0x1515922aad92adb8ULL;\
    ((u64*)T)[0x7EB] = 0xc9c92089602060a9ULL;\
    ((u64*)T)[0x7EC] = 0x87874915db49db5cULL;\
    ((u64*)T)[0x7ED] = 0xaaaaff4f1aff1ab0ULL;\
    ((u64*)T)[0x7EE] = 0x505078a0887888d8ULL;\
    ((u64*)T)[0x7EF] = 0xa5a57a518e7a8e2bULL;\
    ((u64*)T)[0x7F0] = 0x03038f068a8f8a89ULL;\
    ((u64*)T)[0x7F1] = 0x5959f8b213f8134aULL;\
    ((u64*)T)[0x7F2] = 0x090980129b809b92ULL;\
    ((u64*)T)[0x7F3] = 0x1a1a173439173923ULL;\
    ((u64*)T)[0x7F4] = 0x6565daca75da7510ULL;\
    ((u64*)T)[0x7F5] = 0xd7d731b553315384ULL;\
    ((u64*)T)[0x7F6] = 0x8484c61351c651d5ULL;\
    ((u64*)T)[0x7F7] = 0xd0d0b8bbd3b8d303ULL;\
    ((u64*)T)[0x7F8] = 0x8282c31f5ec35edcULL;\
    ((u64*)T)[0x7F9] = 0x2929b052cbb0cbe2ULL;\
    ((u64*)T)[0x7FA] = 0x5a5a77b4997799c3ULL;\
    ((u64*)T)[0x7FB] = 0x1e1e113c3311332dULL;\
    ((u64*)T)[0x7FC] = 0x7b7bcbf646cb463dULL;\
    ((u64*)T)[0x7FD] = 0xa8a8fc4b1ffc1fb7ULL;\
    ((u64*)T)[0x7FE] = 0x6d6dd6da61d6610cULL;\
    ((u64*)T)[0x7FF] = 0x2c2c3a584e3a4e62ULL;\
};
