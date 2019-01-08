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
*   Memory and i/o access emulation functions.
*
****************************************************************************/


#include "include/x86emu_int.h"
#if defined(__i386__) || defined (__x86_64__)
#include <sys/uio.h>
#endif

static inline void outb(unsigned short port, unsigned char value)
{
  __asm__ __volatile__ ("outb %1, %0" : : "dN" (port), "a" (value));

}
static inline unsigned char inb(unsigned short port)
{
  unsigned char value;
  __asm__ __volatile__ ("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static __inline unsigned short int inw (unsigned short int __port)
{
  unsigned short _v;

  __asm__ __volatile__ ("inw %w1,%0":"=a" (_v):"Nd" (__port));
  return _v;
}

static __inline void outw (unsigned short int __value, unsigned short int __port)
{
  __asm__ __volatile__ ("outw %w0,%w1": :"a" (__value), "Nd" (__port));

}


static __inline unsigned int
inl (unsigned short int port)
{
  unsigned int _v;
  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (port));
  return _v;
}

static __inline void
outl (unsigned int value, unsigned short int port)
{
  __asm__ __volatile__ ("outl %0,%w1": :"a" (value), "Nd" (port));
}

#define PERM16(a)	((a) + ((a) << 8))
#define PERM32(a)	(PERM16(a) + (PERM16(a) << 16))

// avoid unaligned memory accesses
#define STRICT_ALIGN	0

static unsigned vm_r_byte(x86emu_mem_t *vm, unsigned addr);
static unsigned vm_r_byte_noperm(x86emu_mem_t *vm, unsigned addr);
static unsigned vm_r_word(x86emu_mem_t *vm, unsigned addr);
static unsigned vm_r_dword(x86emu_mem_t *vm, unsigned addr);
static unsigned vm_x_byte(x86emu_mem_t *vm, unsigned addr);
static unsigned vm_x_word(x86emu_mem_t *vm, unsigned addr);
static unsigned vm_x_dword(x86emu_mem_t *vm, unsigned addr);
static void vm_w_byte(x86emu_mem_t *vm, unsigned addr, unsigned val);
static void vm_w_byte_noperm(x86emu_mem_t *vm, unsigned addr, unsigned val);
static void vm_w_word(x86emu_mem_t *vm, unsigned addr, unsigned val);
static void vm_w_dword(x86emu_mem_t *vm, unsigned addr, unsigned val);

static mem2_page_t *vm_get_page(x86emu_mem_t *mem, unsigned addr, int create);
static unsigned vm_i_byte(x86emu_t *emu, unsigned addr);
static unsigned vm_i_dword(x86emu_t *emu, unsigned addr);
static unsigned vm_i_word(x86emu_t *emu, unsigned addr);
static void vm_o_byte(x86emu_t *emu, unsigned addr, unsigned val);
static void vm_o_dword(x86emu_t *emu, unsigned addr, unsigned val);
static void vm_o_word(x86emu_t *emu, unsigned addr, unsigned val);

void *mem_dup(const void *src, size_t n)
{
  void *dst;

  if(!src || !n || !(dst = malloc(n))) return NULL;

  memcpy(dst, src, n);

  return dst;
}


x86emu_mem_t *emu_mem_new(unsigned perm)
{
  x86emu_mem_t *mem;

  mem = calloc(1, sizeof *mem);
  mem->def_attr = perm;

  return mem;
}


x86emu_mem_t *emu_mem_free(x86emu_mem_t *mem)
{
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx, u1;

  if(mem) {
    if((pdir = mem->pdir)) {
      for(pdir_idx = 0; pdir_idx < (1 << X86EMU_PDIR_BITS); pdir_idx++) {
        ptable = (*pdir)[pdir_idx];
        if(!ptable) continue;
        for(u1 = 0; u1 < (1 << X86EMU_PTABLE_BITS); u1++) {
          page = (*ptable)[u1];
          free(page.attr);
        }
        free(ptable);
      }
      free(pdir);
    }

    free(mem);
  }

  return NULL;
}


x86emu_mem_t *emu_mem_clone(x86emu_mem_t *mem)
{
  mem2_pdir_t *pdir, *new_pdir;
  mem2_ptable_t *ptable, *new_ptable;
  mem2_page_t page;
  unsigned pdir_idx, u1;
  x86emu_mem_t *new_mem = NULL;

  if(!mem) return new_mem;

  new_mem = mem_dup(mem, sizeof *new_mem);

  if((pdir = mem->pdir)) {
    new_pdir = new_mem->pdir = mem_dup(mem->pdir, sizeof *mem->pdir);
    for(pdir_idx = 0; pdir_idx < (1 << X86EMU_PDIR_BITS); pdir_idx++) {
      ptable = (*pdir)[pdir_idx];
      if(!ptable) continue;
      new_ptable = (*new_pdir)[pdir_idx] = mem_dup(ptable, sizeof *ptable);
      for(u1 = 0; u1 < (1 << X86EMU_PTABLE_BITS); u1++) {
        page = (*ptable)[u1];
        if(page.attr) {
          (*new_ptable)[u1].attr = mem_dup(page.attr, 2 * X86EMU_PAGE_SIZE);
          if(page.data == page.attr + X86EMU_PAGE_SIZE) {
            (*new_ptable)[u1].data = (*new_ptable)[u1].attr + X86EMU_PAGE_SIZE;
          }
        }
      }
    }
  }

  return new_mem;
}


API_SYM void x86emu_reset_access_stats(x86emu_t *emu)
{
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx, u, u1;

  if(!emu || !emu->mem || !(pdir = emu->mem->pdir)) return;

  for(pdir_idx = 0; pdir_idx < (1 << X86EMU_PDIR_BITS); pdir_idx++) {
    ptable = (*pdir)[pdir_idx];
    if(!ptable) continue;
    for(u = 0; u < (1 << X86EMU_PTABLE_BITS); u++) {
      page = (*ptable)[u];
      if(page.attr) {
        for(u1 = 0; u1 < X86EMU_PAGE_SIZE; u1++) {
          page.attr[u1] &= X86EMU_PERM_RWX | X86EMU_PERM_VALID;
        }
      }
    }
  }
}


API_SYM void x86emu_set_io_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm)
{
  if(!emu) return;

  if(end > X86EMU_IO_PORTS - 1) end = X86EMU_IO_PORTS - 1;

  while(start <= end) emu->io.map[start++] = perm;

  for(start = perm = 0; start < X86EMU_IO_PORTS; start++) {
    perm |= emu->io.map[start];
  }

  emu->io.iopl_needed = (perm & (X86EMU_PERM_R | X86EMU_PERM_W)) ? 1 : 0;

#if WITH_IOPL 
  emu->io.iopl_ok = emu->io.iopl_needed && getiopl() != 3 ? 0 : 1;
#else 
  emu->io.iopl_ok = 1;
#endif
}


mem2_page_t *vm_get_page(x86emu_mem_t *mem, unsigned addr, int create)
{
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx = addr >> (32 - X86EMU_PDIR_BITS);
  unsigned ptable_idx = (addr >> X86EMU_PAGE_BITS) & ((1 << X86EMU_PTABLE_BITS) - 1);
  unsigned u;

  pdir = mem->pdir;
  if(!pdir) {
    mem->pdir = pdir = calloc(1, sizeof *pdir);
    // fprintf(stderr, "pdir = %p (%d)\n", pdir, sizeof *pdir);
  }

  ptable = (*pdir)[pdir_idx];
  if(!ptable) {
    ptable = (*pdir)[pdir_idx] = calloc(1, sizeof *ptable);
    // fprintf(stderr, "ptable = %p\n", ptable);
    for(u = 0; u < (1 << X86EMU_PTABLE_BITS); u++) {
      (*ptable)[u].def_attr = mem->def_attr;
      // fprintf(stderr, "ptable[%u] = %p\n", u, &((*ptable)[u].def_attr));
    }
    // fprintf(stderr, "pdir[%d] = %p (%d)\n", pdir_idx, ptable, sizeof *ptable);
  }

  if(create) {
    page = (*ptable)[ptable_idx];
    if(!page.attr) {
      page.attr = calloc(1, 2 * X86EMU_PAGE_SIZE);
      page.data = page.attr + X86EMU_PAGE_SIZE;
      // fprintf(stderr, "page = %p, page.def_attr = %p\n", page, &page.def_attr);
      memset(page.attr, page.def_attr, X86EMU_PAGE_SIZE);
      (*ptable)[ptable_idx] = page;
      // fprintf(stderr, "page.attr[%d] = %p\n", ptable_idx, page.attr);
    }
  }

  return (*ptable) + ptable_idx;
}


API_SYM void x86emu_set_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm)
{
  x86emu_mem_t *mem;
  mem2_page_t *page;
  unsigned idx;

  if(!emu || !(mem = emu->mem)) return;

  if(start > end) return;

  // x86emu_log(emu, "set perm: start 0x%x, end 0x%x, perm 0x%x\n", start, end, perm);

  if((idx = start & (X86EMU_PAGE_SIZE - 1))) {
    page = vm_get_page(mem, start, 1);
    for(; idx < X86EMU_PAGE_SIZE && start <= end; start++) {
      // x86emu_log(emu, "  page %p, idx = 0x%x\n", page, idx);
      page->attr[idx++] = perm;
    }
    if(!start || start > end) return;
  }

  // x86emu_log(emu, "  2: start 0x%x, end 0x%x\n", start, end);

  for(; end - start >= X86EMU_PAGE_SIZE - 1; start += X86EMU_PAGE_SIZE) {
    page = vm_get_page(mem, start, 0);
    page->def_attr = perm;
    // x86emu_log(emu, "  page %p (start 0x%x, end - start 0x%x)\n", page, start, end - start);
    if(page->attr) memset(page->attr, page->def_attr, X86EMU_PAGE_SIZE);
    if(!start) return;
    if(end - start == X86EMU_PAGE_SIZE - 1) {
      start += X86EMU_PAGE_SIZE;
      break;
    }
  }

  if(start > end) return;

  // x86emu_log(emu, "  3: start 0x%x, end 0x%x\n", start, end);

  page = vm_get_page(mem, start, 1);
  end = end - start + 1;
  for(idx = 0; idx < end; idx++) {
    // x86emu_log(emu, "  page %p, idx = 0x%x\n", page, idx);
    page->attr[idx] = perm;
  }
}


API_SYM void x86emu_set_page(x86emu_t *emu, unsigned page, void *address)
{
  x86emu_mem_t *mem;
  mem2_page_t *p;
  unsigned u;

  if(!emu || !(mem = emu->mem)) return;

  p = vm_get_page(mem, page, 1);

  if(address) {
    p->data = address;

    // tag memory as initialized
    for(u = 0; u < X86EMU_PAGE_SIZE; u++) {
      p->attr[u] |= X86EMU_PERM_VALID;
    }
  }
  else {
    p->data = p->attr + X86EMU_PAGE_SIZE;
  }
}


unsigned vm_r_byte(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *perm;

  page = vm_get_page(mem, addr, 1);
  perm = page->attr + page_idx;

  if(*perm & X86EMU_PERM_R) {
    *perm |= X86EMU_ACC_R;
    if(!(*perm & X86EMU_PERM_VALID)) {
      *perm |= X86EMU_ACC_INVALID;
      mem->invalid = 1;
    }
    return page->data[page_idx];
  }

  mem->invalid = 1;

  return 0xff;
}


unsigned vm_r_byte_noperm(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  // unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  // attr = page->attr + page_idx;

  return page->data[page_idx];
}


unsigned vm_r_word(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned val, page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  u16 *perm16;

  page = vm_get_page(mem, addr, 1);
  perm16 = (u16 *) (page->attr + page_idx);

  if(
#if STRICT_ALIGN
    (page_idx & 1) ||
#else
    page_idx >= X86EMU_PAGE_SIZE - 1 ||
#endif
    (*perm16 & PERM16(X86EMU_PERM_R | X86EMU_PERM_VALID)) != PERM16(X86EMU_PERM_R | X86EMU_PERM_VALID)
  ) {
    val = vm_r_byte(mem, addr);
    val += vm_r_byte(mem, addr + 1) << 8;

    return val;
  }

  *perm16 |= PERM16(X86EMU_ACC_R);

#if defined(__BIG_ENDIAN__) || STRICT_ALIGN
  val = page->data[page_idx] + (page->data[page_idx + 1] << 8);
#else
  val = *(u16 *) (page->data + page_idx);
#endif

  return val;
}


unsigned vm_r_dword(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned val, page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  u32 *perm32;

  page = vm_get_page(mem, addr, 1);
  perm32 = (u32 *) (page->attr + page_idx);

  if(
#if STRICT_ALIGN
    (page_idx & 3) ||
#else
    page_idx >= X86EMU_PAGE_SIZE - 3 ||
#endif
    (*perm32 & PERM32(X86EMU_PERM_R | X86EMU_PERM_VALID)) != PERM32(X86EMU_PERM_R | X86EMU_PERM_VALID)
  ) {
    val = vm_r_byte(mem, addr);
    val += vm_r_byte(mem, addr + 1) << 8;
    val += vm_r_byte(mem, addr + 2) << 16;
    val += vm_r_byte(mem, addr + 3) << 24;

    return val;
  }

  *perm32 |= PERM32(X86EMU_ACC_R);

#if defined(__BIG_ENDIAN__) || STRICT_ALIGN
  val = page->data[page_idx] +
    (page->data[page_idx + 1] << 8) +
    (page->data[page_idx + 2] << 16) +
    (page->data[page_idx + 3] << 24);
#else
  val = *(u32 *) (page->data + page_idx);
#endif

  return val;
}


unsigned vm_x_byte(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  if(*attr & X86EMU_PERM_X) {
    *attr |= X86EMU_ACC_X;
    if(!(*attr & X86EMU_PERM_VALID)) {
      *attr |= X86EMU_ACC_INVALID;
      mem->invalid = 1;
    }
    return page->data[page_idx];
  }

  mem->invalid = 1;

  return 0xff;
}


unsigned vm_x_word(x86emu_mem_t *mem, unsigned addr)
{
  return vm_x_byte(mem, addr) + (vm_x_byte(mem, addr + 1) << 8);
}


unsigned vm_x_dword(x86emu_mem_t *mem, unsigned addr)
{
  return vm_x_word(mem, addr) + (vm_x_word(mem, addr + 2) << 16);
}


void vm_w_byte(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  if(*attr & X86EMU_PERM_W) {
    *attr |= X86EMU_PERM_VALID | X86EMU_ACC_W;
    page->data[page_idx] = val;
  }
  else {
    *attr |= X86EMU_ACC_INVALID;

    mem->invalid = 1;
  }
}


void vm_w_byte_noperm(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  *attr |= X86EMU_PERM_VALID | X86EMU_ACC_W;
  page->data[page_idx] = val;
}


void vm_w_word(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  u16 *perm16;

  page = vm_get_page(mem, addr, 1);
  perm16 = (u16 *) (page->attr + page_idx);

  if(
#if STRICT_ALIGN
    (page_idx & 1) ||
#else
    page_idx >= X86EMU_PAGE_SIZE - 1 ||
#endif
    (*perm16 & PERM16(X86EMU_PERM_W)) != PERM16(X86EMU_PERM_W)
  ) {
    vm_w_byte(mem, addr, val);
    vm_w_byte(mem, addr + 1, val >> 8);

    return;
  }

  *perm16 |= PERM16(X86EMU_PERM_VALID | X86EMU_ACC_W);

#if defined(__BIG_ENDIAN__) || STRICT_ALIGN
  page->data[page_idx] = val;
  page->data[page_idx + 1] = val >> 8;
#else
  *(u16 *) (page->data + page_idx) = val;
#endif
}


void vm_w_dword(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  u32 *perm32;

  page = vm_get_page(mem, addr, 1);
  perm32 = (u32 *) (page->attr + page_idx);

  if(
#if STRICT_ALIGN
    (page_idx & 3) ||
#else
    page_idx >= X86EMU_PAGE_SIZE - 3 ||
#endif
    (*perm32 & PERM32(X86EMU_PERM_W)) != PERM32(X86EMU_PERM_W)
  ) {
    vm_w_byte(mem, addr, val);
    vm_w_byte(mem, addr + 1, val >> 8);
    vm_w_byte(mem, addr + 2, val >> 16);
    vm_w_byte(mem, addr + 3, val >> 24);

    return;
  }

  *perm32 |= PERM32(X86EMU_PERM_VALID | X86EMU_ACC_W);

#if defined(__BIG_ENDIAN__) || STRICT_ALIGN
  page->data[page_idx] = val;
  page->data[page_idx + 1] = val >> 8;
  page->data[page_idx + 2] = val >> 16;
  page->data[page_idx + 3] = val >> 24;
#else
  *(u32 *) (page->data + page_idx) = val;
#endif
}


unsigned vm_i_byte(x86emu_t *emu, unsigned addr)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = emu->io.map + addr;

  if(
    emu->io.iopl_ok &&
    (*perm & X86EMU_PERM_R)
  ) {
    *perm |= X86EMU_ACC_R;

    emu->io.stats_i[addr]++;

    return inb(addr);
  }
  else {
    *perm |= X86EMU_ACC_INVALID;
  }

  emu->mem->invalid = 1;

  return 0xff;
}


unsigned vm_i_word(x86emu_t *emu, unsigned addr)
{
  unsigned char *perm;
  unsigned val;

  addr &= 0xffff;
  perm = emu->io.map + addr;

  if(
    !emu->io.iopl_ok ||
    addr == 0xffff ||
    !(perm[0] & X86EMU_PERM_R) ||
    !(perm[1] & X86EMU_PERM_R)
  ) {
    val = vm_i_byte(emu, addr);
    val += (vm_i_byte(emu, addr + 1) << 8);

    return val;
  }

  perm[0] |= X86EMU_ACC_R;
  perm[1] |= X86EMU_ACC_R;

  emu->io.stats_i[addr]++;
  emu->io.stats_i[addr + 1]++;

  return inw(addr);
}


unsigned vm_i_dword(x86emu_t *emu, unsigned addr)
{
  unsigned char *perm;
  unsigned val;

  addr &= 0xffff;
  perm = emu->io.map + addr;

  if(
    !emu->io.iopl_ok ||
    addr >= 0xfffd ||
    !(perm[0] & X86EMU_PERM_R) ||
    !(perm[1] & X86EMU_PERM_R) ||
    !(perm[2] & X86EMU_PERM_R) ||
    !(perm[3] & X86EMU_PERM_R)
  ) {
    val = vm_i_byte(emu, addr);
    val += (vm_i_byte(emu, addr + 1) << 8);
    val += (vm_i_byte(emu, addr + 2) << 16);
    val += (vm_i_byte(emu, addr + 3) << 24);

    return val;
  }

  perm[0] |= X86EMU_ACC_R;
  perm[1] |= X86EMU_ACC_R;
  perm[2] |= X86EMU_ACC_R;
  perm[3] |= X86EMU_ACC_R;

  emu->io.stats_i[addr]++;
  emu->io.stats_i[addr + 1]++;
  emu->io.stats_i[addr + 2]++;
  emu->io.stats_i[addr + 3]++;

  return inl(addr);
}


void vm_o_byte(x86emu_t *emu, unsigned addr, unsigned val)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = emu->io.map + addr;

  if(
    emu->io.iopl_ok &&
    (*perm & X86EMU_PERM_W)
  ) {
    *perm |= X86EMU_ACC_W;

    emu->io.stats_o[addr]++;

    outb(val, addr);
  }
  else {
    *perm |= X86EMU_ACC_INVALID;

    emu->mem->invalid = 1;
  }
}


void vm_o_word(x86emu_t *emu, unsigned addr, unsigned val)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = emu->io.map + addr;

  if(
    !emu->io.iopl_ok ||
    addr == 0xffff ||
    !(perm[0] & X86EMU_PERM_W) ||
    !(perm[1] & X86EMU_PERM_W)
  ) {
    vm_o_byte(emu, addr, val);
    vm_o_byte(emu, addr + 1, val);

    return;
  }

  perm[0] |= X86EMU_ACC_W;
  perm[1] |= X86EMU_ACC_W;

  emu->io.stats_o[addr]++;
  emu->io.stats_o[addr + 1]++;

  outw(val, addr);
}


void vm_o_dword(x86emu_t *emu, unsigned addr, unsigned val)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = emu->io.map + addr;

  if(
    !emu->io.iopl_ok ||
    addr >= 0xfffd ||
    !(perm[0] & X86EMU_PERM_W) ||
    !(perm[1] & X86EMU_PERM_W) ||
    !(perm[2] & X86EMU_PERM_W) ||
    !(perm[3] & X86EMU_PERM_W)
  ) {
    vm_o_byte(emu, addr, val);
    vm_o_byte(emu, addr + 1, val);
    vm_o_byte(emu, addr + 2, val);
    vm_o_byte(emu, addr + 3, val);

    return;
  }

  perm[0] |= X86EMU_ACC_W;
  perm[1] |= X86EMU_ACC_W;
  perm[2] |= X86EMU_ACC_W;
  perm[3] |= X86EMU_ACC_W;

  emu->io.stats_o[addr]++;
  emu->io.stats_o[addr + 1]++;
  emu->io.stats_o[addr + 2]++;
  emu->io.stats_o[addr + 3]++;

  outl(val, addr);
}


unsigned vm_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type)
{
  x86emu_mem_t *mem = emu->mem;
  unsigned bits = type & 0xff;

  type &= ~0xff;

  mem->invalid = 0;

  switch(type) {
    case X86EMU_MEMIO_R:
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_r_byte(mem, addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_r_word(mem, addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_r_dword(mem, addr);
          break;
        case X86EMU_MEMIO_8_NOPERM:
          *val = vm_r_byte_noperm(mem, addr);
          break;
      }
      break;

    case X86EMU_MEMIO_W:
      switch(bits) {
        case X86EMU_MEMIO_8:
          vm_w_byte(mem, addr, *val);
          break;
        case X86EMU_MEMIO_16:
          vm_w_word(mem, addr, *val);
          break;
        case X86EMU_MEMIO_32:
          vm_w_dword(mem, addr, *val);
          break;
        case X86EMU_MEMIO_8_NOPERM:
          vm_w_byte_noperm(mem, addr, *val);
          break;
      }
      break;

    case X86EMU_MEMIO_X:
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_x_byte(mem, addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_x_word(mem, addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_x_dword(mem, addr);
          break;
      }
      break;

    case X86EMU_MEMIO_I:
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_i_byte(emu, addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_i_word(emu, addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_i_dword(emu, addr);
          break;
      }
      break;

    case X86EMU_MEMIO_O:
      switch(bits) {
        case X86EMU_MEMIO_8:
          vm_o_byte(emu, addr, *val);
          break;
        case X86EMU_MEMIO_16:
          vm_o_word(emu, addr, *val);
          break;
        case X86EMU_MEMIO_32:
          vm_o_dword(emu, addr, *val);
          break;
      }
      break;
  }

  return mem->invalid;
}

