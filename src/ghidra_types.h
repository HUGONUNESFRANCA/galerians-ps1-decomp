#ifndef GHIDRA_TYPES_H //Isso são Include Guards e eles impedem o compilador de ler esse arquivo duas vezes e dê erro de duplicação
#define GHIDRA_TYPES_H //caso eu o incluo em vários lugares.

// Dicionário de tradução: Tipos do Ghidra -> C Padrão
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

// Tipos "Undefined" (O Ghidra sabe o tamanho, mas não o que é)
typedef unsigned int   undefined4; // 4 bytes (como um int ou ponteiro)
typedef unsigned short undefined2; // 2 bytes (como um short)
typedef unsigned char  undefined1; // 1 byte  (como um char ou booleano)
typedef unsigned char  undefined;

#endif // GHIDRA_TYPES_H