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
*   Public interface functions.
*
****************************************************************************/


#include "include/x86emu_int.h"

#define LINE_LEN 16


API_SYM x86emu_t *x86emu_new(unsigned def_mem_perm, unsigned def_io_perm)
{
  x86emu_t *emu = calloc(1, sizeof *emu);

  emu->mem = emu_mem_new(def_mem_perm);

  emu->io.map =  calloc(X86EMU_IO_PORTS, sizeof *emu->io.map);
  emu->io.stats_i =  calloc(X86EMU_IO_PORTS, sizeof *emu->io.stats_i);
  emu->io.stats_o =  calloc(X86EMU_IO_PORTS, sizeof *emu->io.stats_o);

  if(def_io_perm) x86emu_set_io_perm(emu, 0, X86EMU_IO_PORTS - 1, def_io_perm);

  x86emu_set_memio_handler(emu, vm_memio);

  x86emu_reset(emu);

  return emu;
}


API_SYM x86emu_t *x86emu_done(x86emu_t *emu)
{
  if(emu) {
    emu_mem_free(emu->mem);

    free(emu->log.buf);

    free(emu->io.map);
    free(emu->io.stats_i);
    free(emu->io.stats_o);

    free(emu->x86.msr);
    free(emu->x86.msr_perm);

    free(emu);
  }

  return NULL;
}


API_SYM x86emu_t *x86emu_clone(x86emu_t *emu)
{
  x86emu_t *new_emu = NULL;

  if(!emu) return new_emu;

  new_emu = mem_dup(emu, sizeof *emu);

  new_emu->mem = emu_mem_clone(emu->mem);

  if(emu->log.buf && emu->log.ptr) {
    new_emu->log.buf = malloc(emu->log.size);
    // copy only used log space
    if(emu->log.ptr <= emu->log.buf + emu->log.size) {
      new_emu->log.ptr = new_emu->log.buf + (emu->log.ptr - emu->log.buf);
      memcpy(new_emu->log.buf, emu->log.buf, emu->log.ptr - emu->log.buf);
    }
  }

  new_emu->io.map = mem_dup(emu->io.map, X86EMU_IO_PORTS * sizeof *emu->io.map);
  new_emu->io.stats_i = mem_dup(emu->io.stats_i, X86EMU_IO_PORTS * sizeof *emu->io.stats_i);
  new_emu->io.stats_o = mem_dup(emu->io.stats_o, X86EMU_IO_PORTS * sizeof *emu->io.stats_o);
  new_emu->x86.msr = mem_dup(emu->x86.msr, X86EMU_MSRS * sizeof *emu->x86.msr);
  new_emu->x86.msr_perm = mem_dup(emu->x86.msr_perm, X86EMU_MSRS * sizeof *emu->x86.msr_perm);

  return new_emu;
}


API_SYM void x86emu_reset(x86emu_t *emu)
{
  x86emu_regs_t *x86 = &emu->x86;

  free(x86->msr);
  free(x86->msr_perm);

  memset(x86, 0, sizeof *x86);

  x86->R_EFLG = 2;

  x86->R_CS_LIMIT = x86->R_DS_LIMIT = x86->R_SS_LIMIT = x86->R_ES_LIMIT =
  x86->R_FS_LIMIT = x86->R_GS_LIMIT = 0xffff;

  // resp. 0x4093/9b for 4GB
  x86->R_CS_ACC = 0x9b;
  x86->R_SS_ACC = x86->R_DS_ACC = x86->R_ES_ACC = x86->R_FS_ACC = x86->R_GS_ACC = 0x93;

  x86->R_CS = 0xf000;
  x86->R_CS_BASE = 0xf0000;
  x86->R_EIP = 0xfff0;

  x86->R_GDT_LIMIT = 0xffff;
  x86->R_IDT_LIMIT = 0xffff;

  x86->msr = calloc(X86EMU_MSRS, sizeof *x86->msr);
  x86->msr_perm = calloc(X86EMU_MSRS, sizeof *x86->msr_perm);

  x86->msr_perm[0x10] = X86EMU_ACC_X;	// tsc
  x86->msr_perm[0x11] = X86EMU_ACC_X;	// last real tsc
  x86->msr_perm[0x12] = X86EMU_ACC_X;	// real tsc
}


API_SYM x86emu_memio_handler_t x86emu_set_memio_handler(x86emu_t *emu, x86emu_memio_handler_t handler)
{
  x86emu_memio_handler_t old = NULL;

  if(emu) {
    old = emu->memio;
    emu->memio = handler;
  }

  return old;
}


API_SYM x86emu_intr_handler_t x86emu_set_intr_handler(x86emu_t *emu, x86emu_intr_handler_t handler)
{
  x86emu_intr_handler_t old = NULL;

  if(emu) {
    old = emu->intr;
    emu->intr = handler;
  }

  return old;
}


API_SYM x86emu_code_handler_t x86emu_set_code_handler(x86emu_t *emu, x86emu_code_handler_t handler)
{
  x86emu_code_handler_t old = NULL;

  if(emu) {
    old = emu->code_check;
    emu->code_check = handler;
  }

  return old;
}


API_SYM unsigned x86emu_read_byte(x86emu_t *emu, unsigned addr)
{
  u32 val = 0xff;

  if(emu) emu->memio(emu, addr, &val, X86EMU_MEMIO_R | X86EMU_MEMIO_8);
 
  return val;
}


API_SYM unsigned x86emu_read_byte_noperm(x86emu_t *emu, unsigned addr)
{
  u32 val = 0xff;

  if(emu) emu->memio(emu, addr, &val, X86EMU_MEMIO_R | X86EMU_MEMIO_8_NOPERM);
 
  return val;
}


API_SYM unsigned x86emu_read_word(x86emu_t *emu, unsigned addr)
{
  u32 val = 0xffff;

  if(emu) emu->memio(emu, addr, &val, X86EMU_MEMIO_R | X86EMU_MEMIO_16);
 
  return val;
}


API_SYM unsigned x86emu_read_dword(x86emu_t *emu, unsigned addr)
{
  u32 val = 0xffffffff;

  if(emu) emu->memio(emu, addr, &val, X86EMU_MEMIO_R | X86EMU_MEMIO_32);
 
  return val;
}


API_SYM void x86emu_write_byte(x86emu_t *emu, unsigned addr, unsigned val)
{
  u32 val32 = val;

  if(emu) emu->memio(emu, addr, &val32, X86EMU_MEMIO_W | X86EMU_MEMIO_8);
}


API_SYM void x86emu_write_byte_noperm(x86emu_t *emu, unsigned addr, unsigned val)
{
  u32 val32 = val;

  if(emu) emu->memio(emu, addr, &val32, X86EMU_MEMIO_W | X86EMU_MEMIO_8_NOPERM);
}


API_SYM void x86emu_write_word(x86emu_t *emu, unsigned addr, unsigned val)
{
  u32 val32 = val;

  if(emu) emu->memio(emu, addr, &val32, X86EMU_MEMIO_W | X86EMU_MEMIO_16);
}


API_SYM void x86emu_write_dword(x86emu_t *emu, unsigned addr, unsigned val)
{
  u32 val32 = val;

  if(emu) emu->memio(emu, addr, &val32, X86EMU_MEMIO_W | X86EMU_MEMIO_32);
}


API_SYM void x86emu_set_log(x86emu_t *emu, unsigned buffer_size, x86emu_flush_func_t flush)
{
  if(emu) {
    if(emu->log.buf) free(emu->log.buf);
    emu->log.size = buffer_size;
    emu->log.buf = buffer_size ? calloc(1, buffer_size) : NULL;
    emu->log.ptr = emu->log.buf;
    emu->log.flush = flush;
  }
}


API_SYM unsigned x86emu_clear_log(x86emu_t *emu, int flush)
{
  if(flush && emu->log.flush) {
    if(emu->log.ptr && emu->log.ptr != emu->log.buf) {
      emu->log.flush(emu, emu->log.buf, emu->log.ptr - emu->log.buf);
    }
  }
  if((emu->log.ptr = emu->log.buf)) *emu->log.ptr = 0;

  return emu->log.ptr ? LOG_FREE(emu) : 0;
}


API_SYM void x86emu_log(x86emu_t *emu, const char *format, ...)
{
  va_list args;
  int size;

  if(!emu || !emu->log.ptr) return;

  size = emu->log.size - (emu->log.ptr - emu->log.buf);

  va_start(args, format);
  if(size > 0) {
    size = vsnprintf(emu->log.ptr, size, format, args);
    if(size > 0) {
      emu->log.ptr += size;
    }
    else {
      *emu->log.ptr = 0;
    }
  }
  va_end(args);  
}


/*
 * flags:
 *   bits 0-7:
 *     0: show all initialized memory
 *     1: show only accessed memory
 *     2: show only invalidly accessed memory
 *
 *   bit 8: show ascii, too
 */
static void dump_data(x86emu_t *emu, unsigned char *data, unsigned char *attr, char *str_data, char *str_attr, int flags)
{
  unsigned u, u1, flag_ascii;
  char c;
  int ok = 0;
  char *sd = str_data, *sa = str_attr;
  char *ascii = str_data + 4 * LINE_LEN + 2;

  flag_ascii = flags & 0x100;
  flags &= 0xff;

  for(u = 0; u < LINE_LEN; u++) {
    *str_data++ = (attr[u] & X86EMU_ACC_INVALID) ? '*' : ' ';
    if(
      (flags == 0 && (attr[u] & X86EMU_PERM_VALID)) ||
      (flags == 1 && (attr[u] & (X86EMU_ACC_R | X86EMU_ACC_W | X86EMU_ACC_X | X86EMU_ACC_INVALID))) ||
      (flags == 2 && (attr[u] & X86EMU_ACC_INVALID))
    ) {
      ok = 1;
      decode_hex2(emu, &str_data, u1 = data[u]);

      c = (attr[u] & X86EMU_PERM_R) ? (attr[u] & X86EMU_ACC_R) ? 'R' : 'r' : ' ';
      *str_attr++ = c;
      c = (attr[u] & X86EMU_PERM_W) ? (attr[u] & X86EMU_ACC_W) ? 'W' : 'w' : ' ';
      *str_attr++ = c;
      c = (attr[u] & X86EMU_PERM_X) ? (attr[u] & X86EMU_ACC_X) ? 'X' : 'x' : ' ';
      *str_attr++ = c;

      if(u1 < 0x20 || u1 >= 0x7f) u1 = '.';
      ascii[u] = u1;
    }
    else {
      *str_data++ = ' ';
      *str_data++ = ' ';

      *str_attr++ = ' ';
      *str_attr++ = ' ';
      *str_attr++ = ' ';

      ascii[u] = ' ';
    }
    *str_data++ = ' ';
    *str_attr++ = ' ';
  }

  if(ok) {
    if(flag_ascii) {
      str_data[0] = ' ';
      str_data[1] = ' ';
      str_data += 2 + LINE_LEN;
    }
  }
  else {
    str_data = sd;
    str_attr = sa;
  }

  *str_data = *str_attr = 0;

  while(str_data > sd && str_data[-1] == ' ') *--str_data = 0;
  while(str_attr > sa && str_attr[-1] == ' ') *--str_attr = 0;
}


API_SYM void x86emu_dump(x86emu_t *emu, int flags)
{
  x86emu_mem_t *mem = emu->mem;
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx, u, u1, u2, addr;
  char str_data[LINE_LEN * 8], str_attr[LINE_LEN * 8], fbuf[64];
  unsigned char def_data[LINE_LEN], def_attr[LINE_LEN];
  int dump_flags;

  if(
    mem &&
    mem->pdir &&
    (flags & (X86EMU_DUMP_MEM | X86EMU_DUMP_ACC_MEM | X86EMU_DUMP_INV_MEM | X86EMU_DUMP_ATTR))
  ) {
    x86emu_log(emu, "; - - memory\n");
    x86emu_log(emu, ";        ");
    for(u1 = 0; u1 < 16; u1++) x86emu_log(emu, "%4x", u1);
    x86emu_log(emu, "\n");

    dump_flags = 0;
    if(flags & X86EMU_DUMP_INV_MEM) dump_flags = 2;
    if(flags & X86EMU_DUMP_ACC_MEM) dump_flags = 1;
    if(flags & X86EMU_DUMP_MEM) dump_flags = 0;
    if(flags & X86EMU_DUMP_ASCII) dump_flags |= 0x100;

    pdir = mem->pdir;
    for(pdir_idx = 0; pdir_idx < (1 << X86EMU_PDIR_BITS); pdir_idx++) {
      ptable = (*pdir)[pdir_idx];
      if(!ptable) continue;
      for(u1 = 0; u1 < (1 << X86EMU_PTABLE_BITS); u1++) {
        page = (*ptable)[u1];
        if(page.data) {
          for(u2 = 0; u2 < X86EMU_PAGE_SIZE; u2 += LINE_LEN) {
            memcpy(def_data, page.data + u2, LINE_LEN);
            if(page.attr) {
              memcpy(def_attr, page.attr + u2, LINE_LEN);
            }
            else {
              memset(def_attr, page.def_attr, LINE_LEN);
            }
            dump_data(emu, def_data, def_attr, str_data, str_attr, dump_flags);
            if(*str_data) {
              addr = (((pdir_idx << X86EMU_PTABLE_BITS) + u1) << X86EMU_PAGE_BITS) + u2;
              x86emu_log(emu, "%08x: %s\n", addr, str_data);
              if((flags & X86EMU_DUMP_ATTR)) x86emu_log(emu, "          %s\n", str_attr);
            }
          }
        }
      }
    }

    x86emu_log(emu, "\n");
  }

  if((flags & X86EMU_DUMP_IO)) {
    x86emu_log(emu, "; - - io accesses\n");

    for(u = 0; u < X86EMU_IO_PORTS; u++) {
      if(emu->io.map[u] & (X86EMU_ACC_R | X86EMU_ACC_W | X86EMU_ACC_INVALID)) {
        x86emu_log(emu,
          "%04x: %c%c%c in=%08x out=%08x\n",
          u,
          (emu->io.map[u] & X86EMU_ACC_INVALID) ? '*' : ' ',
          (emu->io.map[u] & X86EMU_PERM_R) ? 'r' : ' ',
          (emu->io.map[u] & X86EMU_PERM_W) ? 'w' : ' ',
          emu->io.stats_i[u], emu->io.stats_o[u]
        );
      }
    }

    x86emu_log(emu, "\n");
  }

  if((flags & X86EMU_DUMP_INTS)) {
    x86emu_log(emu, "; - - interrupt statistics\n");
    for(u1 = 0; u1 < 0x100; u1++) {
      if(emu->x86.intr_stats[u1]) x86emu_log(emu, "int %02x: %08x\n", u1, emu->x86.intr_stats[u1]);
    }

    x86emu_log(emu, "\n");
  }

  if((flags & X86EMU_DUMP_REGS)) {
    x86emu_log(emu, "; - - registers\n");

    for(u = u1 = 0; u < X86EMU_MSRS; u++) {
      if(u >= 0x11 && u <= 0x12 && !(flags & X86EMU_DUMP_TIME)) continue;
      if(emu->x86.msr_perm[u]) {
        u1 = 1;
        x86emu_log(emu, "msr[%04x] %c%c %016llx",
          u,
          (emu->x86.msr_perm[u] & X86EMU_ACC_R) ? 'r' : ' ',
          (emu->x86.msr_perm[u] & X86EMU_ACC_W) ? 'w' : ' ',
          (unsigned long long) emu->x86.msr[u]
        );
        switch(u) {
          case 0x10:
            x86emu_log(emu, " ; tsc");
            break;
          case 0x11:
            x86emu_log(emu, " ; real tsc (previous)");
            break;
          case 0x12:
            x86emu_log(emu, " ; real tsc");
            if(emu->x86.R_TSC) {
              x86emu_log(emu, ", ratio=%.2f", (double) emu->x86.R_REAL_TSC / emu->x86.R_TSC);
            }
            break;
        }
        x86emu_log(emu, "\n");
      }
    }

    if(u1) x86emu_log(emu, "\n");

    x86emu_log(emu, "cr0=%08x cr1=%08x cr2=%08x cr3=%08x cr4=%08x\n",
      emu->x86.R_CR0, emu->x86.R_CR1, emu->x86.R_CR2, emu->x86.R_CR3, emu->x86.R_CR4
    );

    x86emu_log(emu, "dr0=%08x dr1=%08x dr2=%08x dr3=%08x dr6=%08x dr7=%08x\n\n",
      emu->x86.R_DR0, emu->x86.R_DR1, emu->x86.R_DR2, emu->x86.R_DR3,
      emu->x86.R_DR6, emu->x86.R_DR7
    );

    x86emu_log(emu,
      "gdt.base=%08x gdt.limit=%04x\n",
      emu->x86.R_GDT_BASE, emu->x86.R_GDT_LIMIT
    );

    x86emu_log(emu,
      "idt.base=%08x idt.limit=%04x\n",
      emu->x86.R_IDT_BASE, emu->x86.R_IDT_LIMIT
    );

    x86emu_log(emu,
      "tr=%04x tr.base=%08x tr.limit=%08x tr.acc=%04x\n",
      emu->x86.R_TR, emu->x86.R_TR_BASE, emu->x86.R_TR_LIMIT, emu->x86.R_TR_ACC
    );
    x86emu_log(emu,
      "ldt=%04x ldt.base=%08x ldt.limit=%08x ldt.acc=%04x\n\n",
      emu->x86.R_LDT, emu->x86.R_LDT_BASE, emu->x86.R_LDT_LIMIT, emu->x86.R_LDT_ACC
    );

    x86emu_log(emu,
      "cs=%04x cs.base=%08x cs.limit=%08x cs.acc=%04x\n",
      emu->x86.R_CS, emu->x86.R_CS_BASE, emu->x86.R_CS_LIMIT, emu->x86.R_CS_ACC
    );
    x86emu_log(emu,
      "ss=%04x ss.base=%08x ss.limit=%08x ss.acc=%04x\n",
      emu->x86.R_SS, emu->x86.R_SS_BASE, emu->x86.R_SS_LIMIT, emu->x86.R_SS_ACC
    );
    x86emu_log(emu,
      "ds=%04x ds.base=%08x ds.limit=%08x ds.acc=%04x\n",
      emu->x86.R_DS, emu->x86.R_DS_BASE, emu->x86.R_DS_LIMIT, emu->x86.R_DS_ACC
    );
    x86emu_log(emu,
      "es=%04x es.base=%08x es.limit=%08x es.acc=%04x\n",
      emu->x86.R_ES, emu->x86.R_ES_BASE, emu->x86.R_ES_LIMIT, emu->x86.R_ES_ACC
    );
    x86emu_log(emu,
      "fs=%04x fs.base=%08x fs.limit=%08x fs.acc=%04x\n",
      emu->x86.R_FS, emu->x86.R_FS_BASE, emu->x86.R_FS_LIMIT, emu->x86.R_FS_ACC
    );
    x86emu_log(emu,
      "gs=%04x gs.base=%08x gs.limit=%08x gs.acc=%04x\n\n",
      emu->x86.R_GS, emu->x86.R_GS_BASE, emu->x86.R_GS_LIMIT, emu->x86.R_GS_ACC
    );
    x86emu_log(emu, "eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
      emu->x86.R_EAX, emu->x86.R_EBX, emu->x86.R_ECX, emu->x86.R_EDX
    );
    x86emu_log(emu, "esi=%08x edi=%08x ebp=%08x esp=%08x\n",
      emu->x86.R_ESI, emu->x86.R_EDI, emu->x86.R_EBP, emu->x86.R_ESP
    );
    x86emu_log(emu, "eip=%08x eflags=%08x", emu->x86.R_EIP, emu->x86.R_EFLG);

    *fbuf = 0;
    if(emu->x86.R_EFLG & 0x800) strcat(fbuf, " of");
    if(emu->x86.R_EFLG & 0x400) strcat(fbuf, " df");
    if(emu->x86.R_EFLG & 0x200) strcat(fbuf, " if");
    if(emu->x86.R_EFLG & 0x080) strcat(fbuf, " sf");
    if(emu->x86.R_EFLG & 0x040) strcat(fbuf, " zf");
    if(emu->x86.R_EFLG & 0x010) strcat(fbuf, " af");
    if(emu->x86.R_EFLG & 0x004) strcat(fbuf, " pf");
    if(emu->x86.R_EFLG & 0x001) strcat(fbuf, " cf");

    if(*fbuf) x86emu_log(emu, " ;%s", fbuf);

    x86emu_log(emu, "\n\n");
  }
}

