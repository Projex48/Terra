#ifndef TERRA_GRAPHICS
#define TERRA_GRAPHICS

/*************************************/
// EXOs Graphics Driver

/***********************************************************/
// System libs

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/***********************************************************/
// Local includes

#include "limine.h"
#include "terra_syscalls.c"

/***********************************************************/
// External

// Load font
extern char _binary_font_psf_start[];
extern char _binary_font_psf_end;


/***********************************************************/
// Structs

// For PSF1
#define PSF1_FONT_MAGIC 0x0436
typedef struct {
  uint16_t magic;        // Magic bytes for identification.
  uint8_t fontMode;      // PSF font mode.
  uint8_t characterSize; // PSF character size.
} PSF1_Header;

// for PSF2
#define PSF_FONT_MAGIC 0x864ab572
typedef struct {
  uint32_t magic;         /* magic bytes to identify PSF */
  uint32_t version;       /* zero */
  uint32_t headersize;    /* offset of bitmaps in file, 32 */
  uint32_t flags;         /* 0 if there's no unicode table */
  uint32_t numglyph;      /* number of glyphs */
  uint32_t bytesperglyph; /* size of each glyph */
  uint32_t height;        /* height in pixels */
  uint32_t width;         /* width in pixels */
} PSF_font;

/***********************************************************/
// Enums

enum color_palette {
  BLACK = 0x000000,
  GRAY = 0x212121,
  WHITE = 0xFFFFFF,
  BLUE = 0x0000FF,
  GREEN = 0x00FF00,
  RED = 0xFF0000,
};

/***********************************************************/
// typedefs

typedef uint32_t size_type;

/***********************************************************/
// Globals

// Set the base revision to 5, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(5);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

// Linear framebuffer
struct limine_framebuffer *framebuffer;

// unicode table address;
uint16_t *unicode;

// terminal width, height
size_type width, height;

/***********************************************************/
// Simple graphics functions

void psf_initialize() {
  uint16_t glyph = 0;
  PSF_font *font = (PSF_font *)&_binary_font_psf_start;
  // Check if there is a unicode table
  if (font->flags == 0) {
    unicode = NULL;
    return;
  }

  // Gets the offset of the table
  char *s = (char *)((unsigned char *)&_binary_font_psf_start + font->headersize +
                     font->numglyph + font->bytesperglyph);

  // Allocate memory for translation
  // PAIN AND SUFFERING, THAT'S IT... IT'S ALL PAIN 💀
  unicode = calloc(USHRT_MAX, 2);
  while (s < (unsigned char *)&_binary_font_psf_end) {
    uint16_t uc = (uint16_t)((unsigned char *)s[0]);
    if (uc == 0xFF) {
      glyph++;
      s++;
      continue;
    } else if (uc & 128) {
      // UTF-8 => Unicode
      if ((uc & 32) == 0) {
        uc = ((s[0] & 0x1F) << 6) + (s[1] & 0x3F);
        s++;
      } else if ((uc & 16) == 0) {
        uc = ((((s[0] & 0xF) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F);
        s += 2;
      } else if ((uc & 8) == 0) {
        uc = ((((((s[0] & 0x7) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F))
              << 6) +
             (s[3] & 0x3F);
        s += 3;
      } else
        uc = 0;
    }
    unicode[uc] = glyph;
    s++;
  }
}

// Initialize terminal
void graphics_initialize(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
    hcf();
  }

  // Ensure we got a framebuffer.
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  // Fetch the first framebuffer.
  framebuffer = framebuffer_request.response->framebuffers[0];

  // Get width and height of framebuffer;
  width = framebuffer->width;
  height = framebuffer->height;

  psf_initialize();
}

void setBackground(size_type VGA_COLOR) {
  volatile size_type *fb_ptr = framebuffer->address;
  size_type size = framebuffer->height * framebuffer->width;
  for (size_type i = 0; i < size; ++i) {
    fb_ptr[i] = VGA_COLOR;
  }
}

void createRectangle(size_type start_x, size_type start_y, size_type end_x,
                     size_type end_y, size_type VGA_COLOR) {
  volatile size_type *fb_ptr = framebuffer->address;
  for (size_type y = start_y; y < end_y; ++y) {
    for (size_type x = start_x; x < end_x; ++x) {
      fb_ptr[y * (framebuffer->pitch / 4) + x] = VGA_COLOR;
    }
  }
}

void putpixel(size_type pos_x, size_type pos_y, size_type VGA_COLOR) {
  volatile size_type *fb_ptr = framebuffer->address;
  fb_ptr[pos_y * (framebuffer->pitch / 4) + pos_x] = VGA_COLOR;
}

#define PIXEL size_type
void putchar(
    // int, because it's unicode
    unsigned short int c,
    // cursor x,y
    int cx, int cy,
    // fg & bg colors
    uint32_t fg, uint32_t bg) {
  PSF_font *font = (PSF_font *)&_binary_font_psf_start;

  if (unicode != NULL)
    c = unicode[c];

  unsigned char *glyph =
      (unsigned char *)&_binary_font_psf_start + font->headersize +
      (c > 0 && c < font->numglyph ? c : 0) * font->bytesperglyph;

  // Calculates the position of the top left corner on screen
  int offs =
      (cy * font->height * framebuffer->pitch) + (cx * (font->width + 1) * sizeof(PIXEL));

  size_type bytesPerGlyphLine = (width + 7) / 8;

  int x, y, line;
  for (y = 0; y < font->height; ++y) {
    line = offs;
    // Calculate address of first byte for glyph
    unsigned char *currentByte = glyph + (bytesPerGlyphLine * y);
    // Get most significant bit
    uint8_t mask = 1 << 7;
    // Display row
    for (x = 0; x < font->width; ++x)
    {
      *((PIXEL*)(framebuffer + line)) = (*currentByte & mask) ? fg : bg;
      mask >>= 1;
      if (!mask) {
        // Already read byte from glyph
        mask = 1<<7;
        // move on to next byte
        ++currentByte;
      }
      line += sizeof(PIXEL);
    }
    offs += framebuffer->pitch;
  }
}

#endif