#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

/*
 * Sistema de Câmera — Galerians PS1
 *
 * Câmera ativa:   0x801BFD10  (g_ActiveCamera)
 * Tabela:         0x801C3200  (g_CameraTable)
 * Camera_LoadToGTE: 0x80187338
 *
 * O PS1 usa a GTE (Geometry Transformation Engine, COP2) para
 * transformar vértices. Camera_LoadToGTE escreve os registradores
 * GTE R11-R33 (matriz de rotação) e TRX/TRY/TRZ (translação) a
 * partir do início da struct — portanto GTEMatrix DEVE estar em +0x00.
 *
 * Ângulos PS1: int16_t em [0, 4095], onde 4096 == 360°.
 * Matriz GTE:  int16_t fixed-point, onde 4096 == 1.0.
 */

/* ── Matriz GTE (PsyQ MATRIX) ──────────────────────────────────────
 * Stride: 0x20 bytes (32)
 * Layout idêntico ao PsyQ libgte MATRIX.
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    int16_t  m[3][3];   /* +0x00: matriz de rotação 3×3 (fixed-point 4096=1.0) */
    int16_t  _pad;      /* +0x12: alinhamento PsyQ (não usar) */
    int32_t  t[3];      /* +0x14: vetor de translação (espaço câmera) */
} GTEMatrix;            /* sizeof = 0x20 */

/* ── CameraEntry ───────────────────────────────────────────────────
 * Entrada na tabela de câmeras e struct da câmera ativa.
 *
 * Confiança dos campos:
 *   +0x00  ✅ GTEMatrix     — confirmado por Camera_LoadToGTE
 *   +0x20  🟡 pos_x/y/z    — posição mundo (int32), hipótese de layout
 *   +0x2C  🟡 rot_x/y/z    — ângulos Euler PS1 (int16)
 *   +0x32  🔴 flags        — propósito desconhecido
 *   +0x34  🔴 camera_id    — hipótese: índice na tabela
 *   +0x36  ??  _unknown    — gap não mapeado
 * ─────────────────────────────────────────────────────────────── */
typedef struct {

    /* +0x00 — carregado diretamente nos registradores GTE */
    GTEMatrix  gte;         /* 0x20 bytes */

    /* +0x20 — posição da câmera no espaço mundo */
    int32_t    pos_x;       /* 0x801BFD30 (ativa) */
    int32_t    pos_y;       /* 0x801BFD34 */
    int32_t    pos_z;       /* 0x801BFD38 */

    /* +0x2C — orientação em ângulos Euler (0..4095, 4096=360°) */
    int16_t    rot_x;       /* 0x801BFD3C */
    int16_t    rot_y;       /* 0x801BFD3E */
    int16_t    rot_z;       /* 0x801BFD40 */

    /* +0x32 — campos não confirmados */
    uint16_t   flags;       /* 0x801BFD42 — 🔴 desconhecido */
    uint16_t   camera_id;  /* 0x801BFD44 — 🔴 hipótese: índice na tabela */

    uint8_t    _unknown[0x16]; /* +0x36: gap até 0x4C — não mapeado */

} CameraEntry;              /* sizeof hipotético = 0x4C */

/* ── Globais ───────────────────────────────────────────────────── */
#define g_ActiveCamera  ((CameraEntry *)0x801BFD10)
#define g_CameraTable   ((CameraEntry *)0x801C3200)

/* ── Funções ───────────────────────────────────────────────────── */

/* 0x80187338 — copia GTEMatrix da câmera nos registradores GTE */
void Camera_LoadToGTE(const CameraEntry *cam);

/* helpers reconstruídos — endereços a confirmar */
void Camera_SetActive(int index);
void Camera_BuildMatrix(CameraEntry *cam);

#endif /* CAMERA_H */
