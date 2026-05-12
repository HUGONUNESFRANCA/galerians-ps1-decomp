#ifndef GHIDRA_TYPES_H
#define GHIDRA_TYPES_H

#include <stdint.h>

typedef uint32_t  uint;
typedef uint16_t  ushort;
typedef uint8_t   uchar;

typedef uint32_t  undefined4;
typedef uint16_t  undefined2;
typedef uint8_t   undefined1;
typedef uint8_t   undefined;

/* undefined6/uint6: quase sempre ponteiro mal interpretado pelo Ghidra.
 * Substituir pelo tipo real quando identificado. */
typedef struct { uint8_t bytes[6]; } undefined6;
typedef struct { uint8_t bytes[6]; } uint6;

#endif /* GHIDRA_TYPES_H */