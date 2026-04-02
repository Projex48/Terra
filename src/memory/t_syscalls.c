#ifndef TERRA_SYSCALLS
#define TERRA_SYSCALLS

/************************************************/
// System Includes

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/************************************************/
// Local Includes

#include "../limine.h"

/************************************************/
// Limine requests

__attribute__((
    used, section(".requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

/************************************************/
// Linker symbols

extern char _kernel_start;
extern char _kernel_end;

/************************************************/
// Globals

uint8_t *bitmap;
size_t total_pages;

/************************************************/

// Halt and catch fire function.
static void hcf(void) {
  for (;;) {
    asm("hlt");
  }
}

/************************************************/

// Mark a memory page as free
void mark_page_free(uint64_t page_index) {
  bitmap[page_index / 8] &= ~(1 << (page_index % 8));
}

// Mark a memory page as used
void mark_page_used(uint64_t page_index) {
  bitmap[page_index / 8] |= (1 << (page_index % 8));
}

/************************************************/

// Parse limine's memory map to find usable memory regions
// Mark free regions in bitmap
void init_parse_mm() {
  struct limine_memmap_response *res = memmap_request.response;

  if (!res)
    hcf();

  for (uint64_t i = 0; i < res->entry_count; ++i) {
    struct limine_memmap_entry *entry = res->entries[i];

    if (entry->type != LIMINE_MEMMAP_USABLE) {
      // This memory entry is unsafe to use
      continue;
    }

    // Break page into 4kb pages
#define PAGE_SIZE 4096
    uint64_t start = entry->base;
    uint64_t end = entry->base + entry->length;

    // Align
    start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    end = end & ~(PAGE_SIZE - 1);

    for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
      // avoid addresses < 1MB for stability
      if (addr < 0x100000)
        continue;

      // avoid kernel regions
      if (addr >= (uint64_t)&_kernel_start && addr < (uint64_t)&_kernel_end)
        continue;

      uint64_t idx = addr / PAGE_SIZE;
      mark_page_free(idx);
    }
  }
}

/************************************************/
// Alloc and free

// Really simple kernel allocator method
void *kalloc() {
    for (size_t i = 0; i < total_pages; i++) {
        // Check bitmap to see if memory is free
        if (!bitmap[i]) {
            mark_page_used(i);
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL; // out of memory
}

// Oversimplified kernel free method
void kfree(void *addr) {
    size_t index = (size_t)addr / PAGE_SIZE;
    mark_page_free(index);
}

/************************************************/

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.
void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
  uint8_t *restrict pdest = (uint8_t *restrict)dest;
  const uint8_t *restrict psrc = (const uint8_t *restrict)src;

  for (size_t i = 0; i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *memset(void *s, int c, size_t n) {
  uint8_t *p = (uint8_t *)s;

  for (size_t i = 0; i < n; i++) {
    p[i] = (uint8_t)c;
  }

  return s;
}

void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  if ((uintptr_t)src > (uintptr_t)dest) {
    for (size_t i = 0; i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if ((uintptr_t)src < (uintptr_t)dest) {
    for (size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }

  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] < p2[i] ? -1 : 1;
    }
  }

  return 0;
}

#endif