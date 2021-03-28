//DMHSW's "Kernel 8"
//A notation for state machine computing which reflects underlying hardware.
//Unknown if it will actually be useful for any computer nerds out there,
//but it's kinda cool

/*
********
What is a kernel?
********
A finite state transformation.
It maps from a finite domain of states into a finite range of states.

A kernel takes in a power-of-2 number of bytes and outputs some result of the same size.


Every single kernel can be replaced by a lookup table given the user has enough storage space to accomodate it.

The fundamental and most important part of a kernel is that it MUST ***ALWAYS*** operate ***ONLY*** on
the state it is passed- there are no "global variables"


Kernels are named by the following convention
kernelN is a kernel which operates on N *BITS*. not bytes, BITS.
kernelbN is a kernel which operates on 2^(n-1) *BYTES*. 
kernelpb is kernelbN, but with pass-by-pointer syntax. This is more expensive
for small states (<64 bytes) but absolutely essential with large states.

kernelb1 operates on a single byte. it is equivalent to "Kernel8"

kernelb10 operates on 512 bytes. it is equivalent to "Kernel4096"



******
What is a state?
******
Bits on your computers, in groupings of bytes.

state1 is a  1 byte state.
state2 is a  2 byte state.
state3 is a  4 byte state.
state4 is an 8 byte state.
state5 is a 16 byte state...

A kernel operates ONLY Using the state it is passed, and nothing else.

It will never return a different result if given the same state.

if it violates this rule, it is not a valid kernel, it is an ordinary function.

You can write a function which matches the syntax of a kernel (this might be useful)

but it is *not* a kernel even if the type system says it is.

*******
Known special properties of kernels
*******
1) Kernels are extremely trivial to optimize at compile time. 
2) Multithreading the operation of kernels is trivial since they never use external state.
3) Every state machine can be represented as a kernel. A single-threaded program can be encapsulated in one.
4) You can propagate error conditions and edge cases through a kernel tree.
5) Code written as kernel code is very often a direct mapping to real hardware instructions.
	This means that getting SIMD acceleration with portable code is extremely easy- just write the code
	that does what the simd instruction does in kernel code, and it will probably compile into the simd instruction.
	States are aligned to memory boundaries matching their size (Until 16, which is the largest useful on my machine)
6) Kernels are *already* serialized
	Big note:
	The endianness of your computer may affect serialization. use k_endian_cond_swap
	if you are trying to transfer state from one computer to another and you are taking advantage of
	a hardware floating point unit, you may find that your results vary with that as well.
7) C library code written as a kernel is incredibly portable (No external variables!)
8) it should be possible to write a static analyzer for your kernel code which can
	detect all possible errors by propagating edge cases. (This is something I am still thinking about)

	You might say "this is true for any finite state machine!" and you would be correct.
	However, even for a 128 bit arbitrary state machine, it would take centuries to
	brute force all 2^128 possible cases.

	With kernel code, there is one huge trick

	Since a kernel can be replaced with a lookup table, we can propagate domain restrictions
	into range restrictions. We can define contiguous "regions" of states to be possible while others impossible,
	and some erroneous while others not.	
9)  Functional programming paradigms- kernels have no side effects.
10) Kernel code can be created in ASIC/FPGA hardware, making this a form of HDL.

*/

#define KERNEL_FAST_FLOAT_MATH 1

#ifndef KERNELN_H
#define KERNELN_H

#if defined(_OPENMP)
#define PRAGMA_PARALLEL _Pragma("omp parallel for")
#define PRAGMA_SIMD _Pragma("omp simd")
#else
#define PRAGMA_PARALLEL _Pragma("acc loop")
#define PRAGMA_SIMD /*a comment*/
#endif

#include <stdint.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#ifndef KERNEL_NO_ALIGN
#include <stdalign.h>
#define KERNEL_ALIGN(n) alignas(n)
#else
#define KERNEL_ALIGN(n) /*a comment*/
#endif

//define a 2^(n-1) byte state, and kernel type,
//as well as common operations.
//These are the state member declarations...
//so you can do state30.state10s[3]
#define STATE_MEMBERS(n,alignment) STATE_MEMBERS_##n(alignment)
#define STATE_MEMBERS_1(alignment) /*a comment*/
#define STATE_MEMBERS_2(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<1];
#define STATE_MEMBERS_3(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<2];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<1];
#define STATE_MEMBERS_4(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<3];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<2];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<1];
#define STATE_MEMBERS_5(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<4];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<3];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<2];\
	KERNEL_ALIGN(alignment) state4 state4s[1<<1];
#define STATE_MEMBERS_6(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<5];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<4];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<3];\
	KERNEL_ALIGN(alignment) state4 state4s[1<<2];\
	KERNEL_ALIGN(alignment) state5 state5s[1<<1];
#define STATE_MEMBERS_7(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<6];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<5];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<4];\
	KERNEL_ALIGN(alignment) state4 state4s[1<<3];\
	KERNEL_ALIGN(alignment) state5 state5s[1<<2];\
	KERNEL_ALIGN(alignment) state6 state6s[1<<1];
#define STATE_MEMBERS_8(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<7];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<6];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<5];\
	KERNEL_ALIGN(alignment) state4 state4s[1<<4];\
	KERNEL_ALIGN(alignment) state5 state5s[1<<3];\
	KERNEL_ALIGN(alignment) state6 state6s[1<<2];\
	KERNEL_ALIGN(alignment) state7 state7s[1<<1];
#define STATE_MEMBERS_9(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<8];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<7];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<6];\
	KERNEL_ALIGN(alignment) state4 state4s[1<<5];\
	KERNEL_ALIGN(alignment) state5 state5s[1<<4];\
	KERNEL_ALIGN(alignment) state6 state6s[1<<3];\
	KERNEL_ALIGN(alignment) state7 state7s[1<<2];\
	KERNEL_ALIGN(alignment) state8 state8s[1<<1];
#define STATE_MEMBERS_10(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s[1<<9];\
	KERNEL_ALIGN(alignment) state2 state2s[1<<8];\
	KERNEL_ALIGN(alignment) state3 state3s[1<<7];\
	KERNEL_ALIGN(alignment) state4 state4s[1<<6];\
	KERNEL_ALIGN(alignment) state5 state5s[1<<5];\
	KERNEL_ALIGN(alignment) state6 state6s[1<<4];\
	KERNEL_ALIGN(alignment) state7 state7s[1<<3];\
	KERNEL_ALIGN(alignment) state8 state8s[1<<2];\
	KERNEL_ALIGN(alignment) state9 state9s[1<<1];
#define STATE_MEMBERS_11(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<10];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<9];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<8];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<7];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<6];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<5];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<4];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<3];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<2];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<1];
#define STATE_MEMBERS_12(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<11];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<10];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<9];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<8];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<7];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<6];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<5];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<4];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<3];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<2];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<1];
#define STATE_MEMBERS_13(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<12];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<11];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<10];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<9];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<8];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<7];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<6];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<5];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<4];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<3];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<2];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<1];
#define STATE_MEMBERS_14(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<13];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<12];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<11];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<10];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<9];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<8];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<7];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<6];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<5];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<4];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<3];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<2];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<1];
#define STATE_MEMBERS_15(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<14];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<13];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<12];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<11];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<10];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<9];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<8];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<7];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<6];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<5];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<4];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<3];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<2];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<1];
#define STATE_MEMBERS_16(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<15];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<14];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<13];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<12];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<11];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<10];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<9];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<8];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<7];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<6];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<5];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<4];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<3];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<2];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<1];
#define STATE_MEMBERS_17(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s	[1<<16];\
	KERNEL_ALIGN(alignment) state2 state2s	[1<<15];\
	KERNEL_ALIGN(alignment) state3 state3s	[1<<14];\
	KERNEL_ALIGN(alignment) state4 state4s	[1<<13];\
	KERNEL_ALIGN(alignment) state5 state5s	[1<<12];\
	KERNEL_ALIGN(alignment) state6 state6s	[1<<11];\
	KERNEL_ALIGN(alignment) state7 state7s	[1<<10];\
	KERNEL_ALIGN(alignment) state8 state8s	[1<<9];\
	KERNEL_ALIGN(alignment) state9 state9s	[1<<8];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<7];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<6];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<5];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<4];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<3];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<2];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<1];


#define STATE_MEMBERS_18(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<17];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<16];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<15];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<14];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<13];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<12];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<11];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<10];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<9];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<8];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<7];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<6];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<5];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<4];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<3];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<2];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<1];

#define STATE_MEMBERS_19(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<18];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<17];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<16];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<15];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<14];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<13];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<12];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<11];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<10];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<9];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<8];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<7];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<6];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<5];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<4];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<3];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<2];\
	KERNEL_ALIGN(alignment) state18 state18s[1<<1];

#define STATE_MEMBERS_20(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<19];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<18];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<17];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<16];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<15];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<14];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<13];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<12];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<11];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<10];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<9];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<8];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<7];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<6];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<5];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<4];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<3];\
	KERNEL_ALIGN(alignment) state18 state18s[1<<2];\
	KERNEL_ALIGN(alignment) state19 state19s[1<<1];

#define STATE_MEMBERS_21(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<20];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<19];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<18];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<17];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<16];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<15];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<14];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<13];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<12];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<11];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<10];\
	KERNEL_ALIGN(alignment) state12 state12s[1<< 9];\
	KERNEL_ALIGN(alignment) state13 state13s[1<< 8];\
	KERNEL_ALIGN(alignment) state14 state14s[1<< 7];\
	KERNEL_ALIGN(alignment) state15 state15s[1<< 6];\
	KERNEL_ALIGN(alignment) state16 state16s[1<< 5];\
	KERNEL_ALIGN(alignment) state17 state17s[1<< 4];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 3];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 2];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 1];

#define STATE_MEMBERS_22(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<21];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<20];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<19];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<18];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<17];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<16];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<15];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<14];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<13];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<12];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<11];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<10];\
	KERNEL_ALIGN(alignment) state13 state13s[1<< 9];\
	KERNEL_ALIGN(alignment) state14 state14s[1<< 8];\
	KERNEL_ALIGN(alignment) state15 state15s[1<< 7];\
	KERNEL_ALIGN(alignment) state16 state16s[1<< 6];\
	KERNEL_ALIGN(alignment) state17 state17s[1<< 5];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 4];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 3];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 2];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 1];

#define STATE_MEMBERS_23(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<22];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<21];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<20];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<19];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<18];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<17];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<16];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<15];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<14];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<13];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<12];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<11];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<10];\
	KERNEL_ALIGN(alignment) state14 state14s[1<< 9];\
	KERNEL_ALIGN(alignment) state15 state15s[1<< 8];\
	KERNEL_ALIGN(alignment) state16 state16s[1<< 7];\
	KERNEL_ALIGN(alignment) state17 state17s[1<< 6];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 5];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 4];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 3];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 2];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 1];
#define STATE_MEMBERS_24(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<23];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<22];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<21];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<20];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<19];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<18];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<17];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<16];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<15];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<14];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<13];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<12];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<11];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<10];\
	KERNEL_ALIGN(alignment) state15 state15s[1<< 9];\
	KERNEL_ALIGN(alignment) state16 state16s[1<< 8];\
	KERNEL_ALIGN(alignment) state17 state17s[1<< 7];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 6];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 5];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 4];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 3];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 2];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 1];

#define STATE_MEMBERS_25(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<24];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<23];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<22];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<21];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<20];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<19];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<18];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<17];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<16];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<15];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<14];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<13];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<12];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<11];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<10];\
	KERNEL_ALIGN(alignment) state16 state16s[1<< 9];\
	KERNEL_ALIGN(alignment) state17 state17s[1<< 8];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 7];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 6];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 5];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 4];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 3];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 2];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 1];

#define STATE_MEMBERS_26(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<25];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<24];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<23];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<22];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<21];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<20];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<19];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<18];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<17];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<16];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<15];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<14];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<13];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<12];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<11];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<10];\
	KERNEL_ALIGN(alignment) state17 state17s[1<< 9];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 8];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 7];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 6];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 5];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 4];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 3];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 2];\
	KERNEL_ALIGN(alignment) state25 state25s[1<< 1];

#define STATE_MEMBERS_27(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<26];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<25];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<24];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<23];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<22];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<21];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<20];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<19];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<18];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<17];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<16];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<15];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<14];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<13];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<12];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<11];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<10];\
	KERNEL_ALIGN(alignment) state18 state18s[1<< 9];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 8];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 7];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 6];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 5];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 4];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 3];\
	KERNEL_ALIGN(alignment) state25 state25s[1<< 2];\
	KERNEL_ALIGN(alignment) state26 state26s[1<< 1];

#define STATE_MEMBERS_28(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<27];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<26];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<25];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<24];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<23];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<22];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<21];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<20];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<19];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<18];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<17];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<16];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<15];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<14];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<13];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<12];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<11];\
	KERNEL_ALIGN(alignment) state18 state18s[1<<10];\
	KERNEL_ALIGN(alignment) state19 state19s[1<< 9];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 8];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 7];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 6];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 5];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 4];\
	KERNEL_ALIGN(alignment) state25 state25s[1<< 3];\
	KERNEL_ALIGN(alignment) state26 state26s[1<< 2];\
	KERNEL_ALIGN(alignment) state27 state27s[1<< 1];

#define STATE_MEMBERS_29(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<28];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<27];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<26];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<25];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<24];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<23];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<22];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<21];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<20];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<19];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<18];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<17];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<16];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<15];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<14];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<13];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<12];\
	KERNEL_ALIGN(alignment) state18 state18s[1<<11];\
	KERNEL_ALIGN(alignment) state19 state19s[1<<10];\
	KERNEL_ALIGN(alignment) state20 state20s[1<< 9];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 8];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 7];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 6];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 5];\
	KERNEL_ALIGN(alignment) state25 state25s[1<< 4];\
	KERNEL_ALIGN(alignment) state26 state26s[1<< 3];\
	KERNEL_ALIGN(alignment) state27 state27s[1<< 2];\
	KERNEL_ALIGN(alignment) state28 state28s[1<< 1];

#define STATE_MEMBERS_30(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<29];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<28];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<27];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<26];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<25];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<24];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<23];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<22];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<21];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<20];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<19];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<18];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<17];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<16];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<15];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<14];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<13];\
	KERNEL_ALIGN(alignment) state18 state18s[1<<12];\
	KERNEL_ALIGN(alignment) state19 state19s[1<<11];\
	KERNEL_ALIGN(alignment) state20 state20s[1<<10];\
	KERNEL_ALIGN(alignment) state21 state21s[1<< 9];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 8];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 7];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 6];\
	KERNEL_ALIGN(alignment) state25 state25s[1<< 5];\
	KERNEL_ALIGN(alignment) state26 state26s[1<< 4];\
	KERNEL_ALIGN(alignment) state27 state27s[1<< 3];\
	KERNEL_ALIGN(alignment) state28 state28s[1<< 2];\
	KERNEL_ALIGN(alignment) state29 state29s[1<< 1];

#define STATE_MEMBERS_31(alignment)\
	KERNEL_ALIGN(alignment) state1 state1s  [1<<30];\
	KERNEL_ALIGN(alignment) state2 state2s  [1<<29];\
	KERNEL_ALIGN(alignment) state3 state3s  [1<<28];\
	KERNEL_ALIGN(alignment) state4 state4s  [1<<27];\
	KERNEL_ALIGN(alignment) state5 state5s  [1<<26];\
	KERNEL_ALIGN(alignment) state6 state6s  [1<<25];\
	KERNEL_ALIGN(alignment) state7 state7s  [1<<24];\
	KERNEL_ALIGN(alignment) state8 state8s  [1<<23];\
	KERNEL_ALIGN(alignment) state9 state9s  [1<<22];\
	KERNEL_ALIGN(alignment) state10 state10s[1<<21];\
	KERNEL_ALIGN(alignment) state11 state11s[1<<20];\
	KERNEL_ALIGN(alignment) state12 state12s[1<<19];\
	KERNEL_ALIGN(alignment) state13 state13s[1<<18];\
	KERNEL_ALIGN(alignment) state14 state14s[1<<17];\
	KERNEL_ALIGN(alignment) state15 state15s[1<<16];\
	KERNEL_ALIGN(alignment) state16 state16s[1<<15];\
	KERNEL_ALIGN(alignment) state17 state17s[1<<14];\
	KERNEL_ALIGN(alignment) state18 state18s[1<<13];\
	KERNEL_ALIGN(alignment) state19 state19s[1<<12];\
	KERNEL_ALIGN(alignment) state20 state20s[1<<11];\
	KERNEL_ALIGN(alignment) state21 state21s[1<<10];\
	KERNEL_ALIGN(alignment) state22 state22s[1<< 9];\
	KERNEL_ALIGN(alignment) state23 state23s[1<< 8];\
	KERNEL_ALIGN(alignment) state24 state24s[1<< 7];\
	KERNEL_ALIGN(alignment) state25 state25s[1<< 6];\
	KERNEL_ALIGN(alignment) state26 state26s[1<< 5];\
	KERNEL_ALIGN(alignment) state27 state27s[1<< 4];\
	KERNEL_ALIGN(alignment) state28 state28s[1<< 3];\
	KERNEL_ALIGN(alignment) state29 state29s[1<< 2];\
	KERNEL_ALIGN(alignment) state30 state30s[1<< 1];
typedef uint8_t BYTE;
#define KERNELB_NO_OP(n, alignment)\
typedef union{\
  KERNEL_ALIGN(alignment) BYTE state[(size_t)1<<(n-1)];\
  STATE_MEMBERS(n, alignment);\
} state##n;\
typedef state##n (* kernelb##n )( state##n);\
typedef void (* kernelpb##n )( state##n*);\
static state##n state##n##_zero() {state##n a = {0}; return a;}\
static state##n mem_to_state##n(void* p){state##n a; memcpy(a.state, p, 1<<(n-1)); return a;}\
static void mem_to_statep##n(void* p, state##n *a){memcpy(a->state, p, 1<<(n-1));}\
/*inline kernelpb call*/\
static inline state##n ikpb##n(state##n s, kernelpb##n func){\
	func(&s);\
	return s;\
}

#define KERNELB(n, alignment)\
KERNELB_NO_OP(n, alignment)\
/*perform the operation between the two halves and return it*/\
static void k_and##n (state##n *a){\
	PRAGMA_SIMD\
	for(long long int i = 0; i < (1<<(n-1))/2; i++)\
		a->state[i] = a->state[i] & a->state[i + (1<<(n-2))];\
}\
static void k_or##n (state##n *a){\
	PRAGMA_SIMD\
	for(long long int i = 0; i < (1<<(n-1))/2; i++)\
		a->state[i] = a->state[i] | a->state[i + (1<<(n-2))];\
}\
static void k_xor##n (state##n *a){\
	PRAGMA_SIMD\
	for(long long int i = 0; i < (1<<(n-1))/2; i++)\
		a->state[i] = a->state[i] ^ a->state[i + (1<<(n-2))];\
}\
static void k_byteswap##n (state##n *a){\
	PRAGMA_SIMD\
	for(long long int i = 0; i < (1<<(n-1))/2; i++){\
		uint8_t c = a->state[i];\
		a->state[i] = a->state[(1<<(n-1))-1-i];\
		a->state[(1<<(n-1))-1-i] = c;\
	}\
}\
static void k_endian_cond_byteswap##n (state##n *a){\
	static const int i = 1;\
	if(*((char*)&i))\
		k_byteswap##n(a);\
}

//Define functions which need to know nn and nm.
#define KERNELCONV(nn, nm)\
/*Retrieve the highest precision bits*/\
static state##nn state_high##nm(state##nm a){\
	return a.state##nn##s[0];\
}\
static void state_highp##nm(state##nm *a, state##nn *ret){\
	*ret = a->state##nn##s[0];\
}\
/*Retrieve the lowest precision bits*/\
static state##nn state_low##nm(state##nm a){\
	return a.state##nn##s[1];\
}\
static void state_lowp##nm(state##nm *a, state##nn *ret){\
	*ret = a->state##nn##s[1];\
}\
static state##nn* state_pointer_low##nm(state##nm *a){\
	return a->state##nn##s + 1;\
}\
static state##nn* state_pointer_high##nm(state##nm *a){\
	return a->state##nn##s + 0;\
}\
/*One of the most important functions- Reduce state by half with arbitrary division.*/\
static void state_reducep##nm(state##nm *a, state##nn *ret, size_t byteoffset){\
	memcpy(ret->state, a->state+byteoffset, 1<<(nn-1));\
}\
/*Kernels*/\
/*Swap the upper and lower halves.*/\
static void k_swap##nm(state##nm *a){\
	PRAGMA_SIMD\
	for(long long int i = 0; i < 1<<(nn-1); i++){\
		uint8_t c = a->state##nn##s[0].state[i];\
		a->state##nn##s[0].state[i] = a->state##nn##s[1].state[i];\
		a->state##nn##s[1].state[i] = c;\
	}\
}



#define KERNEL_FOREACH(func, arr, nn, nm)\
for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	arr.state##nn##s[i] = func(arr.state##nn##s[i]);

#define KERNEL_FOREACHP(func, arr, nn, nm)\
for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	func(arr.state##nn##s +i);

#define KERNEL_CHAINP(name, func1, func2, n)\
static void name(state##n *c){\
	func1(c);\
	func2(c);\
}

#define KERNEL_CHAIN(name, func1, func2, n)\
static void name(state##n *c){\
	*c = func1(*c);\
	*c = func2(*c);\
}

#define KERNEL_MULTIPLEX_CALLP(iscopy, func, nn) KERNEL_MULTIPLEX_CALLP_##iscopy(func, nn)
#define KERNEL_MULTIPLEX_CALLP_1(func, nn) a->state##nn##s[i] = func(a->state##nn##s[i]);
#define KERNEL_MULTIPLEX_CALLP_0(func, nn) func(a->state##nn##s + i);

//Multiplex a low level kernel to a higher level.
//The most basic implementation, with parallelism.
//The last argument specifies what type of kernel it is-
//if it is a type1 or type 2 kernel.
//These macros ALWAYS produce pointer kernels, they produce better bytecode.
#define KERNEL_MULTIPLEX(name, func, nn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = (1<<(nm-1)) / (1<<(nn-1));\
	PRAGMA_PARALLEL\
	for(long long int i = 0; i < end; i++)\
		KERNEL_MULTIPLEX_CALLP(iscopy, func, nn);\
}

#define KERNEL_MULTIPLEX_SIMD(name, func, nn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = (1<<(nm-1)) / (1<<(nn-1));\
	PRAGMA_SIMD\
	for(long long int i = 0; i < end; i++)\
		KERNEL_MULTIPLEX_CALLP(iscopy, func, nn);\
}

#define KERNEL_MULTIPLEX_NP(name, func, nn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = (1<<(nm-1)) / (1<<(nn-1));\
	for(long long int i = 0; i < end; i++)\
		KERNEL_MULTIPLEX_CALLP(iscopy, func, nn);\
}

//pointer version
#define KERNEL_MULTIPLEX_ICALLP(iscopy, func) KERNEL_MULTIPLEX_ICALLP_##iscopy(func)
#define KERNEL_MULTIPLEX_ICALLP_1(func) current_indexed = func(current_indexed);
#define KERNEL_MULTIPLEX_ICALLP_0(func) func(&current_indexed);

//Multiplex a low level kernel to a higher level, with index in the upper half.
//Your kernel must operate on statennn but the input array will be treated as statenn's
#define KERNEL_MULTIPLEX_INDEXED(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	state##nn current, index; \
	state##nnn current_indexed;\
	PRAGMA_PARALLEL\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	{\
		uint32_t ind32 = i; uint16_t ind16 = i; uint8_t ind8 = i;\
		current = a->state##nn##s[i];\
		if(nn == 1)/*Single byte indices.*/\
			memcpy(index.state, &ind8, 1);\
		else if (nn == 2)/*Two byte indices*/\
			memcpy(index.state, &ind16, 2);\
		else if (nn == 3)/*Three byte indices*/\
			memcpy(index.state, &ind32, 4);\
		else	/*We must copy the 32 bit index into the upper half.*/\
			memcpy(index.state, &ind32, 4);\
		/*We have the current and the index, combine them.*/\
		current_indexed.state##nn##s[0] = index;\
		current_indexed.state##nn##s[1] = current;\
		KERNEL_MULTIPLEX_ICALLP(iscopy, func);\
		/*Run the function on the indexed thing and return the low */\
		current = current_indexed.state##nn##s[1];\
		memcpy(a->state + i*(1<<(nn-1)), current.state, (1<<(nn-1)) );\
	}\
}


#define KERNEL_MULTIPLEX_INDEXED_NP(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	state##nn current, index; \
	state##nnn current_indexed;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	{\
		uint32_t ind32 = i; uint16_t ind16 = i; uint8_t ind8 = i;\
		current = a->state##nn##s[i];\
		if(nn == 1)/*Single byte indices.*/\
			memcpy(index.state, &ind8, 1);\
		else if (nn == 2)/*Two byte indices*/\
			memcpy(index.state, &ind16, 2);\
		else if (nn == 3)/*Three byte indices*/\
			memcpy(index.state, &ind32, 4);\
		else	/*We must copy the 32 bit index into the upper half.*/\
			memcpy(index.state, &ind32, 4);\
		/*We have the current and the index, combine them.*/\
		current_indexed.state##nn##s[0] = index;\
		current_indexed.state##nn##s[1] = current;\
		KERNEL_MULTIPLEX_ICALLP(iscopy, func);\
		/*Run the function on the indexed thing and return the low */\
		current = current_indexed.state##nn##s[1];\
		memcpy(a->state + i*(1<<(nn-1)), current.state, (1<<(nn-1)) );\
	}\
}

#define KERNEL_SHUFFLE_CALL(func, iscopy) KERNEL_SHUFFLE_CALL_##iscopy (func)
#define KERNEL_SHUFFLE_CALL_1(func) index = func(index);
#define KERNEL_SHUFFLE_CALL_0(func) func(&index);

#define KERNEL_SHUFFLE_IND32(name, func, nn, nm, iscopy)\
static void name(state##nm* a){\
	state##nm ret;\
	state3 index; \
	memcpy(&ret, a, sizeof(state##nm));\
	static const size_t emplacemask = (1<<(nm-1)) / (1<<(nn-1)) - 1;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	{	\
		index=to_state3(i);\
		KERNEL_SHUFFLE_CALL(func, iscopy);\
		a->state##nn##s[from_state3(index) & emplacemask] = \
		ret.state##nn##s[i];\
	}\
}

#define KERNEL_SHUFFLE_IND16(name, func, nn, nm, iscopy)\
static void name(state##nm* a){\
	state##nm ret;\
	state2 index; \
	memcpy(&ret, a, sizeof(state##nm));\
	static const size_t emplacemask = (1<<(nm-1)) / (1<<(nn-1)) - 1;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	{	\
		index=to_state2(i);\
		KERNEL_SHUFFLE_CALL(func, iscopy);\
		a->state##nn##s[from_state2(index) & emplacemask] = \
		ret.state##nn##s[i];\
	}\
}

#define KERNEL_SHUFFLE_IND8(name, func, nn, nm, iscopy)\
static void name(state##nm* a){\
	state##nm ret;\
	state1 index; \
	memcpy(&ret, a, sizeof(state##nm));\
	static const size_t emplacemask = (1<<(nm-1)) / (1<<(nn-1)) - 1;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	{	\
		index=to_state1(i);\
		KERNEL_SHUFFLE_CALL(func, iscopy);\
		a->state##nn##s[from_state1(index) & emplacemask] = \
		ret.state##nn##s[i];\
	}\
}


#define KERNEL_MULTIPLEX_INDEXED_EMPLACE(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm* a){\
	state##nm ret;\
	state##nn current, index; \
	state##nnn current_indexed;\
	memcpy(&ret, a, sizeof(state##nm));\
	static const size_t emplacemask = (1<<(nm-1)) / (1<<(nn-1)) - 1;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
	{	\
		uint32_t ind32 = i; uint16_t ind16 = i; uint8_t ind8 = i;\
		current = a->state##nn##s[i];\
		if(nn == 1)/*Single byte indices.*/\
			index = mem_to_state##nn(&ind8);\
		else if (nn == 2)/*Two byte indices*/\
			index = mem_to_state##nn(&ind16);\
		else if (nn == 3)/*Three byte indices*/\
			index = mem_to_state##nn(&ind32);\
		else	/*We must copy the 32 bit index into the upper half.*/\
			memcpy(index.state, &ind32, 4);\
		/*We have the current and the index, combine them.*/\
		current_indexed.state##nn##s[0] = index;\
		current_indexed.state##nn##s[1] = current;\
		/*Run the function on the indexed thing and return the low */\
		KERNEL_MULTIPLEX_ICALLP(iscopy, func);\
		index = current_indexed.state##nn##s[0];\
		current = current_indexed.state##nn##s[1];\
		if(nn == 1){/*Single byte indices.*/\
			memcpy(&ind8, index.state, 1);\
			ind8 &= emplacemask;\
			memcpy(ret.state + ind8*(1<<(nn-1)), current.state, (1<<(nn-1)) );\
		}else if (nn == 2){/*Two byte indices*/\
			memcpy(&ind16, index.state, 2);\
			ind16 &= emplacemask;\
			memcpy(ret.state + ind16*(1<<(nn-1)), current.state, (1<<(nn-1)) );\
		}else if (nn == 3){/*Three byte indices*/\
			memcpy(&ind32, index.state, 4);\
			ind32 &= emplacemask;\
			memcpy(ret.state + ind32*(1<<(nn-1)), current.state, (1<<(nn-1)) );\
		}else{	/*We must copy the 32 bit index into the upper half.*/\
			memcpy(&ind32, index.state, 4);\
			ind32 &= emplacemask;\
			memcpy(ret.state + ind32*(1<<(nn-1)), current.state, (1<<(nn-1)) );\
		}\
	}\
	memcpy(a, &ret, sizeof(state##nm));\
}

//The shared state function.
//func must take in nnn of state.
//the very first statenn within the statenm is considered "shared"
//every single nn thereafter is looped over.
//The shared state and the i'th statenn is merged into a single statennn
//which is passed to your function.
//nnn must be nn + 1
//The shared state is presumed to be very large, so this is all done with pointers and heap memory.
//All that said, you *can* pass a copy-kernel.
#define KERNEL_SHARED_CALL(iscopy, func) KERNEL_SHARED_CALL_##iscopy(func)
#define KERNEL_SHARED_CALL_1(func) passed = func(passed);
#define KERNEL_SHARED_CALL_0(func) func(&passed);

//TODO
#define KERNEL_SHARED_STATE(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	state##nnn passed;\
	/*memcpy(passed->state, a->state, sizeof(state##nn));*/\
	passed.state##nn##s[0] = a->state##nn##s[0];\
	/*i = 1 because the 0'th element is shared.*/\
	for(long long int i = 1; i < (1<<(nm-1)) / (1<<(nn-1)); i++){\
		passed.state##nn##s[1] = a->state##nn##s[i];\
		KERNEL_SHARED_CALL(iscopy, func)\
		a->state##nn##s[i] = passed.state##nn##s[1];\
	}\
	/*Copy the shared state back.*/\
	memcpy(a->state, passed.state, sizeof(state##nn));\
}\

//Variant in which the shared state is "read only"
#define KERNEL_RO_SHARED_STATE(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = (1<<(nm-1)) / (1<<(nn-1));\
	PRAGMA_PARALLEL\
	for(long long int i = 1; i < end; i++){\
		state##nnn passed;\
		passed.state##nn##s[0] = a->state##nn##s[0];\
		passed.state##nn##s[1] = a->state##nn##s[i];\
		KERNEL_SHARED_CALL(iscopy, func)\
		a->state##nn##s[i] = passed.state##nn##s[1];\
	}\
}\

//Variant in which the shared state is "read only", no parallelism
#define KERNEL_RO_SHARED_STATE_NP(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = (1<<(nm-1)) / (1<<(nn-1));\
	for(long long int i = 1; i < end; i++){\
		state##nnn passed;\
		passed.state##nn##s[0] = a->state##nn##s[0];\
		passed.state##nn##s[1] = a->state##nn##s[i];\
		KERNEL_SHARED_CALL(iscopy, func)\
		a->state##nn##s[i] = passed.state##nn##s[1];\
	}\
}\

//Variant in which the shared state is "read only"
#define KERNEL_RO_SHARED_STATE_SIMD(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = (1<<(nm-1)) / (1<<(nn-1));\
	PRAGMA_SIMD\
	for(long long int i = 1; i < end; i++){\
		state##nnn passed;\
		passed.state##nn##s[0] = a->state##nn##s[0];\
		passed.state##nn##s[1] = a->state##nn##s[i];\
		KERNEL_SHARED_CALL(iscopy, func)\
		a->state##nn##s[i] = passed.state##nn##s[1];\
	}\
}

#define KERNEL_MHALVES_CALLP(iscopy, func) KERNEL_MHALVES_CALLP_##iscopy(func)
#define KERNEL_MHALVES_CALLP_1(func) passed = func(passed);
#define KERNEL_MHALVES_CALLP_0(func) func(&passed);


#define KERNEL_MHALVES_CALL(iscopy, func) KERNEL_MHALVES_CALL_##iscopy(func)
#define KERNEL_MHALVES_CALL_1(func) passed = func(passed);
#define KERNEL_MHALVES_CALL_0(func) func(&passed);
//Multiplex on halves.

#define KERNEL_MULTIPLEX_HALVES(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = ((1<<(nm-1))/(1<<(nn-1))) /2;\
	PRAGMA_PARALLEL\
	for(long long int i = 0; i < end; i++){\
		state##nnn passed;\
		passed.state##nn##s[0] = state_pointer_high##nm(a)->state##nn##s[i];\
		passed.state##nn##s[1] = state_pointer_low##nm(a)->state##nn##s[i];\
		KERNEL_MHALVES_CALLP(iscopy, func)\
		state_pointer_high##nm(a)->state##nn##s[i] = passed.state##nn##s[0];\
		state_pointer_low##nm(a)->state##nn##s[i] = passed.state##nn##s[1];\
	}\
}

#define KERNEL_MULTIPLEX_HALVES_SIMD(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = ((1<<(nm-1))/(1<<(nn-1))) /2;\
	PRAGMA_SIMD\
	for(long long int i = 0; i < end; i++){\
		state##nnn passed;\
		passed.state##nn##s[0] = state_pointer_high##nm(a)->state##nn##s[i];\
		passed.state##nn##s[1] = state_pointer_low##nm(a)->state##nn##s[i];\
		KERNEL_MHALVES_CALLP(iscopy, func)\
		state_pointer_high##nm(a)->state##nn##s[i] = passed.state##nn##s[0];\
		state_pointer_low##nm(a)->state##nn##s[i] = passed.state##nn##s[1];\
	}\
}

#define KERNEL_MULTIPLEX_HALVES_NP(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	static const long long int end = ((1<<(nm-1))/(1<<(nn-1))) /2;\
	for(long long int i = 0; i < end; i++){\
		state##nnn passed;\
		passed.state##nn##s[0] = state_pointer_high##nm(a)->state##nn##s[i];\
		passed.state##nn##s[1] = state_pointer_low##nm(a)->state##nn##s[i];\
		KERNEL_MHALVES_CALLP(iscopy, func)\
		state_pointer_high##nm(a)->state##nn##s[i] = passed.state##nn##s[0];\
		state_pointer_low##nm(a)->state##nn##s[i] = passed.state##nn##s[1];\
	}\
}


#define KERNEL_MULTIKERNEL_CALL(iscopy, funcarr, nn) KERNEL_MULTIKERNEL_CALL_##iscopy(funcarr, nn)
#define KERNEL_MULTIKERNEL_CALL_1(funcarr, nn) a->state##nn##s[i] = (funcarr[i])(a->state##nn##s[i]);
#define KERNEL_MULTIKERNEL_CALL_0(funcarr, nn) (funcarr[i])(a->state##nn##s +i);
//Create a multiplexed kernel which taks in an array of function pointers
//
#define KERNEL_MULTIPLEX_MULTIKERNEL(name, funcarr, nn, nm, iscopy)\
static void name(state##nm *a){\
	PRAGMA_PARALLEL\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
		KERNEL_MULTIKERNEL_CALL(iscopy, funcarr, nn);\
}

#define KERNEL_MULTIPLEX_MULTIKERNEL_NP(name, funcarr, nn, nm, iscopy)\
static void name(state##nm *a){\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
		KERNEL_MULTIKERNEL_CALL(iscopy, funcarr, nn);\
}

#define KERNEL_MULTIPLEX_MULTIKERNEL_SIMD(name, funcarr, nn, nm, iscopy)\
static void name(state##nm *a){\
	PRAGMA_SIMD\
	for(long long int i = 1; i < (1<<(nm-1)) / (1<<(nn-1)); i++)\
		KERNEL_MULTIKERNEL_CALL(iscopy, funcarr);\
}

#define KERNEL_MULTIPLEX_NLOGN_CALLP(func, iscopy) KERNEL_MULTIPLEX_NLOGN_CALLP_##iscopy(func)
#define KERNEL_MULTIPLEX_NLOGN_CALLP_1(func) current_b = func(current_b);
#define KERNEL_MULTIPLEX_NLOGN_CALLP_0(func) func(&current_b);

//Read-only i'th element, non-parallel.
#define KERNEL_MULTIPLEX_NP_NLOGNRO(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	state##nnn current_b;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)) - 1; i++){\
		current_b.state##nn##s[0] = a->state##nn##s[i];\
		for(long long int j = i+1; j < (1<<(nm-1)) / (1<<(nn-1)); j++)\
		{\
			current_b.state##nn##s[1] = a->state##nn##s[j];\
			KERNEL_MULTIPLEX_NLOGN_CALLP(func, iscopy)\
			a->state##nn##s[j] = current_b.state##nn##s[1];\
		}\
	}\
}

//NLOGN but parallel, the i element is considered "read only"
//This is useful in situations where you want NLOGN functionality, but you dont want to modify i element.
#define KERNEL_MULTIPLEX_NLOGNRO(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)) - 1; i++){\
		PRAGMA_PARALLEL\
		for(long long int j = i+1; j < (1<<(nm-1)) / (1<<(nn-1)); j++)\
		{\
			state##nnn current_b;\
			current_b.state##nn##s[0] = a->state##nn##s[i];\
			current_b.state##nn##s[1] = a->state##nn##s[j];\
			KERNEL_MULTIPLEX_NLOGN_CALLP(func, iscopy)\
			a->state##nn##s[j] = current_b.state##nn##s[1];\
		}\
	}\
}


#define KERNEL_MULTIPLEX_NLOGN(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	state##nnn current_b;\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)) - 1; i++){\
		current_b.state##nn##s[0] = a->state##nn##s[i];\
		for(long long int j = i+1; j < (1<<(nm-1)) / (1<<(nn-1)); j++)\
		{\
			current_b.state##nn##s[1] = a->state##nn##s[j];\
			KERNEL_MULTIPLEX_NLOGN_CALLP(func, iscopy)\
			a->state##nn##s[j] = current_b.state##nn##s[1];\
		}\
		/*Write back elem i*/\
		a->state##nn##s[i] = current_b.state##nn##s[0];\
	}\
}

//NLOGN but parallel, the i element is considered "read only"
//This is useful in situations where you want NLOGN functionality, but you dont want to modify i element.
//Simd variant.
#define KERNEL_MULTIPLEX_SIMD_NLOGNRO(name, func, nn, nnn, nm, iscopy)\
static void name(state##nm *a){\
	for(long long int i = 0; i < (1<<(nm-1)) / (1<<(nn-1)) - 1; i++){\
		PRAGMA_SIMD\
		for(long long int j = i+1; j < (1<<(nm-1)) / (1<<(nn-1)); j++)\
		{\
			state##nnn current_b;\
			current_b.state##nn##s[0] = a->state##nn##s[i];\
			current_b.state##nn##s[1] = a->state##nn##s[j];\
			KERNEL_MULTIPLEX_NLOGN_CALLP(func, iscopy)\
			a->state##nn##s[j] = current_b.state##nn##s[1];\
		}\
	}\
}


#define KERNEL_COMPLETE_ARITHMETIC(n, nn, bb)\
static void k_shl_s##n(state##nn *q){\
	uint##bb##_t a = from_state##n(q->state##n##s[0]);\
	uint##bb##_t b = from_state##n(q->state##n##s[1]);\
	b &= bb - 1;\
	a <<= b;\
	q->state##n##s[0] = to_state##n(a);\
}\
static void k_shr_s##n(state##nn *q){\
	uint##bb##_t a = from_state##n(q->state##n##s[0]);\
	uint##bb##_t b = from_state##n(q->state##n##s[1]);\
	b &= bb - 1;\
	a >>= b;\
	q->state##n##s[0] = to_state##n(a);\
}\
static void k_and_s##n(state##nn *q){\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) & from_state##n(q->state##n##s[1]) );\
}\
static void k_or_s##n(state##nn *q){\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) | from_state##n(q->state##n##s[1]) );\
}\
static void k_xor_s##n(state##nn *q){\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) ^ from_state##n(q->state##n##s[1]) );\
}\
static void k_add_s##n(state##nn *q){\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) + from_state##n(q->state##n##s[1]) );\
}\
static void k_sub_s##n(state##nn *q){\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) - from_state##n(q->state##n##s[1]) );\
}\
static void k_mul_s##n(state##nn *q){\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) * from_state##n(q->state##n##s[1]) );\
}\
static void k_div_s##n(state##nn *q){\
	if(from_state##n(q->state##n##s[1]) == 0) q->state##n##s[0] = to_state##n(0);\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) / from_state##n(q->state##n##s[1]) );\
}\
static void k_mod_s##n(state##nn *q){\
	if(from_state##n(q->state##n##s[1]) == 0) q->state##n##s[0] = signed_to_state##n(0);\
	q->state##n##s[0] = to_state##n( from_state##n(q->state##n##s[0]) % from_state##n(q->state##n##s[1]) );\
}\
static void k_sneg##n(state##n *q){\
	*q = signed_to_state##n( -1 * signed_from_state##n(*q));\
}\
static void k_abs##n(state##n *q){\
	*q = signed_to_state##n( labs(signed_from_state##n(*q)) );\
}\
static void k_neg##n(state##n *q){\
	*q = to_state##n( ~(from_state##n(*q)) );\
}\
static void k_incr##n(state##n *q){\
	*q = to_state##n( (from_state##n(*q))+1 );\
}\
static void k_decr##n(state##n *q){\
	*q = to_state##n( (from_state##n(*q))-1 );\
}\
static void k_sadd_s##n(state##nn *q){\
	k_add_s##n(q);\
}\
static void k_ssub_s##n(state##nn *q){\
	q->state##n##s[1] = signed_to_state##n(\
		-1*signed_from_state##n(q->state##n##s[1])\
	);\
	k_sadd_s##n(q);\
}\
static void k_smul_s##n(state##nn *q){\
	int##bb##_t test = ((signed_from_state##n(q->state##n##s[0])<0) != (signed_from_state##n(q->state##n##s[1])<0));\
	q->state##n##s[0] = signed_to_state##n(labs(signed_from_state##n(q->state##n##s[0])));\
	q->state##n##s[1] = signed_to_state##n(labs(signed_from_state##n(q->state##n##s[1])));\
	k_mul_s##n(q);\
	if(test)\
		k_sneg##n(q->state##n##s);\
}\
static void k_sdiv_s##n(state##nn *q){\
	int##bb##_t test = ((signed_from_state##n(q->state##n##s[0])<0) != (signed_from_state##n(q->state##n##s[1])<0));\
	q->state##n##s[0] = signed_to_state##n(labs(signed_from_state##n(q->state##n##s[0])));\
	q->state##n##s[1] = signed_to_state##n(labs(signed_from_state##n(q->state##n##s[1])));\
	k_div_s##n(q);\
	if(test)\
		k_sneg##n(q->state##n##s);\
}\
static void k_smod_s##n(state##nn *q){\
	int##bb##_t test = ((signed_from_state##n(q->state##n##s[0])<0) != (signed_from_state##n(q->state##n##s[1])<0));\
	q->state##n##s[0] = signed_to_state##n(labs(signed_from_state##n(q->state##n##s[0])));\
	q->state##n##s[1] = signed_to_state##n(labs(signed_from_state##n(q->state##n##s[1])));\
	k_mod_s##n(q);\
	if(test)\
		k_sneg##n(q->state##n##s);\
}

#ifndef KERNEL_FAST_FLOAT_MATH
//Safe by default.
#define KERNEL_FAST_FLOAT_MATH 0
#endif

#define KERNEL_COMPLETE_FLOATING_ARITHMETIC(n, nn, type)\
static void k_fadd_s##n(state##nn *q){\
	type a = type##_from_state##n(q->state##n##s[0]);\
	type b = type##_from_state##n(q->state##n##s[1]);\
	if(KERNEL_FAST_FLOAT_MATH || (isfinite(a) && isfinite(b)))\
		q->state##n##s[0] = type##_to_state##n(a+b);\
	else\
		q->state##n##s[0] = type##_to_state##n(0);\
}\
static void k_fsub_s##n(state##nn *q){\
	type a = type##_from_state##n(q->state##n##s[0]);\
	type b = type##_from_state##n(q->state##n##s[1]);\
	if(KERNEL_FAST_FLOAT_MATH || (isfinite(a) && isfinite(b)))\
		q->state##n##s[0] = type##_to_state##n(a-b);\
	else\
		q->state##n##s[0] = type##_to_state##n(0);\
}\
static void k_fmul_s##n(state##nn *q){\
	type a = type##_from_state##n(q->state##n##s[0]);\
	type b = type##_from_state##n(q->state##n##s[1]);\
	if(KERNEL_FAST_FLOAT_MATH || (isfinite(a) && isfinite(b)))\
		q->state##n##s[0] = type##_to_state##n(a*b);\
	else\
		q->state##n##s[0] = type##_to_state##n(0);\
}\
static void k_fdiv_s##n(state##nn *q){\
	type a = type##_from_state##n(q->state##n##s[0]);\
	type b = type##_from_state##n(q->state##n##s[1]);\
	if(KERNEL_FAST_FLOAT_MATH || (isfinite(a) && isnormal(b)))\
		q->state##n##s[0] = type##_to_state##n(a/b);\
	else\
		q->state##n##s[0] = type##_to_state##n(0);\
}\
static void k_fmod_s##n(state##nn *q){\
	type a = type##_from_state##n(q->state##n##s[0]);\
	type b = type##_from_state##n(q->state##n##s[1]);\
	if(KERNEL_FAST_FLOAT_MATH || (isfinite(a) && isnormal(b)))\
		q->state##n##s[0] = type##_to_state##n(fmod(a,b));\
	else\
		q->state##n##s[0] = type##_to_state##n(0);\
}\
static void k_fceil_s##n(state##n *q){\
	type a = type##_from_state##n(*q);\
	if(KERNEL_FAST_FLOAT_MATH || isfinite(a))\
		*q = type##_to_state##n(ceil(a));\
	else\
		*q = type##_to_state##n(0);\
}\
static void k_ffloor_s##n(state##n *q){\
	type a = type##_from_state##n(*q);\
	if(KERNEL_FAST_FLOAT_MATH || isfinite(a))\
		*q = type##_to_state##n(floor(a));\
	else\
		*q = type##_to_state##n(0);\
}\
static void k_fabs_s##n(state##n *q){\
	type a = type##_from_state##n(*q);\
	if(KERNEL_FAST_FLOAT_MATH || isfinite(a))\
		*q = type##_to_state##n(fabsl(a));\
	else\
		*q = type##_to_state##n(0);\
}\
static void k_fsqrt_s##n(state##n *q){\
	type a = type##_from_state##n(*q);\
	if(KERNEL_FAST_FLOAT_MATH || isfinite(a))\
		*q = type##_to_state##n(sqrt(a));\
	else\
		*q = type##_to_state##n(0);\
}\
static void k_fsqr_s##n(state##n *q){\
	state##nn p;\
	p.state##n##s[0] = *q;\
	p.state##n##s[1] = *q;\
	k_fmul_s##n(&p);\
	*q = p.state##n##s[0];\
}\
static void k_fneg_s##n(state##n *q){\
	state##nn p;\
	p.state##n##s[0] = *q;\
	p.state##n##s[1] = type##_to_state##n(-1);\
	k_fmul_s##n(&p);\
	*q = p.state##n##s[0];\
}

//There is no relevant op for 1.
KERNELB_NO_OP(1,1);
//helper function.
static state1 to_state1(uint8_t a){
	state1 q;
	memcpy(q.state, &a,1);
	return q;
}
static state1 signed_to_state1(int8_t a){
	state1 q;
	memcpy(q.state, &a,1);
	return q;
}
static uint8_t from_state1(state1 q){
	return q.state[0];
}
static int8_t signed_from_state1(state1 q){
	int8_t ret; 
	memcpy(&ret, q.state, 1);
	return ret;
}
//state2. Contains 2^(2-1) bytes, or 2 bytes.
KERNELB(2,2);
//Conversion function to up from 1 byte to 2 bytes.
KERNELCONV(1,2);
static state2 to_state2(uint16_t a){
	state2 q;
	memcpy(q.state, &a,2);
	return q;
}
static uint16_t from_state2(state2 q){
	uint16_t a;
	memcpy(&a, q.state, 2);
	return a;
}
static state2 signed_to_state2(int16_t a){
	state2 q;
	memcpy(q.state, &a, 2);
	return q;
}
static int16_t signed_from_state2(state2 q){
	int16_t a;
	memcpy(&a, q.state, 2);
	return a;
}
KERNEL_COMPLETE_ARITHMETIC(1,2, 8)

//state3. contains 4 bytes- so, most of your typical types go here.
KERNELB(3,4);
KERNELCONV(2,3);
static state3 to_state3(uint32_t a){
	state3 q;
	memcpy(q.state, &a, 4);
	return q;
}
static uint32_t from_state3(state3 q){
	uint32_t a;
	memcpy(&a, q.state, 4);
	return a;
}


static state3 signed_to_state3(int32_t a){
	state3 q;
	memcpy(q.state, &a, 4);
	return q;
}
static int32_t signed_from_state3(state3 q){
	int32_t a;
	memcpy(&a, q.state, 4);
	return a;
}
static state3 float_to_state3(float a){
	state3 q;
	memcpy(q.state, &a, 4);
	return q;
}
static float float_from_state3(state3 q){
	float a;
	memcpy(&a, q.state, 4);
	return a;
}
KERNEL_COMPLETE_ARITHMETIC(2,3, 16)

//Fast Inverse Square Root.
static void k_fisr(state3 *xx){
	int32_t x = from_state3(*xx);
	int32_t i; 
	float x2;
	memcpy(&i, xx->state, 4);
	i = 0x5F1FFFF9 - (i>>1);
	memcpy(&x2, &i, 4);
	x2 *= 0.703952253f * (2.38924456f - x * x2 * x2);
	memcpy(xx->state, &x2, 4);
}

KERNELB(4,8);
KERNELCONV(3,4);
//The to and from functions can't be used unless we have uint64_t
#ifdef UINT64_MAX
static state4 to_state4(uint64_t a){
	state4 q;
	memcpy(q.state, &a, 8);
	return q;
}
static uint64_t from_state4(state4 q){
	uint64_t a;
	memcpy(&a, q.state, 8);
	return a;
}

static state4 signed_to_state4(int64_t a){
	state4 q;
	memcpy(q.state, &a, 8);
	return q;
}
static int64_t signed_from_state4(state4 q){
	int64_t a;
	memcpy(&a, q.state, 8);
	return a;
}

static state4 double_to_state4(double a){
	state4 q;
	memcpy(q.state, &a, 8);
	return q;
}
static double double_from_state4(state4 q){
	double a;
	memcpy(&a, q.state, 8);
	return a;
}
#endif


KERNEL_COMPLETE_ARITHMETIC(3,4, 32)
KERNEL_COMPLETE_FLOATING_ARITHMETIC(3, 4, float)


//larger kernels.
//Enough for a vec4
KERNELB(5,16);
KERNELCONV(4,5);
#ifdef UINT64_MAX
KERNEL_COMPLETE_ARITHMETIC(4,5, 64)
KERNEL_COMPLETE_FLOATING_ARITHMETIC(4, 5, double)
#endif

#ifdef __FLT128_MANT_DIG__
typedef __float128 float128;
static float128 float128_from_state5(state5 c){
	__float128 ret;
	memcpy(&ret, c.state, 16);
	return ret;
}
static state5 float128_to_state5(float128 c){
	state5 ret;
	memcpy(ret.state, &c, 16);
	return ret;
}
#endif

static void k_muladdmul_v4(state5 *c){
	k_fmul_s3(c->state4s+0);
	k_fmul_s3(c->state4s+1);
	state4 b;
		b.state3s[0] = c->state4s[0].state3s[0];
		b.state3s[1] = c->state4s[1].state3s[0];
	k_fadd_s3(&b);
	c->state3s[0] = b.state3s[0];
}
static void k_mulsubmul_v4(state5 *c){
	k_fmul_s3(c->state4s+0);
	k_fmul_s3(c->state4s+1);
	state4 b;
		b.state3s[0] = c->state4s[0].state3s[0];
		b.state3s[1] = c->state4s[1].state3s[0];
	k_fsub_s3(&b);
	c->state3s[0] = b.state3s[0];
}
static void k_divadddiv_v4(state5 *c){
	k_fdiv_s3(c->state4s+0);
	k_fdiv_s3(c->state4s+1);
	state4 b;
		b.state3s[0] = c->state4s[0].state3s[0];
		b.state3s[1] = c->state4s[1].state3s[0];
	k_fadd_s3(&b);
	c->state3s[0] = b.state3s[0];
}
static void k_divsubdiv_v4(state5 *c){
	k_fdiv_s3(c->state4s+0);
	k_fdiv_s3(c->state4s+1);
	state4 b;
		b.state3s[0] = c->state4s[0].state3s[0];
		b.state3s[1] = c->state4s[1].state3s[0];
	k_fsub_s3(&b);
	c->state3s[0] = b.state3s[0];
}

KERNEL_MULTIPLEX_HALVES_NP(k_addv2, k_fadd_s3, 3, 4, 5, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_subv2, k_fsub_s3, 3, 4, 5, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_dotv2, k_fmul_s3, 3, 4, 5, 0)
static void k_scalev3(state5 *c){
	for(int i = 0; i < 3; i++){
		state4 q;
		q.state3s[1] = c->state3s[i];
		q.state3s[0] = c->state3s[3];
		k_fmul_s3(&q);
		c->state3s[i] = q.state3s[0];
	}
}
KERNEL_SHARED_STATE(k_sumv4, k_fadd_s3, 3, 4, 5, 0)
//KERNEL_MULTIPLEX_SIMD(name, func, nn, nm, iscopy)
KERNEL_MULTIPLEX_SIMD(k_sqrv4, k_fsqr_s3, 3, 5, 0)
static void k_sqrlengthv4(state5 *c){
	k_sqrv4(c);
	k_sumv4(c);
}
static void k_lengthv4(state5 *c){
	k_sqrlengthv4(c);
	k_fsqrt_s3(c->state3s);
}
static void k_normalizev4(state5 *c){
	state3 length;
	{state5 tempc = *c;
			k_lengthv4(&tempc);
			length = tempc.state3s[0];
	}
	for(int i = 0; i<4; i++){
		state4 worker;
		worker.state3s[0] = c->state3s[i];
		worker.state3s[1] = length;
		k_fdiv_s3(&worker);
		c->state3s[i] = worker.state3s[0];
	}
}
#if KERNEL_FAST_FLOAT_MATH
static void k_fisrnormalizev4(state5 *c){
	state3 length;
	{
		state5 tempc = *c;
			k_sqrlengthv4(&tempc);
			length = (tempc.state3s[0]);
			k_fisr(&length);
	}
	for(int i = 0; i<4; i++)
		c->state3s[i] = float_to_state3(float_from_state3(c->state3s[i]) * float_from_state3(length));
}
#endif
static void k_clampf(state5* c){
	float a = float_from_state3(c->state3s[0]);
	float min = float_from_state3(c->state3s[1]);
	float max = float_from_state3(c->state3s[2]);
#if KERNEL_FAST_FLOAT_MATH == 0
	if(!isfinite(a) || !isfinite(min) || !isfinite(max)) return;
#endif
	if(a<min) {c->state3s[0] = float_to_state3(min); return;}
	if(a>max) {c->state3s[0] = float_to_state3(max); return;}
	return;
}
//Enough for a mat2x4 or 4x2
KERNELB(6,16);
KERNELCONV(5,6);
#ifdef INT128_MAX
KERNEL_COMPLETE_ARITHMETIC(5, 6, 128)
#endif

#ifdef __FLT128_MANT_DIG__
KERNEL_COMPLETE_FLOATING_ARITHMETIC(5, 6, float128)
#endif
static void k_scalev4(state6 *c){
	for(int i = 0; i < 4; i++){
		state4 w;
		w.state3s[0] = c->state5s[0].state3s[i];
		w.state3s[1] = c->state5s[1].state3s[0];
		k_fmul_s3(&w);
		c->state3s[i] = w.state3s[0];
	}
	return;
}
KERNEL_MULTIPLEX_HALVES_NP(k_addv4, k_fadd_s3, 3, 4, 6, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_subv4, k_fsub_s3, 3, 4, 6, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_mulv4, k_fmul_s3, 3, 4, 6, 0)
static void k_dotv4(state6 *c){
	k_mulv4(c);
	k_sumv4(c->state5s+0);
}
//Enough for a 4x4. TODO implement SIMD-accelerated matrix math.
KERNELB(7,16);
KERNELCONV(6,7);
/*Limited memory version which works in-place.*/
static void k_mat4_transpose(state7 *c){
	for(int row = 1; row < 4; row++)
	for(int col = 0; col < row; col++){
		state3 temp;
		temp = c->state3s[row*4 + col];
		c->state3s[row*4 + col] = c->state3s[col*4 + row];
		c->state3s[col*4 + row] = temp;
	}
}


static void k_mat4_det(state7 *c){

	state3 a00 = (c->state3s[0]), 	a01 = (c->state3s[1]), 	a02 = (c->state3s[2]), 	a03 = (c->state3s[3]),
			a10 = (c->state3s[4]), 	a11 = (c->state3s[5]), 	a12 = (c->state3s[6]), 	a13 = (c->state3s[7]),
			a20 = (c->state3s[8]), 	a21 = (c->state3s[9]), 	a22 = (c->state3s[10]), a23 = (c->state3s[11]),
			a30 = (c->state3s[12]), a31 = (c->state3s[13]), a32 = (c->state3s[14]), a33 = (c->state3s[15]);
	state5 worker;

	/*dest00 = a00 * a11 - 
			   a01 * a10,*/
			worker.state3s[0] = a00; 	worker.state3s[1] = a11;
			worker.state3s[2] = a01; 	worker.state3s[3] = a10;
		k_mulsubmul_v4(&worker);
	state3 dest00 = worker.state3s[0];
		    /*dest01 = a00 * a12 - 
		    		   a02 * a10,*/
		worker.state3s[0] = a00; 	worker.state3s[1] = a12;
		worker.state3s[2] = a02; 	worker.state3s[3] = a10;
			k_mulsubmul_v4(&worker);
	state3 dest01 = worker.state3s[0];
		    /*dest02 = a00 * a13 -
		    		 	a03 * a10,*/
		worker.state3s[0] = a00; 	worker.state3s[1] = a13;
		worker.state3s[2] = a03; 	worker.state3s[3] = a10;
			k_mulsubmul_v4(&worker);
	state3 dest02 = worker.state3s[0];
		    /*dest03 = a01 * a12 -
		    		 a02 * a11,*/
		worker.state3s[0] = a01; 	worker.state3s[1] = a12;
		worker.state3s[2] = a02; 	worker.state3s[3] = a11;
			k_mulsubmul_v4(&worker);
	state3 dest03 = worker.state3s[0];


		    /*dest04 = a01 * a13 -
		    		 a03 * a11,*/
		worker.state3s[0] = a01; 	worker.state3s[1] = a13;
		worker.state3s[2] = a03; 	worker.state3s[3] = a11;
			k_mulsubmul_v4(&worker);
	state3 dest04 = worker.state3s[0];
		    /*dest05 = a02 * a13 -
		    		 	a03 * a12,*/
	worker.state3s[0] = a02; 	worker.state3s[1] = a13;
	worker.state3s[2] = a03; 	worker.state3s[3] = a12;
		k_mulsubmul_v4(&worker);
	state3 dest05 = worker.state3s[0];
	/*
		dest06 = a20 * a31 -
				 a21 * a30,
	*/

		worker.state3s[0] = a20; 	worker.state3s[1] = a31;
		worker.state3s[2] = a21; 	worker.state3s[3] = a30;
			k_mulsubmul_v4(&worker);
	state3 dest06 = worker.state3s[0];
/*
		    dest07 = a20 * a32 -
		    		 a22 * a30,
*/

		worker.state3s[0] = a20; 	worker.state3s[1] = a32;
		worker.state3s[2] = a22; 	worker.state3s[3] = a30;
			k_mulsubmul_v4(&worker);
	state3 dest07 = worker.state3s[0];
/*
		    dest08 = a20 * a33 -
		    		 a23 * a30,

*/
		worker.state3s[0] = a20; 	worker.state3s[1] = a33;
		worker.state3s[2] = a23; 	worker.state3s[3] = a30;
			k_mulsubmul_v4(&worker);
	state3 dest08 = worker.state3s[0];

		    /*dest09 = a21 * a32 -
		    		 a22 * a31,*/
		worker.state3s[0] = a21; 	worker.state3s[1] = a32;
		worker.state3s[2] = a22; 	worker.state3s[3] = a31;
			k_mulsubmul_v4(&worker);
	state3 dest09 = worker.state3s[0];
		   /*
		    dest10 = a21 * a33 -
		    		 a23 * a31,
		   */
		worker.state3s[0] = a21; 	worker.state3s[1] = a33;
		worker.state3s[2] = a23; 	worker.state3s[3] = a31;
			k_mulsubmul_v4(&worker);
	state3 dest10 = worker.state3s[0];
	/*
		    dest11 = a22 * a33 -
		    		 a23 * a32;
	*/
		worker.state3s[0] = a22; 	worker.state3s[1] = a33;
		worker.state3s[2] = a23; 	worker.state3s[3] = a32;
			k_mulsubmul_v4(&worker);
	state3 dest11 = worker.state3s[0];

//as it so happens, worker.state3 is already
//set to dest11.
//the following line will therefore do nothing.
//TERM 1
	worker.state3s[0] = dest11;
	worker.state3s[1] = dest00;
	k_fmul_s3(worker.state4s);
	//store our result... this is the first term, we aren't
	//adding or subtracting yet
	worker.state4s[1].state3s[0] = worker.state3s[0];
//TERM 2
	worker.state3s[0] = dest01;
	worker.state3s[1] = dest10;
	k_fmul_s3(worker.state4s);
	//subtract off our result (this is a subtracting term.)
	worker.state4s[1].state3s[1] = worker.state3s[0];
	k_fsub_s3(worker.state4s+1);
//TERM 3
	worker.state3s[0] = dest02;
	worker.state3s[1] = dest09;
	k_fmul_s3(worker.state4s);
	//add our result (this is an adding term.)
	worker.state4s[1].state3s[1] = worker.state3s[0];
	k_fadd_s3(worker.state4s+1);
//TERM 4
	worker.state3s[0] = dest03;
	worker.state3s[1] = dest08;
	k_fmul_s3(worker.state4s);
	//add our result (this is an adding term.)
	worker.state4s[1].state3s[1] = worker.state3s[0];
	k_fadd_s3(worker.state4s+1);
//TERM 5
	worker.state3s[0] = dest04;
	worker.state3s[1] = dest07;
	k_fmul_s3(worker.state4s);
	//sub our result
	worker.state4s[1].state3s[1] = worker.state3s[0];
	k_fsub_s3(worker.state4s+1);
//TERM 6
	worker.state3s[0] = dest05;
	worker.state3s[1] = dest06;
	k_fmul_s3(worker.state4s);
	//add our result (this is an adding term.)
	worker.state4s[1].state3s[1] = worker.state3s[0];
	k_fadd_s3(worker.state4s+1);
	c->state3s[0] = worker.state4s[1].state3s[0];
}

static void k_mat4_det_old(state7 *c){
	float a00 = float_from_state3(c->state3s[0]),
			a01 = float_from_state3(c->state3s[1]),
			a02 = float_from_state3(c->state3s[2]), 
			a03 = float_from_state3(c->state3s[3]),
			a10 = float_from_state3(c->state3s[4]), 
			a11 = float_from_state3(c->state3s[5]), 
			a12 = float_from_state3(c->state3s[6]), 
			a13 = float_from_state3(c->state3s[7]),
			a20 = float_from_state3(c->state3s[8]), 
			a21 = float_from_state3(c->state3s[9]), 
			a22 = float_from_state3(c->state3s[10]), 
			a23 = float_from_state3(c->state3s[11]),
			a30 = float_from_state3(c->state3s[12]), 
			a31 = float_from_state3(c->state3s[13]), 
			a32 = float_from_state3(c->state3s[14]), 
			a33 = float_from_state3(c->state3s[15]);
	float dest00 = a00 * a11 - a01 * a10,
		    dest01 = a00 * a12 - a02 * a10,
		    dest02 = a00 * a13 - a03 * a10,
		    dest03 = a01 * a12 - a02 * a11,
		    dest04 = a01 * a13 - a03 * a11,
		    dest05 = a02 * a13 - a03 * a12,
		    dest06 = a20 * a31 - a21 * a30,
		    dest07 = a20 * a32 - a22 * a30,
		    dest08 = a20 * a33 - a23 * a30,
		    dest09 = a21 * a32 - a22 * a31,
		    dest10 = a21 * a33 - a23 * a31,
		    dest11 = a22 * a33 - a23 * a32;
    c->state3s[0] = float_to_state3(
    	dest00 * dest11 - dest01 * dest10 +
    	dest02 * dest09 +
    	dest03 * dest08 - dest04 * dest07 +
    	dest05 * dest06
    );
}
//Enough for TWO 4x4s
KERNELB(8,16);
KERNELCONV(7,8);

KERNEL_MULTIPLEX_HALVES_NP(k_addmat4, k_fadd_s3, 3, 4, 8, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_submat4, k_fsub_s3, 3, 4, 8, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_mulv16, k_fmul_s3, 3, 4, 8, 0)
KERNEL_MULTIPLEX_HALVES_NP(k_divv16, k_fmul_s3, 3, 4, 8, 0)

static void k_backwards_mul_mat4(state8 *c){
	/*Matrix multiplication for dummies.
						col
		A 			B   v 			C
		1 0 0 0 	1 0 0 0     =	X X X X 
	row>0 1 0 0 	0 1 0 0 	=	X X T X
		0 0 1 0 	0 0 1 0 	=	X X X X
		0 0 0 1 	0 0 0 1 	=	X X X X
		where T = dotv4(row, col)

		These matrices are COLUMN MAJOR, which means state7.state3s looks like this:
		0 4 8 12
		1 5 9 13
		2 6 1014
		3 7 1115

		To preserve convention, we are going to say that A is lower half and B is in the upper half.
	*/
	for(int col = 0; col < 4; col++){
		state6 workmem;
		//pre-emptively retrieve the column of B, which we are about to overwite.
		workmem.state5s[1] = c->state7s[0].state5s[col];
		for(int row = 0; row < 4; row++){
			//retrieve the row of A. A is state7s[1]
			workmem.state5s[0].state3s[0] = c->state7s[1].state5s[0].state3s[row];
			workmem.state5s[0].state3s[1] = c->state7s[1].state5s[1].state3s[row];
			workmem.state5s[0].state3s[2] = c->state7s[1].state5s[2].state3s[row];
			workmem.state5s[0].state3s[3] = c->state7s[1].state5s[3].state3s[row];
			k_dotv4(&workmem);
			c->state7s[0].state5s[col].state3s[row] = workmem.state3s[0];
		}
	}
}
static void k_mulmat4(state8 *c){
	k_swap8(c);
	k_backwards_mul_mat4(c);
}

static void k_mat4xvec4(state8 *c){
	state7 mat = c->state7s[0];
	state5 vec = c->state7s[1].state5s[0];
	for(int row = 0; row < 4; row++){
		state6 ret;
		for(int b = 0; b < 4; b++)
			ret.state5s[0].state3s[b] = mat.state3s[row + 4*b];
		ret.state5s[1] = vec;
		k_dotv4(&ret);
		c->state5s[0].state3s[row] = ret.state3s[0];
	}
}
KERNELB(9,16);
KERNELCONV(8,9);
KERNELB(10,16);
KERNELCONV(9,10);

//The eleventh order kernel holds 2^(11-1) bytes, or 1024 bytes.
KERNELB(11,16);
KERNELCONV(10,11);
KERNELB(12,16);
KERNELCONV(11,12);
KERNELB(13,16);
KERNELCONV(12,13);
KERNELB(14,16);
KERNELCONV(13,14);
KERNELB(15,16);
KERNELCONV(14,15);
KERNELB(16,16);
KERNELCONV(15,16);
KERNELB(17,16);
KERNELCONV(16,17);
KERNELB(18,16);
KERNELCONV(17,18);
KERNELB(19,16);
KERNELCONV(18,19);
KERNELB(20,16);
KERNELCONV(19,20);
//Holds an entire megabyte. 2^(21-1) bytes, 2^20 bytes, 2^10 * 2^10, 1024 * 1024 bytes.
KERNELB(21,16);
KERNELCONV(20,21);
//TWO ENTIRE MEGABYTES
KERNELB(22,16);
KERNELCONV(21,22);
//FOUR ENTIRE MEGABYTES
KERNELB(23,16);
KERNELCONV(22,23);
//EIGHT ENTIRE MEGABYTES. As much as the Dreamcast.
KERNELB(24,16);
KERNELCONV(23,24);
//16 megs
KERNELB(25,16);
KERNELCONV(24,25);
//32 megs
KERNELB(26,16);
KERNELCONV(25,26);
//64 megs
KERNELB(27,16);
KERNELCONV(26,27);
//128 megs
KERNELB(28,16);
KERNELCONV(27,28);
//256 megs.
KERNELB(29,16);
KERNELCONV(28,29);
//512 megs.
KERNELB(30,16);
KERNELCONV(29,30);
//1G
KERNELB(31,16);
KERNELCONV(30,31);

#endif
