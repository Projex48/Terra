/************************************************/
// TerraOS Main File

/************************************************/
// System includes

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/************************************************/
// Bootloader

#include "limine.h"

/************************************************/
// Local includes

#include "./graphics/t_graphics.c"
#include "./memory/t_syscalls.c"

/************************************************/
// Limine requests

// Set the base revision to 5, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used,
               section(".limine_requests_start"))) static volatile uint64_t
    limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
    limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

/************************************************/

void kernel_main(void) {

  graphics_initialize();

  setBackground(GRAY);

  putchar('h', 0, 0, WHITE, GRAY);
  putchar('i', 1, 0, WHITE, GRAY);

  // We're done, just hang...
  hcf();
}