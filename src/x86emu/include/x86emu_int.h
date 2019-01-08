/****************************************************************************
*
* Realmode X86 Emulator Library
*
* Copyright (c) 2007-2017 SUSE LINUX GmbH; Author: Steffen Winterfeldt
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Description:
*   Header file for library internal macros and definitions.
*
****************************************************************************/


#ifndef __X86EMU_X86EMU_INT_H
#define __X86EMU_X86EMU_INT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// exported symbol - all others are not exported by the library
#define API_SYM			__attribute__((visibility("default")))

#include "x86emu.h"
#include "decode.h"
#include "ops.h"
#include "prim_ops.h"
#include "mem.h"

#define INTR_RAISE_DIV0(a)	x86emu_intr_raise(a, 0, INTR_TYPE_SOFT | INTR_MODE_RESTART, 0)
#define INTR_RAISE_SOFT(a, n)	x86emu_intr_raise(a, n, INTR_TYPE_SOFT, 0)
#define INTR_RAISE_GP(a, err)	x86emu_intr_raise(a, 0x0d, INTR_TYPE_FAULT | INTR_MODE_RESTART | INTR_MODE_ERRCODE, err)
#define INTR_RAISE_UD(a)	x86emu_intr_raise(a, 0x06, INTR_TYPE_FAULT | INTR_MODE_RESTART, 0)

#define MODE_REPE		((emu)->x86.mode & _MODE_REPE)
#define MODE_REPNE		((emu)->x86.mode & _MODE_REPNE)
#define MODE_REP		((emu)->x86.mode & (_MODE_REPE | _MODE_REPNE))
#define MODE_DATA32		((emu)->x86.mode & _MODE_DATA32)
#define MODE_ADDR32		((emu)->x86.mode & _MODE_ADDR32)
#define MODE_STACK32		((emu)->x86.mode & _MODE_STACK32)
#define MODE_CODE32		((emu)->x86.mode & _MODE_CODE32)
#define MODE_HALTED		((emu)->x86.mode & _MODE_HALTED)

#define MODE_PROTECTED(a)	((a)->x86.R_CR0 & 1)
#define MODE_REAL(a)		(!MODE_PROTECTED(a))

#define TOGGLE_FLAG(flag)     	((emu)->x86.R_FLG ^= (flag))
#define SET_FLAG(flag)        	((emu)->x86.R_FLG |= (flag))
#define CLEAR_FLAG(flag)      	((emu)->x86.R_FLG &= ~(flag))
#define ACCESS_FLAG(flag)     	((emu)->x86.R_FLG & (flag))
#define CLEARALL_FLAG(m)    	((emu)->x86.R_FLG = 0)

#define CONDITIONAL_SET_FLAG(COND,FLAG) \
  if(COND) SET_FLAG(FLAG); else CLEAR_FLAG(FLAG)

#define LOG_STR(a) memcpy(*p, a, sizeof (a) - 1), *p += sizeof (a) - 1
#define LOG_FREE(emu) ((emu)->log.size + (emu)->log.buf - (emu)->log.ptr)

#if defined(__i386__) || defined (__x86_64__)
#define WITH_TSC	1
#define WITH_IOPL	1
#else
#define WITH_TSC	0
#define WITH_IOPL	0
#endif


#if WITH_TSC
#if defined(__i386__)
static inline u64 tsc()
{
  register u64 tsc asm ("%eax");

  asm (
    "rdtsc"
    : "=r" (tsc)
  );

  return tsc;
}
#endif

#if defined (__x86_64__)
static inline u64 tsc()
{
  register u64 tsc asm ("%rax");

  asm (
    "push %%rdx\n"
    "rdtsc\n"
    "xchg %%edx,%%eax\n"
    "shl $32,%%rax\n"
    "add %%rdx,%%rax\n"
    "pop %%rdx"
    : "=r" (tsc)
  );

  return tsc;
}
#endif

#endif


#if WITH_IOPL
#if defined(__i386__)
static inline unsigned getiopl() 
{
  register u32 i asm ("%eax");
  
  asm(
    "pushf\n"
    "pop %%eax"
    : "=r" (i)
  );

  i = (i >> 12) & 3;

  return i;
}
#endif

#if defined (__x86_64__)
static inline unsigned getiopl()
{
  register unsigned i asm ("%rax");

  asm(
    "pushf\n"
    "pop %%rax"
    : "=r" (i)
  );

  i = (i >> 12) & 3;

  return i;
}
#endif

#endif


#endif /* __X86EMU_X86EMU_INT_H */

