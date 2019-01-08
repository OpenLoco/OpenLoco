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
*   Header file for primitive operation functions.
*
****************************************************************************/


#ifndef __X86EMU_PRIM_OPS_H
#define __X86EMU_PRIM_OPS_H

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

u16     aaa_word (x86emu_t *emu, u16 d);
u16     aas_word (x86emu_t *emu, u16 d);
u16     aad_word (x86emu_t *emu, u16 d, u8 base);
u16     aam_word (x86emu_t *emu, u8 d, u8 base);
u8      adc_byte (x86emu_t *emu, u8 d, u8 s);
u16     adc_word (x86emu_t *emu, u16 d, u16 s);
u32     adc_long (x86emu_t *emu, u32 d, u32 s);
u8      add_byte (x86emu_t *emu, u8 d, u8 s);
u16     add_word (x86emu_t *emu, u16 d, u16 s);
u32     add_long (x86emu_t *emu, u32 d, u32 s);
u8      and_byte (x86emu_t *emu, u8 d, u8 s);
u16     and_word (x86emu_t *emu, u16 d, u16 s);
u32     and_long (x86emu_t *emu, u32 d, u32 s);
u8      cmp_byte (x86emu_t *emu, u8 d, u8 s);
u16     cmp_word (x86emu_t *emu, u16 d, u16 s);
u32     cmp_long (x86emu_t *emu, u32 d, u32 s);
u8      daa_byte (x86emu_t *emu, u8 d);
u8      das_byte (x86emu_t *emu, u8 d);
u8      dec_byte (x86emu_t *emu, u8 d);
u16     dec_word (x86emu_t *emu, u16 d);
u32     dec_long (x86emu_t *emu, u32 d);
u8      inc_byte (x86emu_t *emu, u8 d);
u16     inc_word (x86emu_t *emu, u16 d);
u32     inc_long (x86emu_t *emu, u32 d);
u8      or_byte (x86emu_t *emu, u8 d, u8 s);
u16     or_word (x86emu_t *emu, u16 d, u16 s);
u32     or_long (x86emu_t *emu, u32 d, u32 s);
u8      neg_byte (x86emu_t *emu, u8 s);
u16     neg_word (x86emu_t *emu, u16 s);
u32     neg_long (x86emu_t *emu, u32 s);
u8      not_byte (x86emu_t *emu, u8 s);
u16     not_word (x86emu_t *emu, u16 s);
u32     not_long (x86emu_t *emu, u32 s);
u8      rcl_byte (x86emu_t *emu, u8 d, u8 s);
u16     rcl_word (x86emu_t *emu, u16 d, u8 s);
u32     rcl_long (x86emu_t *emu, u32 d, u8 s);
u8      rcr_byte (x86emu_t *emu, u8 d, u8 s);
u16     rcr_word (x86emu_t *emu, u16 d, u8 s);
u32     rcr_long (x86emu_t *emu, u32 d, u8 s);
u8      rol_byte (x86emu_t *emu, u8 d, u8 s);
u16     rol_word (x86emu_t *emu, u16 d, u8 s);
u32     rol_long (x86emu_t *emu, u32 d, u8 s);
u8      ror_byte (x86emu_t *emu, u8 d, u8 s);
u16     ror_word (x86emu_t *emu, u16 d, u8 s);
u32     ror_long (x86emu_t *emu, u32 d, u8 s);
u8      shl_byte (x86emu_t *emu, u8 d, u8 s);
u16     shl_word (x86emu_t *emu, u16 d, u8 s);
u32     shl_long (x86emu_t *emu, u32 d, u8 s);
u8      shr_byte (x86emu_t *emu, u8 d, u8 s);
u16     shr_word (x86emu_t *emu, u16 d, u8 s);
u32     shr_long (x86emu_t *emu, u32 d, u8 s);
u8      sar_byte (x86emu_t *emu, u8 d, u8 s);
u16     sar_word (x86emu_t *emu, u16 d, u8 s);
u32     sar_long (x86emu_t *emu, u32 d, u8 s);
u16     shld_word (x86emu_t *emu, u16 d, u16 fill, u8 s);
u32     shld_long (x86emu_t *emu, u32 d, u32 fill, u8 s);
u16     shrd_word (x86emu_t *emu, u16 d, u16 fill, u8 s);
u32     shrd_long (x86emu_t *emu, u32 d, u32 fill, u8 s);
u8      sbb_byte (x86emu_t *emu, u8 d, u8 s);
u16     sbb_word (x86emu_t *emu, u16 d, u16 s);
u32     sbb_long (x86emu_t *emu, u32 d, u32 s);
u8      sub_byte (x86emu_t *emu, u8 d, u8 s);
u16     sub_word (x86emu_t *emu, u16 d, u16 s);
u32     sub_long (x86emu_t *emu, u32 d, u32 s);
void    test_byte (x86emu_t *emu, u8 d, u8 s);
void    test_word (x86emu_t *emu, u16 d, u16 s);
void    test_long (x86emu_t *emu, u32 d, u32 s);
u8      xor_byte (x86emu_t *emu, u8 d, u8 s);
u16     xor_word (x86emu_t *emu, u16 d, u16 s);
u32     xor_long (x86emu_t *emu, u32 d, u32 s);
void    imul_byte (x86emu_t *emu, u8 s);
void    imul_word (x86emu_t *emu, u16 s);
void    imul_long (x86emu_t *emu, u32 s);
void 	imul_long_direct(u32 *res_lo, u32* res_hi, u32 d, u32 s);
void    mul_byte (x86emu_t *emu, u8 s);
void    mul_word (x86emu_t *emu, u16 s);
void    mul_long (x86emu_t *emu, u32 s);
void    idiv_byte (x86emu_t *emu, u8 s);
void    idiv_word (x86emu_t *emu, u16 s);
void    idiv_long (x86emu_t *emu, u32 s);
void    div_byte (x86emu_t *emu, u8 s);
void    div_word (x86emu_t *emu, u16 s);
void    div_long (x86emu_t *emu, u32 s);
void    ins (x86emu_t *emu, int size);
void    outs (x86emu_t *emu, int size);
void    push_word (x86emu_t *emu, u16 w);
void    push_long (x86emu_t *emu, u32 w);
u16     pop_word (x86emu_t *emu);
u32     pop_long (x86emu_t *emu);
int eval_condition(x86emu_t *emu, unsigned type);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_PRIM_OPS_H */
