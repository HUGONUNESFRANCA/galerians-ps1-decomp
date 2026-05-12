#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

/*
 * Sistema de Câmera — Galerians PS1
 * SDK: PsyQ v1.140 (12 Janeiro 1998 — Sony SCEE)
 *
 * Arquitetura:
 *   - Cada sala possui uma tabela de ângulos pré-calculados (g_CameraTable).
 *   - O slot ativo é copiado para g_ActiveCameraMatrix a cada troca de ângulo.
 *   - Camera_LoadToGTE copia a matriz para a scratchpad (0x1F800168) que é
 *     então carregada nos registradores COP2 (GTE) antes da transformação
 *     de vértices.
 *   - Camera_Manager zera os 4 slots de câmera no início de cena/sala.
 *
 * Confiança das funções:
 *   0x80187320  ✅ Camera_LoadToGTE
 *   0x8013b584  ✅ Camera_Manager
 *   0x80187350  ✅ Camera_RecordFrame
 *   0x8018b320  ✅ Room_SetupCameraSlots — init 4 camera angles per room
 */

/* ── Constantes ────────────────────────────────────────────── */
#define CAMERA_SLOT_COUNT            4
#define CAMERA_SLOT_STRIDE           0xC60  /* bytes entre slots em g_CameraSlots */
#define GTE_SCRATCHPAD_ADDR          0x1F800168  /* destino de Camera_LoadToGTE (COP2 source) */
#define CAMERA_HISTORY_STRIDE        0x20   /* bytes por entrada no ring buffer de histórico */
#define CAMERA_HISTORY_DATA_OFFSET   0x04   /* offset até a primeira entrada no buffer */

/* Room_SetupCameraSlots: número de ângulos de câmera por sala */
#define ROOM_CAMERA_ANGLES           4
/* Cor ambiente default escrita em cada slot ao inicializar uma sala */
#define ROOM_AMBIENT_R               0x80
#define ROOM_AMBIENT_G               0x80
#define ROOM_AMBIENT_B               0x80
/* Offsets do "extra data" dentro do camera_ptr — angle-dependent */
#define ROOM_EXTRA_OFFSET_ANGLE0     0x54
#define ROOM_EXTRA_OFFSET_ANGLE1     0x44
#define ROOM_EXTRA_OFFSET_ANGLE2     0x6C
#define ROOM_EXTRA_OFFSET_ANGLE3     0x54

/* ── CameraEntry ───────────────────────────────────────────────────
 * Entrada da tabela de câmeras e struct da câmera ativa.
 *
 * Layout compatível com PsyQ MATRIX (libgte) + campo extra de flag:
 *   +0x00  ✅ rot_matrix[9]  — matriz de rotação 3×3 (int16 fixed-point, 4096=1.0)
 *   +0x12  ✅ _pad           — alinhamento PsyQ (não usar)
 *   +0x14  ✅ pos_x/y/z      — translação no espaço mundo (int32)
 *   +0x20  ✅ active_flag    — (-1) = entrada válida / 0 = vazia
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    int16_t  rot_matrix[9];  /* +0x00: matriz 3×3 row-major (R11..R33) */
    int16_t  _pad;           /* +0x12: padding PsyQ MATRIX */
    int32_t  pos_x;          /* +0x14: translação X */
    int32_t  pos_y;          /* +0x18: translação Y */
    int32_t  pos_z;          /* +0x1C: translação Z */
    int16_t  active_flag;    /* +0x20: -1 = válido, 0 = inativo */
} CameraEntry;

/* ── CameraSlot ────────────────────────────────────────────────────
 * Slot de câmera residente (4 slots consecutivos em g_CameraSlots).
 * Stride fixo de 0xC60 bytes — apenas dois campos confirmados por
 * Camera_Manager:
 *   +0x000  ✅ func_ptr      — ponteiro para função ativa do slot
 *   +0xC3E  ✅ active_flag   — flag resetada pelo Camera_Manager
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    void     *func_ptr;              /* +0x000: callback/handler do slot */
    uint8_t   _unknown[0xC3E - 4];   /* +0x004 – 0xC3D: payload não mapeado */
    int16_t   active_flag;           /* +0xC3E: 0 = slot inativo */
    uint8_t   _tail[CAMERA_SLOT_STRIDE - 0xC40];
} CameraSlot;  /* sizeof = 0xC60 */

/* ── Globais ───────────────────────────────────────────────────── */
#define g_CameraBuffer        ((uint32_t   *)0x801eb560)  /* uint32[8] — fonte de Camera_LoadToGTE */
#define g_CameraSlots         ((CameraSlot *)0x801C1778)  /* 4 × stride 0xC60 */
#define g_CameraFuncPtr       (*(void **   )0x801C2FF4)   /* ponteiro da função ativa global */
#define g_ActiveCameraMatrix  ((CameraEntry *)0x801BFD10) /* matriz ativa — muda a cada frame */
#define g_CameraTable         ((CameraEntry *)0x801C3200) /* tabela pré-definida da sala */
/* ✅ 0x801eb544 — ponteiro para o ring buffer de histórico de frames.
 * O índice de escrita atual (int16) reside em g_CameraHistoryPtr + 2. */
#define g_CameraHistoryPtr    (*(void **    )0x801eb544)
#define g_CameraFrameIndex    (*(int16_t *)((uint8_t *)g_CameraHistoryPtr + 2))

/* ── Funções ───────────────────────────────────────────────────── */

/* 0x80187320 — ✅ Copia 8 × uint32 de g_CameraBuffer para a scratchpad GTE. */
void Camera_LoadToGTE(uint32_t *dst);

/* 0x8013b584 — ✅ Reseta os 4 slots de câmera (active_flag e func_ptr). */
void Camera_Manager(void);

/* 0x80187350 — ✅ Grava frame atual no ring buffer de histórico.
 * Copia 8 × uint32 de g_CameraBuffer para
 *   g_CameraHistoryPtr + CAMERA_HISTORY_DATA_OFFSET + (frame_index * CAMERA_HISTORY_STRIDE)
 * e então incrementa o índice (int16 em g_CameraHistoryPtr + 2). */
void Camera_RecordFrame(void);

/* ── RoomDescriptor ────────────────────────────────────────────────
 * Descritor de sala consumido por Room_SetupCameraSlots (0x8018b320).
 * O parâmetro é tratado como `int*` no código original — uma struct de
 * ponteiros e shorts cujos índices [0..0xd] são preenchidos pela função.
 *
 *   [0..3]  camera_data_ptr[ROOM_CAMERA_ANGLES]   — ptr para frame data por ângulo
 *   [4..7]  geometry_ptr[ROOM_CAMERA_ANGLES]      — ptr para mesh por ângulo
 *   [9]     frame_count (ushort) — angle 0 = *(ushort*)*camera_ptr
 *   [0xa]   frame_data_ptr  = camera_ptr + 4
 *   [0xb]   extra_data_ptr  = camera_ptr + ROOM_EXTRA_OFFSET_ANGLE<n>
 *   [0xc]   geometry_data_ptr = geom_ptr + 4
 *   [0xd]   current_frame angle 0 = 0 (init)
 *
 * Cor ambiente é gravada em runtime nos offsets +0x30/38/50/68 (R), +1 (G),
 * +2 (B), +3 (flag de *(geometry_data + 0xC) byte [3]). Ver MemoryMap.md.
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    int   *camera_data_ptr[ROOM_CAMERA_ANGLES];   /* [0..3] */
    int   *geometry_ptr   [ROOM_CAMERA_ANGLES];   /* [4..7] */
    int    _unused_8;                              /* [8]    desconhecido */
    int    frame_count_a0;                         /* [9]    ushort em LSB */
    int   *frame_data_ptr;                         /* [0xa]  = camera_ptr + 4 */
    int   *extra_data_ptr;                         /* [0xb]  = camera_ptr + ROOM_EXTRA_OFFSET_ANGLE<n> */
    int   *geometry_data_ptr;                      /* [0xc]  = geom_ptr + 4 */
    int    current_frame_a0;                       /* [0xd]  zerado na init */
} RoomDescriptor;

/* 0x8018b320 — ✅ Room_SetupCameraSlots
 * Inicializa 4 ângulos de câmera de uma sala a partir do RoomDescriptor.
 * Liga frame_data + geometry + cor ambiente RGB(0x80,0x80,0x80) e o flag
 * derivado de *(geometry_data + 0xC)[3]. Chamada em toda troca de sala/área.
 * O sub-bloco em 0x8018B3EC é parte interna desta função (não é função
 * separada — escreve nos slots de câmera). */
void Room_SetupCameraSlots(RoomDescriptor *room);

/* ── Port Notes (Fase 2 — PC) ──────────────────────────────────────
 *
 *  Camera_LoadToGTE:
 *    PS1 escreve em 0x1F800168 (scratchpad) o bloco que será movido aos
 *    registradores COP2. No PC, a matriz equivalente deve ser enviada
 *    ao shader corrente como view matrix:
 *        glUniformMatrix4fv(u_view, 1, GL_FALSE, view);
 *    ou via UBO/Push Constant em Vulkan.
 *
 *  Camera_Manager:
 *    Equivalente a resetar o array de câmeras ao carregar uma cena/sala.
 *    No PC pode ser trocado por memset(scene->cameras, 0, sizeof(...)).
 *
 *  Camera_RecordFrame:
 *    Grava o estado atual de g_CameraBuffer no ring buffer de histórico.
 *    Para uma implementação de freecam no PC: interceptar Camera_LoadToGTE
 *    e substituir o conteúdo de g_CameraBuffer pela view matrix calculada
 *    a partir da posição e ângulos controlados pelo jogador antes que
 *    Camera_RecordFrame seja chamada. O ring buffer pode ser ignorado ou
 *    reaproveitado para interpolação suave em transições de sala/cutscene.
 *
 *  g_CameraBuffer / scratchpad:
 *    No PC, a scratchpad não existe — manter apenas o buffer linear
 *    (uint32[8]) em RAM comum e usar diretamente na construção da view.
 */

#endif /* CAMERA_H */
