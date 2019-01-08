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
*   Subroutines related to instruction decoding and logging.
*
****************************************************************************/


#include "include/x86emu_int.h"
#include <time.h>


/*----------------------------- Implementation ----------------------------*/

static void handle_interrupt(x86emu_t *emu);
static void generate_int(x86emu_t *emu, u8 nr, unsigned type, unsigned errcode);
static void log_regs(x86emu_t *emu);
static void log_code(x86emu_t *emu);
static void check_data_access(x86emu_t *emu, sel_t *seg, u32 ofs, u32 size);
static unsigned decode_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type);
static unsigned emu_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type);
static void idt_lookup(x86emu_t *emu, u8 nr, u32 *new_cs, u32 *new_eip);


int has_hook(x86emu_t *emu);

/****************************************************************************
REMARKS:
Main execution loop for the emulator. We return from here when the system
halts, timeouts, or one of the conditions in flags are met.
****************************************************************************/
API_SYM unsigned x86emu_run(x86emu_t *emu, unsigned flags)
{
  u8 op1, u_m1;
  s32 ofs32;
  char **p;
  unsigned u, rs = 0;
  time_t t0;
  int has_prefix;
#if WITH_TSC
  u64 tsc_ofs;
#endif

  p = &emu->log.ptr;

  t0 = time(NULL);

#if WITH_TSC
  tsc_ofs = tsc() - emu->x86.R_REAL_TSC;
#endif

#if WITH_IOPL
  emu->io.iopl_ok = emu->io.iopl_needed && getiopl() != 3 ? 0 : 1;
#else
  emu->io.iopl_ok = 1;
#endif

  for(;;) {
    printf("> 0x%08X\n", emu->x86.R_EIP);
    *(emu->x86.disasm_ptr = emu->x86.disasm_buf) = 0;

    emu->x86.instr_len = 0;

    emu->x86.mode = 0;

    if(ACC_D(emu->x86.R_CS_ACC)) {
      emu->x86.mode |= _MODE_DATA32 | _MODE_ADDR32 | _MODE_CODE32;
    }
    if(ACC_D(emu->x86.R_SS_ACC)) {
      emu->x86.mode |= _MODE_STACK32;
    }

    emu->x86.default_seg = NULL;

    /* save EIP and CS values */
    emu->x86.saved_cs = emu->x86.R_CS;
    emu->x86.saved_eip = emu->x86.R_EIP;

    log_regs(emu);

    if(
      (flags & X86EMU_RUN_MAX_INSTR) &&
      emu->max_instr &&
      emu->x86.R_TSC >= emu->max_instr
    ) {
      rs |= X86EMU_RUN_MAX_INSTR;
      break;
    }

    if(
      (flags & X86EMU_RUN_TIMEOUT) &&
      emu->timeout &&
      !(emu->x86.R_TSC & 0xffff) &&
      time(NULL) - t0 > emu->timeout
    ) {
      rs |= X86EMU_RUN_TIMEOUT;
      break;
    }

    if(emu->code_check) {
      if((*emu->code_check)(emu) || MODE_HALTED) {
        rs |= X86EMU_RUN_NO_CODE;
        break;
      }
    }

    if (has_hook(emu))
    {
      continue;
    }

    if(emu->x86.R_EIP == 0) {
      return -1;
    }

    memcpy(emu->x86.decode_seg, "[", 1);

    /* handle prefixes here */
    has_prefix = 1;
    while(has_prefix) {
      switch(op1 = fetch_byte(emu)) {
        case 0x26:
          memcpy(emu->x86.decode_seg, "es:[", 4);
          emu->x86.default_seg = emu->x86.seg + R_ES_INDEX;
          break;
        case 0x2e:
          memcpy(emu->x86.decode_seg, "cs:[", 4);
          emu->x86.default_seg = emu->x86.seg + R_CS_INDEX;
          break;
        case 0x36:
          memcpy(emu->x86.decode_seg, "ss:[", 4);
          emu->x86.default_seg = emu->x86.seg + R_SS_INDEX;
          break;
        case 0x3e:
          memcpy(emu->x86.decode_seg, "ds:[", 4);
          emu->x86.default_seg = emu->x86.seg + R_DS_INDEX;
          break;
        case 0x64:
          memcpy(emu->x86.decode_seg, "fs:[", 4);
          emu->x86.default_seg = emu->x86.seg + R_FS_INDEX;
          break;
        case 0x65:
          memcpy(emu->x86.decode_seg, "gs:[", 4);
          emu->x86.default_seg = emu->x86.seg + R_GS_INDEX;
          break;
        case 0x66:
          emu->x86.mode ^= _MODE_DATA32;
          break;
        case 0x67:
          emu->x86.mode ^= _MODE_ADDR32;
          break;
        case 0xf0:
          OP_DECODE("lock: ");
          break;
        case 0xf2:
          OP_DECODE("repne ");
          emu->x86.mode |= _MODE_REPNE;
          break;
        case 0xf3:
          OP_DECODE("repe ");
          emu->x86.mode |= _MODE_REPE;
          break;
        default:
          has_prefix = 0;
          break;
      }
    }

    if(MODE_HALTED) {
      printf("  HALTED");
      rs |= X86EMU_RUN_NO_EXEC;
      emu->x86.R_EIP = emu->x86.saved_eip;
      break;
    }

    if(flags & X86EMU_RUN_LOOP) {
      u = emu->x86.R_CS_BASE + emu->x86.R_EIP;

      ofs32  = 0;

      if(op1 == 0xeb) {
        ofs32 = (s32) (s8) x86emu_read_byte_noperm(emu, u) + 1;
      }
      else if(op1 == 0xe9) {
        if(MODE_DATA32) {
          ofs32 = (x86emu_read_byte_noperm(emu, u) +
            (x86emu_read_byte_noperm(emu, u + 1) << 8)) +
            (x86emu_read_byte_noperm(emu, u + 2) << 16) +
            (x86emu_read_byte_noperm(emu, u + 3) << 24) + 4;
        }
        else {
          ofs32 = (s32) (s16) (
            x86emu_read_byte_noperm(emu, u) +
            (x86emu_read_byte_noperm(emu, u + 1) << 8)) + 2;
        }
      }

      if(ofs32) {
        if(emu->x86.R_EIP + ofs32 == emu->x86.saved_eip) {
          rs |= X86EMU_RUN_LOOP;
        }
        else if(emu->x86.R_EIP + 1 + ofs32 == emu->x86.saved_eip && emu->x86.saved_eip >= 1) {
          u_m1 = x86emu_read_byte_noperm(emu, emu->x86.R_CS_BASE + emu->x86.saved_eip - 1);
          if(u_m1 >= 0xf8 && u_m1 <= 0xfd) rs |= X86EMU_RUN_LOOP;
        }

        if(rs) x86emu_stop(emu);
      }
    }

    if(flags & X86EMU_RUN_NO_CODE) {
      u = emu->x86.R_CS_BASE + emu->x86.R_EIP;

      if(emu->x86.mode == 0 && op1 == 0x00 && x86emu_read_byte_noperm(emu, u) == 0x00) {
        rs |= X86EMU_RUN_NO_CODE;
      }

      if(rs) x86emu_stop(emu);
    }

    printf("  opcode %02X\n", op1);
    (*x86emu_optab[op1])(emu, op1);

    *emu->x86.disasm_ptr = 0;

    handle_interrupt(emu);

#if WITH_TSC
    emu->x86.R_LAST_REAL_TSC = emu->x86.R_REAL_TSC;
    emu->x86.R_REAL_TSC = tsc() - tsc_ofs;
#endif

    log_code(emu);

    if(emu->x86.debug_len) {
      emu_process_debug(emu, emu->x86.debug_start, emu->x86.debug_len);
      emu->x86.debug_len = emu->x86.debug_start = 0;
    }

    emu->x86.R_TSC++;	// time stamp counter

    if(MODE_HALTED) break;
  }

  if(*p) {
    if((rs & X86EMU_RUN_TIMEOUT)) {
      LOG_STR("* timeout\n");
    }
    if((rs & X86EMU_RUN_MAX_INSTR)) {
      LOG_STR("* too many instructions\n");
    }
    if((rs & X86EMU_RUN_NO_EXEC)) {
      LOG_STR("* memory not executable\n");
    }
    if((rs & X86EMU_RUN_NO_CODE)) {
      LOG_STR("* no proper code\n");
    }
    if((rs & X86EMU_RUN_LOOP)) {
      LOG_STR("* infinite loop\n");
    }
    **p = 0;
  }

#if WITH_TSC
  emu->x86.R_REAL_TSC = tsc() - tsc_ofs;
#endif

  return rs;
}

/****************************************************************************
REMARKS:
Halts the system by setting the halted system flag.
****************************************************************************/
API_SYM void x86emu_stop(x86emu_t *emu)
{
  emu->x86.mode |= _MODE_HALTED;
}

/****************************************************************************
REMARKS:
Handles any pending asychronous interrupts.
****************************************************************************/
void handle_interrupt(x86emu_t *emu)
{
  char **p = &emu->log.ptr;
  unsigned lf;

  if(emu->x86.intr_type) {
    if((emu->log.trace & X86EMU_TRACE_INTS) && *p) {
      lf = LOG_FREE(emu);
      if(lf < 128) lf = x86emu_clear_log(emu, 1);
      if(lf >= 128) {
        if((emu->x86.intr_type & 0xff) == INTR_TYPE_FAULT) {
          LOG_STR("* fault ");
        }
        else {
          LOG_STR("* int ");
        }
        decode_hex2(emu, p, emu->x86.intr_nr & 0xff);
        LOG_STR("\n");
        **p = 0;
      }
    }

    generate_int(emu, emu->x86.intr_nr, emu->x86.intr_type, emu->x86.intr_errcode);
  }

  emu->x86.intr_type = 0;
}


API_SYM void x86emu_intr_raise(x86emu_t *emu, u8 intr_nr, unsigned type, unsigned err)
{
  if(emu && !emu->x86.intr_type) {
    emu->x86.intr_nr = intr_nr;
    emu->x86.intr_type = type;
    emu->x86.intr_errcode = err;
  }
}

/****************************************************************************
PARAMETERS:
mod		- Mod value from decoded byte
regh	- Reg h value from decoded byte
regl	- Reg l value from decoded byte

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.
****************************************************************************/
void fetch_decode_modrm(x86emu_t *emu, int *mod, int *regh, int *regl)
{
  u8 fetched;

  fetched = fetch_byte(emu);

  *mod  = (fetched >> 6) & 0x03;
  *regh = (fetched >> 3) & 0x07;
  *regl = (fetched >> 0) & 0x07;
}

/****************************************************************************
RETURNS:
Immediate byte value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.
****************************************************************************/
u8 fetch_byte(x86emu_t *emu)
{
  u32 val;
  unsigned err;

  err = decode_memio(emu, emu->x86.R_CS_BASE + emu->x86.R_EIP, &val, X86EMU_MEMIO_8 + X86EMU_MEMIO_X);

  if(err) x86emu_stop(emu);

  if(MODE_CODE32) {
    emu->x86.R_EIP++;
  }
  else {
    emu->x86.R_IP++;
  }

  if(emu->x86.instr_len < sizeof emu->x86.instr_buf) {
    emu->x86.instr_buf[emu->x86.instr_len++] = val;
  }

  return val;
}

/****************************************************************************
RETURNS:
Immediate word value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.
****************************************************************************/
u16 fetch_word(x86emu_t *emu)
{
  u32 val;
  unsigned err;

  err = decode_memio(emu, emu->x86.R_CS_BASE + emu->x86.R_EIP, &val, X86EMU_MEMIO_16 + X86EMU_MEMIO_X);

  if(err) x86emu_stop(emu);

  if(MODE_CODE32) {
    emu->x86.R_EIP += 2;
  }
  else {
    emu->x86.R_IP += 2;
  }

  if(emu->x86.instr_len + 1 < sizeof emu->x86.instr_buf) {
    emu->x86.instr_buf[emu->x86.instr_len++] = val;
    emu->x86.instr_buf[emu->x86.instr_len++] = val >> 8;
  }

  return val;
}

/****************************************************************************
RETURNS:
Immediate lone value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.
****************************************************************************/
u32 fetch_long(x86emu_t *emu)
{
  u32 val;
  unsigned err;

  err = decode_memio(emu, emu->x86.R_CS_BASE + emu->x86.R_EIP, &val, X86EMU_MEMIO_32 + X86EMU_MEMIO_X);

  if(err) x86emu_stop(emu);

  if(MODE_CODE32) {
    emu->x86.R_EIP += 4;
  }
  else {
    emu->x86.R_IP += 4;
  }

  if(emu->x86.instr_len + 3 < sizeof emu->x86.instr_buf) {
    emu->x86.instr_buf[emu->x86.instr_len++] = val;
    emu->x86.instr_buf[emu->x86.instr_len++] = val >> 8;
    emu->x86.instr_buf[emu->x86.instr_len++] = val >> 16;
    emu->x86.instr_buf[emu->x86.instr_len++] = val >> 24;
  }

  return val;
}


/****************************************************************************
RETURNS:
Value of the default data segment

REMARKS:
Inline function that returns the default data segment for the current
instruction.

On the x86 processor, the default segment is not always DS if there is
no segment override. Address modes such as -3[BP] or 10[BP+SI] all refer to
addresses relative to SS (ie: on the stack). So, at the minimum, all
decodings of addressing modes would have to set/clear a bit describing
whether the access is relative to DS or SS.  That is the function of the
cpu-state-varible emu->x86.mode. There are several potential states:

	repe prefix seen  (handled elsewhere)
	repne prefix seen  (ditto)

	cs segment override
	ds segment override
	es segment override
	fs segment override
	gs segment override
	ss segment override

	ds/ss select (in absense of override)

Each of the above 7 items are handled with a bit in the mode field.
****************************************************************************/
static sel_t *get_data_segment(x86emu_t *emu)
{
  sel_t *seg;

  if(!(seg = emu->x86.default_seg)) {
    seg = emu->x86.seg + (emu->x86.mode & _MODE_SEG_DS_SS ? R_SS_INDEX : R_DS_INDEX);
  }

  return seg;
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.
****************************************************************************/
u8 fetch_data_byte(x86emu_t *emu, u32 ofs)
{
  return fetch_data_byte_abs(emu, get_data_segment(emu), ofs);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.
****************************************************************************/
u16 fetch_data_word(x86emu_t *emu, u32 ofs)
{
  return fetch_data_word_abs(emu, get_data_segment(emu), ofs);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.
****************************************************************************/
u32 fetch_data_long(x86emu_t *emu, u32 ofs)
{
  return fetch_data_long_abs(emu, get_data_segment(emu), ofs);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.
****************************************************************************/
u8 fetch_data_byte_abs(x86emu_t *emu, sel_t *seg, u32 ofs)
{
  u32 val;

  check_data_access(emu, seg, ofs, 1);

  decode_memio(emu, seg->base + ofs, &val, X86EMU_MEMIO_8 + X86EMU_MEMIO_R);

  return val;
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.
****************************************************************************/
u16 fetch_data_word_abs(x86emu_t *emu, sel_t *seg, u32 ofs)
{
  u32 val;

  check_data_access(emu, seg, ofs, 2);

  decode_memio(emu, seg->base + ofs, &val, X86EMU_MEMIO_16 + X86EMU_MEMIO_R);

  return val;
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.
****************************************************************************/
u32 fetch_data_long_abs(x86emu_t *emu, sel_t *seg, u32 ofs)
{
  u32 val;

//  check_data_access(emu, seg, ofs, 4);

  decode_memio(emu, seg->base + ofs, &val, X86EMU_MEMIO_32 + X86EMU_MEMIO_R);

  return val;
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.
****************************************************************************/
void store_data_byte(x86emu_t *emu, u32 ofs, u8 val)
{
  store_data_byte_abs(emu, get_data_segment(emu), ofs, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.
****************************************************************************/
void store_data_word(x86emu_t *emu, u32 ofs, u16 val)
{
  store_data_word_abs(emu, get_data_segment(emu), ofs, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.
****************************************************************************/
void store_data_long(x86emu_t *emu, u32 ofs, u32 val)
{
  store_data_long_abs(emu, get_data_segment(emu), ofs, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a byte value to an absolute memory location.
****************************************************************************/
void store_data_byte_abs(x86emu_t *emu, sel_t *seg, u32 ofs, u8 val)
{
  u32 val32 = val;

  check_data_access(emu, seg, ofs, 1);

  decode_memio(emu, seg->base + ofs, &val32, X86EMU_MEMIO_8 + X86EMU_MEMIO_W);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an absolute memory location.
****************************************************************************/
void store_data_word_abs(x86emu_t *emu, sel_t *seg, u32 ofs, u16 val)
{
  u32 val32 = val;

  check_data_access(emu, seg, ofs, 2);

  decode_memio(emu, seg->base + ofs, &val32, X86EMU_MEMIO_16 + X86EMU_MEMIO_W);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an absolute memory location.
****************************************************************************/
void store_data_long_abs(x86emu_t *emu, sel_t *seg, u32 ofs, u32 val)
{
//  check_data_access(emu, seg, ofs, 4);

  decode_memio(emu, seg->base + ofs, &val, X86EMU_MEMIO_32 + X86EMU_MEMIO_W);
}


u8 fetch_io_byte(x86emu_t *emu, u32 port)
{
  u32 val;

  decode_memio(emu, port, &val, X86EMU_MEMIO_8 + X86EMU_MEMIO_I);

  return val;
}


u16 fetch_io_word(x86emu_t *emu, u32 port)
{
  u32 val;

  decode_memio(emu, port, &val, X86EMU_MEMIO_16 + X86EMU_MEMIO_I);

  return val;
}


u32 fetch_io_long(x86emu_t *emu, u32 port)
{
  u32 val;

  decode_memio(emu, port, &val, X86EMU_MEMIO_32 + X86EMU_MEMIO_I);

  return val;
}


void store_io_byte(x86emu_t *emu, u32 port, u8 val)
{
  u32 val32 = val;

  decode_memio(emu, port, &val32, X86EMU_MEMIO_8 + X86EMU_MEMIO_O);
}


void store_io_word(x86emu_t *emu, u32 port, u16 val)
{
  u32 val32 = val;

  decode_memio(emu, port, &val32, X86EMU_MEMIO_16 + X86EMU_MEMIO_O);
}


void store_io_long(x86emu_t *emu, u32 port, u32 val)
{
  decode_memio(emu, port, &val, X86EMU_MEMIO_32 + X86EMU_MEMIO_O);
}


/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for byte operands. Also enables the decoding of instructions.
****************************************************************************/
u8* decode_rm_byte_register(x86emu_t *emu, int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("al");
      return &emu->x86.R_AL;

    case 1:
      OP_DECODE("cl");
      return &emu->x86.R_CL;

    case 2:
      OP_DECODE("dl");
      return &emu->x86.R_DL;

    case 3:
      OP_DECODE("bl");
      return &emu->x86.R_BL;

    case 4:
      OP_DECODE("ah");
      return &emu->x86.R_AH;

    case 5:
      OP_DECODE("ch");
      return &emu->x86.R_CH;

    case 6:
      OP_DECODE("dh");
      return &emu->x86.R_DH;

    case 7:
      OP_DECODE("bh");
      return &emu->x86.R_BH;
  }

  return NULL;                /* NOT REACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for word operands.  Also enables the decoding of instructions.
****************************************************************************/
u16* decode_rm_word_register(x86emu_t *emu, int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("ax");
      return &emu->x86.R_AX;

    case 1:
      OP_DECODE("cx");
      return &emu->x86.R_CX;

    case 2:
      OP_DECODE("dx");
      return &emu->x86.R_DX;

    case 3:
      OP_DECODE("bx");
      return &emu->x86.R_BX;

    case 4:
      OP_DECODE("sp");
      return &emu->x86.R_SP;

    case 5:
      OP_DECODE("bp");
      return &emu->x86.R_BP;

    case 6:
      OP_DECODE("si");
      return &emu->x86.R_SI;

    case 7:
      OP_DECODE("di");
      return &emu->x86.R_DI;
  }

  return NULL;                /* NOTREACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for dword operands.  Also enables the decoding of instructions.
****************************************************************************/
u32* decode_rm_long_register(x86emu_t *emu, int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("eax");
      return &emu->x86.R_EAX;

    case 1:
      OP_DECODE("ecx");
      return &emu->x86.R_ECX;

    case 2:
      OP_DECODE("edx");
      return &emu->x86.R_EDX;

    case 3:
      OP_DECODE("ebx");
      return &emu->x86.R_EBX;

    case 4:
      OP_DECODE("esp");
      return &emu->x86.R_ESP;

    case 5:
      OP_DECODE("ebp");
      return &emu->x86.R_EBP;

    case 6:
      OP_DECODE("esi");
      return &emu->x86.R_ESI;

    case 7:
      OP_DECODE("edi");
      return &emu->x86.R_EDI;
  }

  return NULL;                /* NOTREACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for word operands, modified from above for the weirdo
special case of segreg operands.  Also enables the decoding of instructions.
****************************************************************************/
sel_t *decode_rm_seg_register(x86emu_t *emu, int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("es");
      break;

    case 1:
      OP_DECODE("cs");
      break;

    case 2:
      OP_DECODE("ss");
      break;

    case 3:
      OP_DECODE("ds");
      break;

    case 4:
      OP_DECODE("fs");
      break;

    case 5:
      OP_DECODE("gs");
      break;

    default:
      INTR_RAISE_UD(emu);
      reg = 6;
      break;
  }

  return emu->x86.seg + reg;
}


void decode_hex(x86emu_t *emu, char **p, u32 ofs)
{
  unsigned u;
  static const char *h = "0123456789abcdef";

  if(ofs) {
    u = 8;
    while(!(ofs & 0xf0000000)) ofs <<= 4, u--;
    for(; u ; ofs <<= 4, u--) {
      *(*p)++ = h[(ofs >> 28) & 0xf];
    }
  }
  else {
    *(*p)++ = '0';
  }
}


void decode_hex1(x86emu_t *emu, char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  (*p)++;

  *s = h[ofs & 0xf];
}


void decode_hex2(x86emu_t *emu, char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 2;

  s[1] = h[ofs & 0xf];
  ofs >>= 4;
  s[0] = h[ofs & 0xf];
}


void decode_hex4(x86emu_t *emu, char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 4;

  s[3] = h[ofs & 0xf];
  ofs >>= 4;
  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
  ofs >>= 4;
  s[0] = h[ofs & 0xf];
}


void decode_hex8(x86emu_t *emu, char **p, u32 ofs)
{
  decode_hex4(emu, p, ofs >> 16);
  decode_hex4(emu, p, ofs & 0xffff);
}


void decode_hex_addr(x86emu_t *emu, char **p, u32 ofs)
{
  if(MODE_CODE32) {
    decode_hex4(emu, p, ofs >> 16);
    decode_hex4(emu, p, ofs & 0xffff);
  }
  else {
    decode_hex4(emu, p, ofs & 0xffff);
  }
}


void decode_hex2s(x86emu_t *emu, char **p, s32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 3;

  if(ofs >= 0) {
    s[0] = '+';
  }
  else {
    s[0] = '-';
    ofs = -ofs;
  }

  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
}


void decode_hex4s(x86emu_t *emu, char **p, s32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 5;

  if(ofs >= 0) {
    s[0] = '+';
  }
  else {
    s[0] = '-';
    ofs = -ofs;
  }

  s[4] = h[ofs & 0xf];
  ofs >>= 4;
  s[3] = h[ofs & 0xf];
  ofs >>= 4;
  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
}


void decode_hex8s(x86emu_t *emu, char **p, s32 ofs)
{
  if(ofs >= 0) {
    *(*p)++ = '+';
  }
  else {
    *(*p)++ = '-';
    ofs = -ofs;
  }

  decode_hex8(emu, p, ofs);
}


/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=00 addressing.  Also enables the
decoding of instructions.

NOTE: 	The code which specifies the corresponding segment (ds vs ss)
		below in the case of [BP+..].  The assumption here is that at the
		point that this subroutine is called, the bit corresponding to
		_MODE_SEG_DS_SS will be zero.  After every instruction
		except the segment override instructions, this bit (as well
		as any bits indicating segment overrides) will be clear.  So
		if a SS access is needed, set this bit.  Otherwise, DS access
		occurs (unless any of the segment override bits are set).
****************************************************************************/
u32 decode_rm_address(x86emu_t *emu, int mod, int rl)
{
  switch(mod) {
    case 0:
      return decode_rm00_address(emu, rl);
      break;

    case 1:
      return decode_rm01_address(emu, rl);
      break;

    case 2:
      return decode_rm10_address(emu, rl);
      break;

    default:
      INTR_RAISE_UD(emu);
      break;
  }

  return 0;
}


u32 decode_rm00_address(x86emu_t *emu, int rm)
{
  u32 offset, base;
  int sib;

  if(MODE_ADDR32) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax]");
        return emu->x86.R_EAX;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx]");
        return emu->x86.R_ECX;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx]");
        return emu->x86.R_EDX;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx]");
        return emu->x86.R_EBX;

      case 4:
        sib = fetch_byte(emu);
        base = decode_sib_address(emu, sib, 0);
        OP_DECODE("]");
        return base;

      case 5:
        offset = fetch_long(emu);
        SEGPREF_DECODE;
        DECODE_HEX8(offset);
        OP_DECODE("]");
        return offset;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi]");
        return emu->x86.R_ESI;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi]");
        return emu->x86.R_EDI;
    }
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si]");
        return (emu->x86.R_BX + emu->x86.R_SI) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di]");
        return (emu->x86.R_BX + emu->x86.R_DI) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + emu->x86.R_SI) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + emu->x86.R_DI) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si]");
        return emu->x86.R_SI;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di]");
        return emu->x86.R_DI;

      case 6:
        offset = fetch_word(emu);
        SEGPREF_DECODE;
        DECODE_HEX4(offset);
        OP_DECODE("]");
        return offset;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx]");
        return emu->x86.R_BX;
      }
  }

  return 0;
}


u32 decode_rm01_address(x86emu_t *emu, int rm)
{
  s32 displacement = 0;
  u32 base;
  int sib;

  /* Fetch disp8 if no SIB byte */
  if(!(MODE_ADDR32 && (rm == 4))) {
    displacement = (s8) fetch_byte(emu);
  }

  if(MODE_ADDR32) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EAX + displacement;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_ECX + displacement;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EDX + displacement;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EBX + displacement;

      case 4:
        sib = fetch_byte(emu);
        base = decode_sib_address(emu, sib, 1);
        displacement = (s8) fetch_byte(emu);
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return base + displacement;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("ebp");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EBP + displacement;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_ESI + displacement;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EDI + displacement;
    }
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_BX + emu->x86.R_SI + displacement) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_BX + emu->x86.R_DI + displacement) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + emu->x86.R_SI + displacement) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + emu->x86.R_DI + displacement) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_SI + displacement) & 0xffff;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_DI + displacement) & 0xffff;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("bp");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + displacement) & 0xffff;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_BX + displacement) & 0xffff;
      }
  }

  return 0;
}


u32 decode_rm10_address(x86emu_t *emu, int rm)
{
  s32 displacement = 0;
  u32 base;
  int sib;

  /* Fetch disp16 if 16-bit addr mode */
  if(!MODE_ADDR32) {
    displacement = (s16) fetch_word(emu);
  }
  else {
    /* Fetch disp32 if no SIB byte */
    if(rm != 4) displacement = (s32) fetch_long(emu);
  }

  if(MODE_ADDR32) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EAX + displacement;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return emu->x86.R_ECX + displacement;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EDX + displacement;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EBX + displacement;

      case 4:
        sib = fetch_byte(emu);
        base = decode_sib_address(emu, sib, 2);
        displacement = (s32) fetch_long(emu);
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return base + displacement;
        break;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("ebp");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return emu->x86.R_EBP + displacement;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return emu->x86.R_ESI + displacement;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return emu->x86.R_EDI + displacement;
    }
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_BX + emu->x86.R_SI + displacement) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_BX + emu->x86.R_DI + displacement) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + emu->x86.R_SI + displacement) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + emu->x86.R_DI + displacement) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_SI + displacement) & 0xffff;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_DI + displacement) & 0xffff;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("bp");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        emu->x86.mode |= _MODE_SEG_DS_SS;
        return (emu->x86.R_BP + displacement) & 0xffff;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (emu->x86.R_BX + displacement) & 0xffff;
    }
  }

  return 0;
}


/*
 *
 * return offset from the SIB Byte
 */
u32 decode_sib_address(x86emu_t *emu, int sib, int mod)
{
  u32 base = 0, i = 0, scale = 1;

  /* sib base */
  switch(sib & 0x07) {
    case 0:
      SEGPREF_DECODE;
      OP_DECODE("eax");
      base = emu->x86.R_EAX;
      break;

    case 1:
      SEGPREF_DECODE;
      OP_DECODE("ecx");
      base = emu->x86.R_ECX;
      break;

    case 2:
      SEGPREF_DECODE;
      OP_DECODE("edx");
      base = emu->x86.R_EDX;
      break;

    case 3:
      SEGPREF_DECODE;
      OP_DECODE("ebx");
      base = emu->x86.R_EBX;
      break;

    case 4:
      SEGPREF_DECODE;
      OP_DECODE("esp");
      base = emu->x86.R_ESP;
      emu->x86.mode |= _MODE_SEG_DS_SS;
      break;

    case 5:
      SEGPREF_DECODE;
      if(mod == 0) {
        base = fetch_long(emu);
        DECODE_HEX8(base);
      }
      else {
        OP_DECODE("ebp");
        base = emu->x86.R_EBP;
        emu->x86.mode |= _MODE_SEG_DS_SS;
      }
      break;

    case 6:
      SEGPREF_DECODE;
      OP_DECODE("esi");
      base = emu->x86.R_ESI;
      break;

    case 7:
      SEGPREF_DECODE;
      OP_DECODE("edi");
      base = emu->x86.R_EDI;
      break;
  }

  /* sib index */
  switch((sib >> 3) & 0x07) {
    case 0:
      OP_DECODE("+eax");
      i = emu->x86.R_EAX;
      break;

    case 1:
      OP_DECODE("+ecx");
      i = emu->x86.R_ECX;
      break;

    case 2:
      OP_DECODE("+edx");
      i = emu->x86.R_EDX;
      break;

    case 3:
      OP_DECODE("+ebx");
      i = emu->x86.R_EBX;
      break;

    case 4:
      i = 0;
      break;

    case 5:
      OP_DECODE("+ebp");
      i = emu->x86.R_EBP;
      break;

    case 6:
      OP_DECODE("+esi");
      i = emu->x86.R_ESI;
      break;

    case 7:
      OP_DECODE("+edi");
      i = emu->x86.R_EDI;
      break;
  }

  scale = (sib >> 6) & 0x03;

  if(((sib >> 3) & 0x07) != 4) {
    if(scale) {
      OP_DECODE("*");
      *emu->x86.disasm_ptr++ = '0' + (1 << scale);
    }
  }

  return base + (i << scale);
}


void log_code(x86emu_t *emu)
{
  unsigned u, lf;
  char **p = &emu->log.ptr;

  if(!(emu->log.trace & X86EMU_TRACE_CODE) || !*p) return;
  lf = LOG_FREE(emu);
  if(lf < 512) lf = x86emu_clear_log(emu, 1);
  if(lf < 512) return;

  decode_hex(emu, p, emu->x86.R_TSC);

#if WITH_TSC
  if(emu->log.trace & X86EMU_TRACE_TIME) {
    LOG_STR(" +");
    decode_hex(emu, p, emu->x86.R_REAL_TSC - emu->x86.R_LAST_REAL_TSC);
  }
#endif
  LOG_STR(" ");
  decode_hex4(emu, p, emu->x86.saved_cs);
  LOG_STR(":");
  MODE_CODE32 ? decode_hex8(emu, p, emu->x86.saved_eip) : decode_hex4(emu, p, emu->x86.saved_eip);
  LOG_STR(" ");

  for(u = 0; u < emu->x86.instr_len; u++) {
    decode_hex2(emu, p, emu->x86.instr_buf[u]);
  }

  while(u++ < 12) {
    LOG_STR("  ");
  }

  LOG_STR(" ");

  u = emu->x86.disasm_ptr - emu->x86.disasm_buf;
  memcpy(*p, emu->x86.disasm_buf, u);
  *p += u;

  LOG_STR("\n");

  **p = 0;
}


void log_regs(x86emu_t *emu)
{
  char **p = &emu->log.ptr;
  unsigned lf;

  if(!(emu->log.trace & X86EMU_TRACE_REGS) || !*p) return;
  lf = LOG_FREE(emu);
  if(lf < 512) lf = x86emu_clear_log(emu, 1);
  if(lf < 512) return;

  LOG_STR("\neax ");
  decode_hex8(emu, p, emu->x86.R_EAX);
  LOG_STR(", ebx ");
  decode_hex8(emu, p, emu->x86.R_EBX);
  LOG_STR(", ecx ");
  decode_hex8(emu, p, emu->x86.R_ECX);
  LOG_STR(", edx ");
  decode_hex8(emu, p, emu->x86.R_EDX);

  LOG_STR("\nesi ");
  decode_hex8(emu, p, emu->x86.R_ESI);
  LOG_STR(", edi ");
  decode_hex8(emu, p, emu->x86.R_EDI);
  LOG_STR(", ebp ");
  decode_hex8(emu, p, emu->x86.R_EBP);
  LOG_STR(", esp ");
  decode_hex8(emu, p, emu->x86.R_ESP);

  LOG_STR("\ncs ");
  decode_hex4(emu, p, emu->x86.R_CS);
  LOG_STR(", ss ");
  decode_hex4(emu, p, emu->x86.R_SS);
  LOG_STR(", ds ");
  decode_hex4(emu, p, emu->x86.R_DS);
  LOG_STR(", es ");
  decode_hex4(emu, p, emu->x86.R_ES);
  LOG_STR(", fs ");
  decode_hex4(emu, p, emu->x86.R_FS);
  LOG_STR(", gs ");
  decode_hex4(emu, p, emu->x86.R_GS);

  LOG_STR("\neip ");
  decode_hex8(emu, p, emu->x86.R_EIP);
  LOG_STR(", eflags ");
  decode_hex8(emu, p, emu->x86.R_EFLG);

  if(ACCESS_FLAG(F_OF)) LOG_STR(" of");
  if(ACCESS_FLAG(F_DF)) LOG_STR(" df");
  if(ACCESS_FLAG(F_IF)) LOG_STR(" if");
  if(ACCESS_FLAG(F_SF)) LOG_STR(" sf");
  if(ACCESS_FLAG(F_ZF)) LOG_STR(" zf");
  if(ACCESS_FLAG(F_AF)) LOG_STR(" af");
  if(ACCESS_FLAG(F_PF)) LOG_STR(" pf");
  if(ACCESS_FLAG(F_CF)) LOG_STR(" cf");

  LOG_STR("\n");

  **p = 0;
}


void check_data_access(x86emu_t *emu, sel_t *seg, u32 ofs, u32 size)
{
  char **p = &emu->log.ptr;
  static char seg_name[7] = "ecsdfg?";
  unsigned idx = seg - emu->x86.seg, lf;

  if((emu->log.trace & X86EMU_TRACE_ACC) && *p) {
    lf = LOG_FREE(emu);
    if(lf < 512) lf = x86emu_clear_log(emu, 1);
    if(lf >= 512) {
      LOG_STR("a [");
      switch(size) {
        case 1:
          LOG_STR("byte ");
          break;
        case 2:
          LOG_STR("word ");
          break;
        case 4:
          LOG_STR("dword ");
          break;
      }
      if(idx > 6) idx = 6;
      *(*p)++ = seg_name[idx];
      LOG_STR("s:");
      decode_hex8(emu, p, ofs);
      LOG_STR("]\n");

      **p = 0;
    }
  }

  if(ofs + size - 1 > seg->limit) {
    INTR_RAISE_GP(emu, seg->sel);
  }

  return;
}


void decode_descriptor(x86emu_t *emu, descr_t *d, u32 dl, u32 dh)
{
  char **p = &emu->log.ptr;
  unsigned lf, acc;

  memset(d, 0, sizeof *d);

  d->acc = acc = ((dh >> 8) & 0xff) + ((dh >> 12) & 0xf00);
  d->base = ((dl >> 16) & 0xffff) + ((dh & 0xff) << 16) + (dh & 0xff000000);
  d->limit = (dl & 0xffff) + (dh & 0xf0000);
  if(ACC_G(acc)) d->limit = (d->limit << 12) + 0xfff;
  d->g = ACC_G(acc);
  d->p = ACC_P(acc);
  d->dpl = ACC_DPL(acc);

  if(ACC_S(acc)) {
    d->seg = 1;
    d->is_i386 = ACC_D(acc);
    d->a = ACC_A(acc);
    if(ACC_E(acc)) {
      // code
      d->x = 1;
      d->c = ACC_C(acc);
      d->r = ACC_R(acc);
    }
    else {
      // data
      d->r = 1;
      d->ed = ACC_ED(acc);
      d->w = ACC_W(acc);
    }
  }
  else {
    if(acc & 8) d->is_i386 = 1;
    switch(acc & 7) {
      case 0:
        d->invalid = 1;
        break;

      case 3:	// tss busy
        d->busy = 1;

      case 1:	// tss avail
        d->tss = 1;
        break;

      case 2:
        d->ldt = 1;
        break;

      case 4:
        d->c_gate = 1;
        break;

      case 5:
        d->t_gate = 1;
        break;

      case 7:
        d->trap = 1;

      case 6:
        d->i_gate = 1;
        break;
    }

    if(d->c_gate || d->i_gate || d->t_gate) {
      d->offset = (dl & 0xffff) + (dh & 0xffff0000);
      d->sel = dl >> 16;
      d->w_count = dh & 0x1f;
    }
  }

  if((emu->log.trace & X86EMU_TRACE_ACC) && *p) {
    lf = LOG_FREE(emu);
    if(lf < 512) lf = x86emu_clear_log(emu, 1);
    if(lf >= 512) {
      LOG_STR("d [");
      decode_hex8(emu, p, dh);
      LOG_STR(" ");
      decode_hex8(emu, p, dl);
      LOG_STR("] =");
      if(d->seg) {
        LOG_STR(" base=");
        decode_hex8(emu, p, d->base);
        LOG_STR(" limit=");
        decode_hex8(emu, p, d->limit);
      }
      else {
        LOG_STR(" sel=");
        decode_hex4(emu, p, d->sel);
        LOG_STR(" ofs=");
        decode_hex8(emu, p, d->offset);
        LOG_STR(" wcnt=");
        decode_hex2(emu, p, d->w_count);
      }
      LOG_STR(" dpl=");
      decode_hex1(emu, p, d->dpl);
      if(d->p) LOG_STR(" p");
      if(d->a) LOG_STR(" a");
      if(d->r) LOG_STR(" r");
      if(d->w) LOG_STR(" w");
      if(d->x) LOG_STR(" x");
      if(d->c) LOG_STR(" c");
      if(d->ed) LOG_STR(" ed");
      if(d->g) LOG_STR(" g");
      if(d->is_i386) LOG_STR(" 32");
      if(d->ldt) LOG_STR(" ldt");
      if(d->tss) LOG_STR(" tss");
      if(d->busy) LOG_STR(" busy");
      if(d->c_gate) LOG_STR(" callgate");
      if(d->i_gate) LOG_STR(" intgate");
      if(d->t_gate) LOG_STR(" taskgate");
      if(d->trap) LOG_STR(" trap");
      if(d->invalid) LOG_STR(" invalid");
      LOG_STR("\n");

      **p = 0;
    }
  }
}


API_SYM void x86emu_set_seg_register(x86emu_t *emu, sel_t *seg, u16 val)
{
  int err = 1;
  unsigned ofs;
  u32 dl, dh, dt_base, dt_limit;
  descr_t d;

  if(MODE_REAL(emu)) {
    seg->sel = val;
    seg->base = val << 4;

    err = 0;
  }
  else {
    ofs = val & ~7;

    if(val & 4) {
      dt_base = emu->x86.R_LDT_BASE;
      dt_limit = emu->x86.R_LDT_LIMIT;
    }
    else {
      dt_base = emu->x86.R_GDT_BASE;
      dt_limit = emu->x86.R_GDT_LIMIT;
    }

    if(ofs == 0) {
      seg->sel = 0;
      seg->base = 0;
      seg->limit = 0;
      seg->acc = 0;

      err = 0;
    }
    else if(ofs + 7 <= dt_limit) {
      err =
        emu_memio(emu, dt_base + ofs, &dl, X86EMU_MEMIO_32 + X86EMU_MEMIO_R) |
        emu_memio(emu, dt_base + ofs + 4, &dh, X86EMU_MEMIO_32 + X86EMU_MEMIO_R);

      if(!err) {
        decode_descriptor(emu, &d, dl, dh);
        if(!d.invalid && d.p && d.seg) {
          seg->sel = val;
          seg->base = d.base;
          seg->limit = d.limit;
          seg->acc = d.acc;
        }
        else {
          err = 1;
        }
      }
    }
  }

  if(err) INTR_RAISE_GP(emu, val);
}


void idt_lookup(x86emu_t *emu, u8 nr, u32 *new_cs, u32 *new_eip)
{
  unsigned err, ofs;
  u32 dl, dh;
  descr_t d1;

  if(MODE_REAL(emu)) {
    ofs = nr << 2;
    err =
      decode_memio(emu, emu->x86.R_IDT_BASE + ofs, new_eip, X86EMU_MEMIO_16 + X86EMU_MEMIO_R) |
      decode_memio(emu, emu->x86.R_IDT_BASE + ofs + 2, new_cs, X86EMU_MEMIO_16 + X86EMU_MEMIO_R);
  }
  else {
    ofs = nr << 3;
    if(ofs + 7 <= emu->x86.R_IDT_LIMIT) {
      err =
        decode_memio(emu, emu->x86.R_IDT_BASE + ofs, &dl, X86EMU_MEMIO_32 + X86EMU_MEMIO_R) |
        decode_memio(emu, emu->x86.R_IDT_BASE + ofs + 4, &dh, X86EMU_MEMIO_32 + X86EMU_MEMIO_R);
    }
    else {
      err = 1;
    }
    if(!err) {
      decode_descriptor(emu, &d1, dl, dh);
      if(!d1.invalid && d1.p && d1.i_gate) {
        *new_cs = d1.sel;
        *new_eip = d1.offset;
      }
    }
  }
}


void generate_int(x86emu_t *emu, u8 nr, unsigned type, unsigned errcode)
{
  u32 cs, eip, new_cs, new_eip;
  int i;

  emu->x86.intr_stats[nr]++;

  i = emu->intr ? (*emu->intr)(emu, nr, type) : 0;

  if(!i) {
    if(type & INTR_MODE_RESTART) {
      eip = emu->x86.saved_eip;
      cs = emu->x86.saved_cs;
    }
    else {
      eip = emu->x86.R_EIP;
      cs = emu->x86.R_CS;
    }

    new_cs = cs;
    new_eip = eip;

    idt_lookup(emu, nr, &new_cs, &new_eip);

    if(MODE_PROTECTED(emu) && MODE_CODE32) {
      push_long(emu, emu->x86.R_EFLG);
      push_long(emu, cs);
      push_long(emu, eip);
    }
    else {
      push_word(emu, emu->x86.R_FLG);
      push_word(emu, cs);
      push_word(emu, eip);
    }

    if(type & INTR_MODE_ERRCODE) push_long(emu, errcode);

    CLEAR_FLAG(F_IF);
    CLEAR_FLAG(F_TF);

    x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, new_cs);
    emu->x86.R_EIP = new_eip;
  }
}


unsigned decode_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type)
{
  unsigned err, bits = type & 0xff, lf;
  char **p = &emu->log.ptr;

  err = emu->memio(emu, addr, val, type);

  type &= ~0xff;

  if(!*p || !((emu->log.trace & X86EMU_TRACE_DATA) || (emu->log.trace & X86EMU_TRACE_IO))) return err;

  if(
    !((emu->log.trace & X86EMU_TRACE_IO) && (type == X86EMU_MEMIO_I || type == X86EMU_MEMIO_O)) &&
    !((emu->log.trace & X86EMU_TRACE_DATA) && (type == X86EMU_MEMIO_R || type == X86EMU_MEMIO_W || type == X86EMU_MEMIO_X))
  ) return err;

  lf = LOG_FREE(emu);
  if(lf < 1024) lf = x86emu_clear_log(emu, 1);
  if(lf < 1024) return err;

  switch(type) {
    case X86EMU_MEMIO_R:
      LOG_STR("r [");
      break;
    case X86EMU_MEMIO_W:
      LOG_STR("w [");
      break;
    case X86EMU_MEMIO_X:
      LOG_STR("x [");
      break;
    case X86EMU_MEMIO_I:
      LOG_STR("i [");
      break;
    case X86EMU_MEMIO_O:
      LOG_STR("o [");
      break;
  }

  decode_hex8(emu, p, addr);

  LOG_STR("] = ");

  switch(bits) {
    case X86EMU_MEMIO_8:
      if(err) {
        LOG_STR("??");
      }
      else {
        decode_hex2(emu, p, *val);
      }
      break;
    case X86EMU_MEMIO_16:
      if(err) {
        LOG_STR("????");
      }
      else {
        decode_hex4(emu, p, *val);
      }
      break;
    case X86EMU_MEMIO_32:
      if(err) {
        LOG_STR("????????");
      }
      else {
        decode_hex8(emu, p, *val);
      }
      break;
  }

  LOG_STR("\n");
  **p = 0;

  return err;
}


unsigned emu_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type)
{
  unsigned err, bits = type & 0xff, lf;
  char **p = &emu->log.ptr;

  err = emu->memio(emu, addr, val, type);

  type &= ~0xff;

  if(!*p || !((emu->log.trace & X86EMU_TRACE_DATA) || (emu->log.trace & X86EMU_TRACE_IO))) return err;

  if(
    !((emu->log.trace & X86EMU_TRACE_IO) && (type == X86EMU_MEMIO_I || type == X86EMU_MEMIO_O)) &&
    !((emu->log.trace & X86EMU_TRACE_DATA) && (type == X86EMU_MEMIO_R || type == X86EMU_MEMIO_W || type == X86EMU_MEMIO_X))
  ) return err;

  lf = LOG_FREE(emu);
  if(lf < 1024) lf = x86emu_clear_log(emu, 1);
  if(lf < 1024) return err;

  switch(type) {
    case X86EMU_MEMIO_R:
      LOG_STR("r [");
      break;
    case X86EMU_MEMIO_W:
      LOG_STR("w [");
      break;
    case X86EMU_MEMIO_X:
      LOG_STR("x [");
      break;
    case X86EMU_MEMIO_I:
      LOG_STR("i [");
      break;
    case X86EMU_MEMIO_O:
      LOG_STR("o [");
      break;
  }

  decode_hex8(emu, p, addr);

  LOG_STR("] = ");

  switch(bits) {
    case X86EMU_MEMIO_8:
      if(err) {
        LOG_STR("??");
      }
      else {
        decode_hex2(emu, p, *val);
      }
      break;
    case X86EMU_MEMIO_16:
      if(err) {
        LOG_STR("????");
      }
      else {
        decode_hex4(emu, p, *val);
      }
      break;
    case X86EMU_MEMIO_32:
      if(err) {
        LOG_STR("????????");
      }
      else {
        decode_hex8(emu, p, *val);
      }
      break;
  }

  LOG_STR("\n");
  **p = 0;

  return err;
}


void emu_process_debug(x86emu_t *emu, unsigned start, unsigned len)
{
  unsigned lf, type, u;
  char **p = &emu->log.ptr;

  if(!*p) return;

  lf = LOG_FREE(emu);
  if(lf < 1024) lf = x86emu_clear_log(emu, 1);
  if(lf < 1024) return;

  type = x86emu_read_byte_noperm(emu, start++);
  len--;

  switch(type) {
    case 1:
      LOG_STR("\n");
      while(len--) {
        *(*p)++ = x86emu_read_byte_noperm(emu, start++);
      }
      LOG_STR("\n");
      break;

    case 2:
      u = x86emu_read_byte_noperm(emu, start++);
      u += x86emu_read_byte_noperm(emu, start++) << 8;
      u += x86emu_read_byte_noperm(emu, start++) << 16;
      u += x86emu_read_byte_noperm(emu, start++) << 24;
      emu->log.trace |= u;
      break;

    case 3:
      u = x86emu_read_byte_noperm(emu, start++);
      u += x86emu_read_byte_noperm(emu, start++) << 8;
      u += x86emu_read_byte_noperm(emu, start++) << 16;
      u += x86emu_read_byte_noperm(emu, start++) << 24;
      emu->log.trace &= ~u;
      break;

    case 4:
      u = x86emu_read_byte_noperm(emu, start++);
      u += x86emu_read_byte_noperm(emu, start++) << 8;
      u += x86emu_read_byte_noperm(emu, start++) << 16;
      u += x86emu_read_byte_noperm(emu, start++) << 24;
      x86emu_dump(emu, u);
      break;

    case 5:
      x86emu_reset_access_stats(emu);
      break;
  }

  **p = 0;
}


