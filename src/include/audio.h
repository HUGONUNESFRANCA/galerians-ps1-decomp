#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/*
 * Sistema de Áudio — Galerians PS1
 * SDK: PsyQ v1.140 (12 Janeiro 1998 — Sony SCEE)
 *
 * Arquitetura:
 *   - SPU PS1: 24 vozes ADPCM de hardware, RAM dedicada de 512 KB.
 *   - Áudio em ADPCM 4-bit (44.1 kHz ou taxa reduzida por voz).
 *   - Streaming de FMV usa XA-ADPCM via CD-ROM direto — não passa
 *     pela SPU RAM (canal separado no hardware do CD-ROM).
 *   - O jogo expõe 4 canais lógicos, cada um gerenciado por uma
 *     corrotina num slot da state machine.
 *   - O rumble do DualShock é enviado via SIO com pacote montado
 *     no scratchpad g_Controllers[2] (0x801AC980).
 *
 * Dispatch:
 *   O áudio é despachado por uma vtable de 7+ entradas em
 *   g_AudioVtable (0x801AC830). Audio_SetChannel (0x8017e050) é
 *   um wrapper para a tabela B do BIOS (0x000000B0): o caller carrega
 *   t1 com o índice da função BIOS desejada. Video_SetMode (0x8017e040)
 *   é o wrapper análogo para a tabela C do BIOS.
 *   PsyQ_VSync — caller de FUN_8017e040/e050 — confirma a semântica
 *   dos 4 canais lógicos (BGM / SFX / Voice / FMV).
 *
 * Confiança das funções:
 *   0x8017e040  ✅ Video_SetMode      (BIOS table C wrapper)
 *   0x8017e050  ✅ Audio_SetChannel   (BIOS table B wrapper)
 *   0x80183a00  ✅ SPU_SetVoiceField
 *   0x80184898  🟡 SPU_SetVolume      (3 SPU refs)
 *   0x801834B4  🟡 SPU_SetADSR        (2 SPU refs)
 *   0x80177328  🟡 SPU_KeyOnOff       (4 SPU refs)
 *   0x8017d558  ✅ Debug_Print        ("VSync: timeout")
 *   0x80182bdc  🟡 SetRumble
 *   0x80182b5c  🟡 SetRumbleMode
 *   0x8018281c  🟡 GetRumbleState
 */

/* ── Canais de Áudio Lógicos ─────────────────────────────────── */
#define AUDIO_CH_BGM    0   /* Background Music */
#define AUDIO_CH_SFX    1   /* Efeitos sonoros */
#define AUDIO_CH_VOICE  2   /* Voz / diálogo */
#define AUDIO_CH_FMV    3   /* Streaming XA-ADPCM de FMV */
#define AUDIO_CH_COUNT  4

/* Slot da state machine correspondente a cada canal:
 *   Slot 0 = FMV   — Video_SetMode + Audio_SetChannel(AUDIO_CH_FMV, 1)
 *   Slot 4 = BGM   — Audio_SetChannel(AUDIO_CH_BGM,   1)
 *   Slot 5 = SFX   — Audio_SetChannel(AUDIO_CH_SFX,   1)
 *   Slot 6 = Voice — Audio_SetChannel(AUDIO_CH_VOICE, 1)
 */
#define AUDIO_SLOT_FMV    0
#define AUDIO_SLOT_BGM    4
#define AUDIO_SLOT_SFX    5
#define AUDIO_SLOT_VOICE  6

/* ── BIOS Function Tables (PS1 kernel) ───────────────────────── */
#define BIOS_TABLE_B_ADDR  0x000000B0u  /* usado por Audio_SetChannel via t1 */
#define BIOS_TABLE_C_ADDR  0x000000C0u  /* usado por Video_SetMode  via t1 */

/* ── SPU — Hardware PS1 ──────────────────────────────────────── */
#define SPU_VOICE_COUNT      24          /* vozes de hardware disponíveis */
#define SPU_VOICE_STRIDE     0x10        /* bytes por bloco de registradores */
#define SPU_RAM_SIZE         0x80000     /* 512 KB de RAM SPU dedicada */
#define SPU_BASE_ADDR        0x1F801C00  /* base dos registradores de voz */
#define SPU_CTRL_ADDR        0x1F801D80  /* SPUCNT — controle global */
#define SPU_STAT_ADDR        0x1F801D82  /* SPUSTAT — status (read-only) */
#define SPU_TRANS_ADDR       0x1F801DA6  /* endereço DMA de transferência */
#define SPU_CD_VOL_L         0x1F801DB0  /* volume CD-audio esquerdo */
#define SPU_CD_VOL_R         0x1F801DB2  /* volume CD-audio direito */
#define SPU_XA_VOL_L         0x1F801DB4  /* volume XA-ADPCM esquerdo */
#define SPU_XA_VOL_R         0x1F801DB6  /* volume XA-ADPCM direito */

/* Bits do SPUCNT (0x1F801D80) */
#define SPU_CTRL_ENABLE       0x8000  /* liga o SPU */
#define SPU_CTRL_MUTE         0x4000  /* muta todas as vozes */
#define SPU_CTRL_REVERB_MASTER 0x0080 /* reverb global on/off */
#define SPU_CTRL_DMA_WRITE    0x0020  /* modo DMA: escrita na SPU RAM */
#define SPU_CTRL_DMA_READ     0x0010  /* modo DMA: leitura da SPU RAM */
#define SPU_CTRL_CDDA_REVERB  0x0004  /* envia CD-audio pelo reverb */
#define SPU_CTRL_CDDA_ENABLE  0x0001  /* habilita CD-audio no mixer SPU */

/* Pitch: 4096 = 44100 Hz (sample rate base do SPU) */
#define SPU_PITCH_BASE        4096

/* ── SpuVoiceRegs — Registradores de voz do SPU ───────────────────
 * Layout dos 0x10 bytes por voz em hardware.
 * Endereço de voz N: SPU_BASE_ADDR + N * SPU_VOICE_STRIDE
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    int16_t   vol_left;    /* +0x00: volume esquerdo (signed) */
    int16_t   vol_right;   /* +0x02: volume direito (signed) */
    uint16_t  pitch;       /* +0x04: sample rate (4096 = 44100 Hz) */
    uint16_t  start_addr;  /* +0x06: endereço na SPU RAM >> 3 */
    uint16_t  adsr_lo;     /* +0x08: attack / decay / sustain mode */
    uint16_t  adsr_hi;     /* +0x0A: sustain level / release rate */
    int16_t   cur_vol;     /* +0x0C: volume corrente (read-only) */
    uint16_t  repeat_addr; /* +0x0E: endereço de loop na SPU RAM >> 3 */
} SpuVoiceRegs;  /* sizeof = 0x10 */

/* ── SpuVoiceStruct — Estado de voz em RAM principal ──────────────
 * Layout de 32 bytes mantido pelo driver SPU em RAM principal.
 * Distinto de SpuVoiceRegs (registradores de hardware, 0x10 bytes).
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    uint16_t vol_r_or_pitch_base; // +0x00 (default 0x1000)
    uint16_t unknown_02;
    uint16_t unknown_04;
    uint16_t unknown_06;
    uint16_t vol_l;               // +0x08 (default 0x1000)
    uint16_t unknown_0A;
    uint16_t unknown_0C;
    uint16_t unknown_0E;
    uint16_t pitch;               // +0x10 (4096 = 44100Hz)
    uint8_t  cleared_pad[14];     // +0x12 to +0x1F (cleared)
} SpuVoiceStruct; // 32 bytes

/* ── SoundTableEntry — Entrada das tabelas de som ─────────────────
 * Layout de 16 bytes apontado pelas tabelas em 0x8011A164 / 0x8011A174.
 * Cada entrada referencia um asset de áudio carregado na SPU RAM,
 * mais parâmetros associados (volume, loop, flags).
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    void *audio_data;     /* +0x00 ptr para audio data (VAG na SPU RAM) */
    void *volume_params;  /* +0x04 ptr para volume/params (e.g. 0x7C) */
    void *loop_data;      /* +0x08 ptr para loop/dados adicionais */
    void *flags;          /* +0x0C ptr para flags ou parâmetros extras */
} SoundTableEntry;  /* sizeof = 0x10 */

/* ── Globais ───────────────────────────────────────────────────── */

/* 0x80195C10 — g_FrameCounter: contador de frames incrementado a cada
 * VBlank pelo handler de PsyQ_VSync. Usado para timing de eventos
 * de áudio e sincronização de FMV. */
#define g_FrameCounter  (*(volatile uint32_t *)0x80195C10u)

/* 0x801AC830 — g_AudioVtable: tabela de despacho do sistema de áudio.
 * 7+ ponteiros de função; entradas confirmadas:
 *   [0] 0x80186938
 *   [1] 0x80186A84
 *   [2] 0x80186D00
 *   [3] 0x80186D38
 *   [4] 0x80186D90
 *   [5] 0x801871E4
 * O índice [6] e além ainda não foram dumpados. */
#define g_AudioVtable   ((void **)0x801AC830u)
#define AUDIO_VTABLE_KNOWN_ENTRIES  6

/* 0x80194078, 0x80195FD0 — DMA tables do SPU (transferências em bloco
 * para a SPU RAM). Layout exato a mapear. */
#define SPU_DMA_TABLE_A_ADDR  0x80194078u
#define SPU_DMA_TABLE_B_ADDR  0x80195FD0u

/* 0x8011A174 – 0x8011B07C — 5 arrays de ponteiros consecutivos
 * (164, 111, 102, 100 e 92 entradas). Candidatos fortes a tabelas de
 * índice de SFX e BGM. A divisão por canal lógico está pendente de
 * confirmação no Ghidra. */
#define SOUND_TABLE_BASE      0x8011A174u
#define SOUND_TABLE_END       0x8011B07Cu
#define SOUND_TABLE_COUNT     5
/* contagens por tabela (em ordem de endereço) */
#define SOUND_TABLE_0_ENTRIES 164
#define SOUND_TABLE_1_ENTRIES 111
#define SOUND_TABLE_2_ENTRIES 102
#define SOUND_TABLE_3_ENTRIES 100
#define SOUND_TABLE_4_ENTRIES  92

/* Ponteiros globais para as tabelas de som. g_SoundTableBase aponta
 * para o início do bloco de 5 tabelas (0x8011A164); g_SFXTable é a
 * primeira tabela utilizável (0x8011A174, 164 entradas). */
extern SoundTableEntry *g_SoundTableBase;  /* 0x8011A164 */
extern SoundTableEntry *g_SFXTable;        /* 0x8011A174 (164 entradas) */

/* 0x801AC980 — g_Controllers[2]: terceiro canal do array de controle
 * (base 0x801AC900, stride 0x40). Usado como scratchpad para o pacote
 * de rumble enviado ao DualShock via SIO. Os campos relevantes são:
 *   +0x24  uint32_t rumble_data  — bytes de controle dos motores
 *   +0x38  uint8_t  vibration_active */
#define RUMBLE_SCRATCHPAD_ADDR  0x801AC980u

/* ── Funções ───────────────────────────────────────────────────── */

/* 0x8011d198 — SoundManager_Init(): central audio asset loader.
 * Inicializa o sistema de som carregando as tabelas em 0x8011A164 /
 * 0x8011A174 e configurando o estado base do driver SPU. */
void SoundManager_Init(void);

/* 0x8017e040 — ✅ Video_SetMode(mode): wrapper para a tabela C do BIOS
 * (0x000000C0). Carrega t1 com o índice da função BIOS desejada e salta.
 * Chamado pelo init do slot 0 (FMV/Vídeo). */
void Video_SetMode(int mode);

/* 0x8017e050 — ✅ Audio_SetChannel(channel, enable): wrapper para a
 * tabela B do BIOS (0x000000B0). channel: AUDIO_CH_*; enable: 1=ativar,
 * 0=desativar. Confirmado por PsyQ_VSync. */
void Audio_SetChannel(int channel, int enable);

/* 0x80183a00 — ✅ SPU_SetVoiceField(voice_ptr, data, flag):
 * escreve param_2 em voice+0x28 e param_3 em voice+0x34.
 * Os offsets +0x28 / +0x34 indicam que voice_ptr não aponta para os
 * registradores de hardware (stride 0x10) e sim para uma struct de
 * estado de voz mantida em RAM principal. */
void SPU_SetVoiceField(void *voice_ptr, uint32_t data, uint32_t flag);

/* 0x80184898 — 🟡 SPU_SetVolume(): 3 acessos a registradores SPU.
 * Provavelmente ajusta volume master ou per-voice. Assinatura exata
 * a confirmar. */
void SPU_SetVolume(int voice, int vol_left, int vol_right);

/* 0x801834B4 — 🟡 SPU_SetADSR(): 2 acessos a registradores SPU.
 * Programa adsr_lo / adsr_hi de uma voz. */
void SPU_SetADSR(int voice, uint16_t adsr_lo, uint16_t adsr_hi);

/* 0x80177328 — 🟡 SPU_KeyOnOff(): 4 acessos a registradores SPU.
 * Provavelmente bate em SPU_KEY_ON / SPU_KEY_OFF (0x1F801D88 / D8C). */
void SPU_KeyOnOff(uint32_t key_on_mask, uint32_t key_off_mask);

/* 0x8017d558 — ✅ Debug_Print(string): print de debug.
 * Confirmado pela string "VSync: timeout" passada como argumento. */
void Debug_Print(const char *string);

/* 0x80182bdc — 🟡 Aciona os motores de vibração do DualShock.
 * channel: 0 = motor pequeno (on/off), 1 = motor grande (0x00–0xFF)
 * data:    intensidade/valor do motor
 * mode:    modo de operação (hipótese — contínuo / timed) */
void SetRumble(int channel, uint32_t data, int mode);

/* 0x80182b5c — 🟡 Configura o modo do rumble via struct apontada por ptr.
 * channel: índice do motor DualShock
 * ptr:     ponteiro para struct de configuração (layout a mapear) */
void SetRumbleMode(int channel, void *ptr);

/* 0x8018281c — 🟡 Retorna o estado atual do motor de rumble.
 * channel: índice do motor DualShock
 * Retorno: 0 = parado, != 0 = ativo (tipo exato a confirmar) */
int GetRumbleState(int channel);

/* ── Port Notes (Fase 2 — PC) ──────────────────────────────────────
 *
 *  Audio_SetChannel  → SDL_Mixer channel enable/disable
 *      Mapear cada canal lógico (BGM/SFX/Voice/FMV) para um channel
 *      group de SDL_Mixer (Mix_GroupChannel + Mix_HaltGroup).
 *
 *  Video_SetMode     → desnecessário no port; substituído pelo decoder
 *      de FMV do PC (avcodec). Manter como no-op.
 *
 *  SPU voices        → OpenAL alGenSources + alSourcei por voz:
 *      alGenSources(SPU_VOICE_COUNT, al_sources);
 *      Um tick por frame reprograma pitch/volume/loop de cada source.
 *
 *  SPU_KeyOnOff      → alSourcePlay / alSourceStop (por bit de máscara).
 *
 *  SPU_SetVolume     → alSourcef(src, AL_GAIN, vol).
 *
 *  SPU_SetADSR       → ADSR em software (sem equivalente direto em OpenAL):
 *      Aplicar a envelope sobre AL_GAIN a cada frame; opcionalmente
 *      pré-renderizar a curva no buffer PCM se a voz não muda de ADSR.
 *
 *  SPU_SetVoiceField → escrever no shadow state em RAM; o tick do
 *      SPU propaga para a source OpenAL no próximo frame.
 *
 *  g_FrameCounter    → PC frame counter via QueryPerformanceCounter
 *      (ou std::chrono::steady_clock no C++). Atualizar por VSync do PC.
 *
 *  Sound tables em 0x8011A174 → carregar como índice de assets PC:
 *      sound_table[i] vira um path para arquivo OGG/WAV no disco
 *      ou um ID em um asset bundle. As 5 tabelas viram 5 categorias.
 *
 *  ADPCM → PCM16:
 *      Decodificar offline (no asset pipeline) e shipar OGG/WAV.
 *      Ver libavcodec ou um decoder ADPCM PS1 embutido.
 *
 *  XA-ADPCM (FMV) → streaming OpenAL:
 *      Extrair setores XA do .bin/.iso com libcdio.
 *      Decodificar XA-ADPCM (estéreo, 37800 Hz ou 18900 Hz).
 *      Stream para OpenAL via buffer circular (AL_STREAMING).
 *
 *  Rumble → XInput / SDL2:
 *      SetRumble / SetRumbleMode / GetRumbleState substituem-se por:
 *          XInputSetState(0, &XINPUT_VIBRATION{lo, hi});  // Win32
 *          SDL_GameControllerRumble(ctrl, lo, hi, ms);    // SDL2
 *      O scratchpad em RUMBLE_SCRATCHPAD_ADDR pode ser descartado.
 *
 *  Debug_Print       → fprintf(stderr, ...) ou logger do port.
 */

#endif /* AUDIO_H */
