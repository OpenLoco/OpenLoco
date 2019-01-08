/****************************************************************************
*
* Realmode X86 Emulator Library
*
* Copyright (c) 1996-1999 SciTech Software, Inc.
* Copyright (c) David Mosberger-Tang
* Copyright (c) 1999 Egbert Eich
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
*   Header file for public interface functions.
*
*   Applications linking against libx86emu should only include this header.
*
****************************************************************************/


#ifndef __X86EMU_X86EMU_H
#define __X86EMU_X86EMU_H

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

#include <stdint.h>


/*---------------------- Macros and type definitions ----------------------*/

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t


/*
 * General EAX, EBX, ECX, EDX type registers.  Note that for
 * portability, and speed, the issue of byte swapping is not addressed
 * in the registers.  All registers are stored in the default format
 * available on the host machine.  The only critical issue is that the
 * registers should line up EXACTLY in the same manner as they do in
 * the 386.  That is:
 *
 * EAX & 0xff  === AL
 * EAX & 0xffff == AX
 *
 * etc.  The result is that alot of the calculations can then be
 * done using the native instruction set fully.
 */

#ifdef	__BIG_ENDIAN__

typedef struct {
  u32 e_reg;
} I32_reg_t;

typedef struct {
  u16 filler0, x_reg;
} I16_reg_t;

typedef struct {
  u8 filler0, filler1, h_reg, l_reg;
} I8_reg_t;

#else /* !__BIG_ENDIAN__ */

typedef struct {
  u32 e_reg;
} I32_reg_t;

typedef struct {
  u16 x_reg;
} I16_reg_t;

typedef struct {
  u8 l_reg, h_reg;
} I8_reg_t;

#endif /* BIG_ENDIAN */

typedef union {
  I32_reg_t I32_reg;
  I16_reg_t I16_reg;
  I8_reg_t I8_reg;
} i386_general_register;

struct i386_general_regs {
  i386_general_register A, B, C, D;
};

struct i386_special_regs {
  i386_general_register SP, BP, SI, DI, IP;
  u32 FLAGS;
};


typedef struct {
  union {
    u32 base;		// segment base
    u32 offset;		// gate offset
  };
  union {
    u32 limit;		// segment limit
    struct {
      u16 sel;		// gate selector
      u16 w_count;	// gate (d)word count
    };
  };
  u16 acc;		// access flags (12 bits)
  unsigned invalid:1;	// invalid descriptor type
  unsigned seg:1;	// is segment
  unsigned dpl:2;
  unsigned r:1;		// readable seg
  unsigned w:1;		// writable seg
  unsigned x:1;		// executable seg
  unsigned c:1;		// conforming code seg
  unsigned a:1;		// accessed seg
  unsigned p:1;		// present
  unsigned g:1;		// granularity
  unsigned ed:1;	// expand down data seg
  unsigned ldt:1;	// ldt
  unsigned c_gate:1;	// call gate
  unsigned i_gate:1;	// interrupt gate
  unsigned t_gate:1;	// task gate
  unsigned tss:1;	// tss
  unsigned busy:1;	// tss is busy
  unsigned trap:1;	// interrupt gate is trap gate
  unsigned is_i386:1;	// i386 (32 bit) descriptor or 32 bit segment
} descr_t;


/*  
 * segment registers here represent 16 bit selectors & base/limit cache
 * ldt & tr are quite similar to segment selectors
 */
typedef struct {
  u32 base, limit;
  u16 sel;
  u16 acc;
} sel_t;

#define ACC_G(a)	((a >> 11) & 1)		/* 0/1: granularity bytes/4k */
#define ACC_D(a)	((a >> 10) & 1)		/* 0/1: default size 16/32 bit */
#define ACC_P(a)	((a >> 7) & 1)		/* 0/1: present no/yes */
#define ACC_DPL(a)	((a >> 5) & 3)		/* 0..3: dpl */
#define ACC_S(a)	((a >> 4) & 1)		/* 0/1: system/normal  */
#define ACC_E(a)	((a >> 3) & 1)		/* 0/1: type data/code (ACC_S = normal) */
#define ACC_ED(a)	((a >> 2) & 1)		/* 0/1: expand up/down (ACC_E = data) */
#define ACC_C(a)	((a >> 2) & 1)		/* 0/1: conforming no/yes (ACC_E = code) */
#define ACC_W(a)	((a >> 1) & 1)		/* 0/1: writable no/yes (ACC_E = data) */
#define ACC_R(a)	((a >> 1) & 1)		/* 0/1: readable no/yes (ACC_E = code) */
#define ACC_A(a)	(a & 1)			/* 0/1: accessed no/yes */
#define ACC_TYPE(a)	(a & 0xf)		/* 0..0xf: system descr type (ACC_S = system) */

/* 8 bit registers */
#define R_AH		gen.A.I8_reg.h_reg
#define R_AL		gen.A.I8_reg.l_reg
#define R_BH		gen.B.I8_reg.h_reg
#define R_BL		gen.B.I8_reg.l_reg
#define R_CH		gen.C.I8_reg.h_reg
#define R_CL		gen.C.I8_reg.l_reg
#define R_DH		gen.D.I8_reg.h_reg
#define R_DL		gen.D.I8_reg.l_reg

/* 16 bit registers */
#define R_AX		gen.A.I16_reg.x_reg
#define R_BX		gen.B.I16_reg.x_reg
#define R_CX		gen.C.I16_reg.x_reg
#define R_DX		gen.D.I16_reg.x_reg

/* 32 bit extended registers */
#define R_EAX		gen.A.I32_reg.e_reg
#define R_EBX		gen.B.I32_reg.e_reg
#define R_ECX		gen.C.I32_reg.e_reg
#define R_EDX		gen.D.I32_reg.e_reg

/* special registers */
#define R_SP		spc.SP.I16_reg.x_reg
#define R_BP		spc.BP.I16_reg.x_reg
#define R_SI		spc.SI.I16_reg.x_reg
#define R_DI		spc.DI.I16_reg.x_reg
#define R_IP		spc.IP.I16_reg.x_reg
#define R_FLG		spc.FLAGS

/* special registers */
#define R_ESP		spc.SP.I32_reg.e_reg
#define R_EBP		spc.BP.I32_reg.e_reg
#define R_ESI		spc.SI.I32_reg.e_reg
#define R_EDI		spc.DI.I32_reg.e_reg
#define R_EIP		spc.IP.I32_reg.e_reg
#define R_EFLG		spc.FLAGS

/* segment registers */
#define R_ES_INDEX	0
#define R_CS_INDEX	1
#define R_SS_INDEX	2
#define R_DS_INDEX	3
#define R_FS_INDEX	4
#define R_GS_INDEX	5
#define R_NOSEG_INDEX	6

#define R_ES_SEL	seg + R_ES_INDEX
#define R_ES		seg[R_ES_INDEX].sel
#define R_ES_BASE	seg[R_ES_INDEX].base
#define R_ES_LIMIT	seg[R_ES_INDEX].limit
#define R_ES_ACC	seg[R_ES_INDEX].acc

#define R_CS_SEL	seg + R_CS_INDEX
#define R_CS		seg[R_CS_INDEX].sel
#define R_CS_BASE	seg[R_CS_INDEX].base
#define R_CS_LIMIT	seg[R_CS_INDEX].limit
#define R_CS_ACC	seg[R_CS_INDEX].acc

#define R_SS_SEL	seg + R_SS_INDEX
#define R_SS		seg[R_SS_INDEX].sel
#define R_SS_BASE	seg[R_SS_INDEX].base
#define R_SS_LIMIT	seg[R_SS_INDEX].limit
#define R_SS_ACC	seg[R_SS_INDEX].acc

#define R_DS_SEL	seg + R_DS_INDEX
#define R_DS		seg[R_DS_INDEX].sel
#define R_DS_BASE	seg[R_DS_INDEX].base
#define R_DS_LIMIT	seg[R_DS_INDEX].limit
#define R_DS_ACC	seg[R_DS_INDEX].acc

#define R_FS_SEL	seg + R_FS_INDEX
#define R_FS		seg[R_FS_INDEX].sel
#define R_FS_BASE	seg[R_FS_INDEX].base
#define R_FS_LIMIT	seg[R_FS_INDEX].limit
#define R_FS_ACC	seg[R_FS_INDEX].acc

#define R_GS_SEL	seg + R_GS_INDEX
#define R_GS		seg[R_GS_INDEX].sel
#define R_GS_BASE	seg[R_GS_INDEX].base
#define R_GS_LIMIT	seg[R_GS_INDEX].limit
#define R_GS_ACC	seg[R_GS_INDEX].acc

#define R_NOSEG_SEL	seg + R_NOSEG_INDEX
#define R_NOSEG		seg[R_NOSEG_INDEX].sel
#define R_NOSEG_BASE	seg[R_NOSEG_INDEX].base
#define R_NOSEG_LIMIT	seg[R_NOSEG_INDEX].limit
#define R_NOSEG_ACC	seg[R_NOSEG_INDEX].acc

/* other registers: tr, ldt, gdt, idt */
#define R_TR		tr.sel
#define R_TR_BASE	tr.base
#define R_TR_LIMIT	tr.limit
#define R_TR_ACC	tr.acc
#define R_LDT		ldt.sel
#define R_LDT_BASE	ldt.base
#define R_LDT_LIMIT	ldt.limit
#define R_LDT_ACC	ldt.acc
#define R_GDT_BASE	gdt.base
#define R_GDT_LIMIT	gdt.limit
#define R_IDT_BASE	idt.base
#define R_IDT_LIMIT	idt.limit

/* machine status & debug registers: CRx, DRx, TRx */
#define R_CR0		crx[0]
#define R_CR1		crx[1]
#define R_CR2		crx[2]
#define R_CR3		crx[3]
#define R_CR4		crx[4]
#define R_CR5		crx[5]
#define R_CR6		crx[6]
#define R_CR7		crx[7]
#define R_DR0		drx[0]
#define R_DR1		drx[1]
#define R_DR2		drx[2]
#define R_DR3		drx[3]
#define R_DR4		drx[4]
#define R_DR5		drx[5]
#define R_DR6		drx[6]
#define R_DR7		drx[7]

#define R_TSC		msr[0x10]
#define R_LAST_REAL_TSC	msr[0x11]
#define R_REAL_TSC	msr[0x12]

/* flag conditions   */
#define FB_CF 0x0001            /* CARRY flag  */
#define FB_PF 0x0004            /* PARITY flag */
#define FB_AF 0x0010            /* AUX  flag   */
#define FB_ZF 0x0040            /* ZERO flag   */
#define FB_SF 0x0080            /* SIGN flag   */
#define FB_TF 0x0100            /* TRAP flag   */
#define FB_IF 0x0200            /* INTERRUPT ENABLE flag */
#define FB_DF 0x0400            /* DIR flag    */
#define FB_OF 0x0800            /* OVERFLOW flag */

/* 80286 and above always have bit#1 set */
#define F_ALWAYS_ON  (0x0002)   /* flag bits always on */

/*
 * Define a mask for only those flag bits we will ever pass back 
 * (via PUSHF) 
 */
#define F_MSK (FB_CF|FB_PF|FB_AF|FB_ZF|FB_SF|FB_TF|FB_IF|FB_DF|FB_OF)

/* following bits masked in to a 16bit quantity */

#define F_CF 0x0001             /* CARRY flag  */
#define F_PF 0x0004             /* PARITY flag */
#define F_AF 0x0010             /* AUX  flag   */
#define F_ZF 0x0040             /* ZERO flag   */
#define F_SF 0x0080             /* SIGN flag   */
#define F_TF 0x0100             /* TRAP flag   */
#define F_IF 0x0200             /* INTERRUPT ENABLE flag */
#define F_DF 0x0400             /* DIR flag    */
#define F_OF 0x0800             /* OVERFLOW flag */

#define X86EMU_TOGGLE_FLAG(emu, flag)     (emu->x86.R_FLG ^= (flag))
#define X86EMU_SET_FLAG(emu, flag)        (emu->x86.R_FLG |= (flag))
#define X86EMU_CLEAR_FLAG(emu, flag)      (emu->x86.R_FLG &= ~(flag))

/*
 * Emulator machine state.
 * Segment usage control.
 */
#define _MODE_SEG_DS_SS         0x00000001
#define _MODE_REPE              0x00000002
#define _MODE_REPNE             0x00000004
#define _MODE_DATA32            0x00000008
#define _MODE_ADDR32            0x00000010
#define _MODE_STACK32           0x00000020
#define _MODE_CODE32            0x00000040
#define _MODE_HALTED            0x00000080

#define INTR_TYPE_SOFT		1
#define INTR_TYPE_FAULT		2
#define INTR_MODE_RESTART	0x100
#define INTR_MODE_ERRCODE	0x200

#define X86EMU_RUN_TIMEOUT	(1 << 0)
#define X86EMU_RUN_MAX_INSTR	(1 << 1)
#define X86EMU_RUN_NO_EXEC	(1 << 2)
#define X86EMU_RUN_NO_CODE	(1 << 3)
#define X86EMU_RUN_LOOP		(1 << 4)

#define X86EMU_MEMIO_8		0
#define X86EMU_MEMIO_16		1
#define X86EMU_MEMIO_32		2
#define X86EMU_MEMIO_8_NOPERM	3
#define X86EMU_MEMIO_R		(0 << 8)
#define X86EMU_MEMIO_W		(1 << 8)
#define X86EMU_MEMIO_X		(2 << 8)
#define X86EMU_MEMIO_I		(3 << 8)
#define X86EMU_MEMIO_O		(4 << 8)

#define X86EMU_MSRS		0x800

struct x86emu_s;

typedef unsigned (* x86emu_memio_handler_t)(struct x86emu_s *, u32 addr, u32 *val, unsigned type);
typedef int (* x86emu_intr_handler_t)(struct x86emu_s *, u8 num, unsigned type);
typedef int (* x86emu_code_handler_t)(struct x86emu_s *);
typedef void (* x86emu_flush_func_t)(struct x86emu_s *, char *buf, unsigned size);

typedef struct {
  struct i386_general_regs gen;
  struct i386_special_regs spc;
  sel_t seg[8];
  sel_t ldt;
  sel_t tr;
  u32 crx[8];
  u32 drx[8];
  struct {
    u32 base, limit;
  } gdt;
  struct {
    u32 base, limit;
  } idt;
  u64 *msr;			/* X86EMU_MSRS */
  unsigned char *msr_perm;	/* X86EMU_MSRS */
  u32 mode;
  sel_t *default_seg;
  u32 saved_eip;
  u16 saved_cs;
  char decode_seg[4];
  unsigned char instr_buf[32];	/* instruction bytes */
  unsigned instr_len;		/* bytes in instr_buf */
  char disasm_buf[256];
  char *disasm_ptr;
  u8 intr_nr;
  unsigned intr_type;
  unsigned intr_errcode;
  unsigned intr_stats[0x100];
  unsigned debug_start, debug_len;
} x86emu_regs_t;


#define X86EMU_TRACE_REGS	(1 << 0)
#define X86EMU_TRACE_CODE	(1 << 1)
#define X86EMU_TRACE_DATA	(1 << 2)
#define X86EMU_TRACE_ACC	(1 << 3)
#define X86EMU_TRACE_IO		(1 << 4)
#define X86EMU_TRACE_INTS	(1 << 5)
#define X86EMU_TRACE_TIME	(1 << 6)
#define X86EMU_TRACE_DEBUG	(1 << 7)
#define X86EMU_TRACE_DEFAULT	(X86EMU_TRACE_REGS | X86EMU_TRACE_CODE | X86EMU_TRACE_DATA | X86EMU_TRACE_IO | X86EMU_TRACE_INTS)

#define X86EMU_DUMP_REGS	(1 << 0)
#define X86EMU_DUMP_MEM		(1 << 1)
#define X86EMU_DUMP_ACC_MEM	(1 << 2)
#define X86EMU_DUMP_INV_MEM	(1 << 3)
#define X86EMU_DUMP_ATTR	(1 << 4)
#define X86EMU_DUMP_ASCII	(1 << 5)
#define X86EMU_DUMP_IO		(1 << 6)
#define X86EMU_DUMP_INTS	(1 << 7)
#define X86EMU_DUMP_TIME	(1 << 8)
#define X86EMU_DUMP_DEFAULT	(X86EMU_DUMP_REGS | X86EMU_DUMP_INV_MEM | X86EMU_DUMP_ATTR | X86EMU_DUMP_ASCII | X86EMU_DUMP_IO | X86EMU_DUMP_INTS | X86EMU_DUMP_TIME)

#define X86EMU_PERM_R		(1 << 0)
#define X86EMU_PERM_W		(1 << 1)
#define X86EMU_PERM_X		(1 << 2)
#define X86EMU_PERM_VALID	(1 << 3)
#define X86EMU_ACC_R		(1 << 4)
#define X86EMU_ACC_W		(1 << 5)
#define X86EMU_ACC_X		(1 << 6)
#define X86EMU_ACC_INVALID	(1 << 7)

/* for convenience */
#define X86EMU_PERM_RW		(X86EMU_PERM_R | X86EMU_PERM_W)
#define X86EMU_PERM_RX		(X86EMU_PERM_R | X86EMU_PERM_X)
#define X86EMU_PERM_RWX		(X86EMU_PERM_R | X86EMU_PERM_W | X86EMU_PERM_X)


/* 4k pages */
#define X86EMU_PAGE_BITS	12
#define X86EMU_PTABLE_BITS	10
#define X86EMU_PDIR_BITS	(32 - X86EMU_PTABLE_BITS - X86EMU_PAGE_BITS)
#define X86EMU_PAGE_SIZE	(1 << X86EMU_PAGE_BITS)

#define X86EMU_IO_PORTS		(1 << 16)

typedef struct {
  unsigned char *attr;	// malloc'ed
  unsigned char *data;	// NOT malloc'ed
  unsigned char def_attr;
} mem2_page_t;

typedef mem2_page_t mem2_ptable_t[1 << X86EMU_PTABLE_BITS];
typedef mem2_ptable_t *mem2_pdir_t[1 << X86EMU_PDIR_BITS];

typedef struct {
  mem2_pdir_t *pdir;
  unsigned invalid:1;
  unsigned char def_attr;
} x86emu_mem_t;


/****************************************************************************
REMARKS:
Structure maintaining the emulator machine state.

MEMBERS:
private			- private data pointer
x86			- X86 registers
****************************************************************************/
typedef struct x86emu_s {
  x86emu_regs_t x86;
  x86emu_code_handler_t code_check;
  x86emu_memio_handler_t memio;
  x86emu_intr_handler_t intr;
  x86emu_mem_t *mem;
  struct {
    unsigned char *map;
    unsigned *stats_i, *stats_o;
    unsigned iopl_needed:1;
    unsigned iopl_ok:1;
  } io;
  struct {
    x86emu_flush_func_t flush;
    unsigned size;
    char *buf;
    char *ptr;
    unsigned trace;		/* trace flags: X86EMU_TRACE_* */
  } log;
  unsigned timeout;
  u64 max_instr;
  union {
    void *_private;
#ifndef	__cplusplus
    void *private;		/* deprecated: use _private */
#endif
  };
} x86emu_t;

/*-------------------------- Function Prototypes --------------------------*/

x86emu_t *x86emu_new(unsigned def_mem_perm, unsigned def_io_perm);
x86emu_t *x86emu_done(x86emu_t *emu);
x86emu_t *x86emu_clone(x86emu_t *emu);

void x86emu_reset(x86emu_t *emu);
unsigned x86emu_run(x86emu_t *emu, unsigned flags) __attribute__ ((nonnull (1)));
void x86emu_stop(x86emu_t *emu);

void x86emu_set_log(x86emu_t *emu, unsigned buffer_size, x86emu_flush_func_t flush);
unsigned x86emu_clear_log(x86emu_t *emu, int flush) __attribute__ ((nonnull (1)));
void x86emu_log(x86emu_t *emu, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
void x86emu_dump(x86emu_t *emu, int flags);

void x86emu_set_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm);
void x86emu_set_io_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm);
void x86emu_set_page(x86emu_t *emu, unsigned page, void *address);
void x86emu_reset_access_stats(x86emu_t *emu);

x86emu_code_handler_t x86emu_set_code_handler(x86emu_t *emu, x86emu_code_handler_t handler);
x86emu_intr_handler_t x86emu_set_intr_handler(x86emu_t *emu, x86emu_intr_handler_t handler);
x86emu_memio_handler_t x86emu_set_memio_handler(x86emu_t *emu, x86emu_memio_handler_t handler);

void x86emu_intr_raise(x86emu_t *emu, u8 intr_nr, unsigned type, unsigned err);

unsigned x86emu_read_byte(x86emu_t *emu, unsigned addr);
unsigned x86emu_read_byte_noperm(x86emu_t *emu, unsigned addr);
unsigned x86emu_read_word(x86emu_t *emu, unsigned addr);
unsigned x86emu_read_dword(x86emu_t *emu, unsigned addr);
void x86emu_write_byte(x86emu_t *emu, unsigned addr, unsigned val);
void x86emu_write_byte_noperm(x86emu_t *emu, unsigned addr, unsigned val);
void x86emu_write_word(x86emu_t *emu, unsigned addr, unsigned val);
void x86emu_write_dword(x86emu_t *emu, unsigned addr, unsigned val);

void x86emu_set_seg_register(x86emu_t *emu, sel_t *seg, u16 val);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_X86EMU_H */

