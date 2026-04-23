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
 * Confiança das funções:
 *   0x8017e040  🟠 Audio_SetMode  (pode ser Video_SetMode — slot 0)
 *   0x8017e050  🟡 Audio_SetChannel
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
 *   Slot 0 = FMV   — Audio_SetMode + Audio_SetChannel(AUDIO_CH_FMV, 1)
 *   Slot 4 = BGM   — Audio_SetChannel(AUDIO_CH_BGM,   1)
 *   Slot 5 = SFX   — Audio_SetChannel(AUDIO_CH_SFX,   1)
 *   Slot 6 = Voice — Audio_SetChannel(AUDIO_CH_VOICE, 1)
 */
#define AUDIO_SLOT_FMV    0
#define AUDIO_SLOT_BGM    4
#define AUDIO_SLOT_SFX    5
#define AUDIO_SLOT_VOICE  6

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

/* ── Globais ───────────────────────────────────────────────────── */

/* 0x801AC980 — g_Controllers[2]: terceiro canal do array de controle
 * (base 0x801AC900, stride 0x40). Usado como scratchpad para o pacote
 * de rumble enviado ao DualShock via SIO. Os campos relevantes são:
 *   +0x24  uint32_t rumble_data  — bytes de controle dos motores
 *   +0x38  uint8_t  vibration_active */
#define RUMBLE_SCRATCHPAD_ADDR  0x801AC980u

/* ── Funções ───────────────────────────────────────────────────── */

/* 0x8017e040 — 🟠 Inicialização do slot 0 (FMV/Vídeo).
 * Nome incerto: pode ser Video_SetMode ou Audio_SetMode.
 * Chamado por StateSlot_Allocate durante a inicialização do slot 0. */
void Audio_SetMode(void);

/* 0x8017e050 — 🟡 Ativa ou desativa um canal de áudio lógico.
 * channel: AUDIO_CH_BGM(0) / SFX(1) / VOICE(2) / FMV(3)
 * param:   1 = ativar, 0 = desativar (hipótese) */
void Audio_SetChannel(int channel, int param);

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
 *  SPU (24 vozes) → OpenAL:
 *    Cada voz SPU mapeia para uma AL source:
 *        alGenSources(SPU_VOICE_COUNT, al_sources);
 *    Pitch: voice.pitch codifica a taxa relativa à 44100 Hz.
 *        float al_pitch = (float)voice.pitch / SPU_PITCH_BASE;
 *        alSourcef(src, AL_PITCH, al_pitch);
 *    O ADSR de hardware deve ser simulado via AL_GAIN por frame.
 *
 *  ADPCM → PCM16:
 *    Todo áudio PS1 está em ADPCM 4-bit (blocos de 16 bytes = 28 amostras).
 *    Decodificar para PCM16 antes de criar o buffer OpenAL:
 *        alBufferData(buf, AL_FORMAT_MONO16, pcm, size, sample_rate);
 *    Usar libavcodec (FFmpeg) ou um decoder ADPCM embutido.
 *
 *  XA-ADPCM (FMV) → streaming OpenAL:
 *    XA-ADPCM não passa pela SPU RAM — é lido diretamente do CD-ROM
 *    em setores Mode2/Form2. No PC:
 *        Extrair setores XA do .bin/.iso com libcdio.
 *        Decodificar XA-ADPCM (estéreo, 37800 Hz ou 18900 Hz).
 *        Fazer stream para OpenAL via buffer circular (AL_STREAMING).
 *
 *  Audio_SetChannel → mixer track:
 *    Equivale a registrar/destruir um mixer track por canal.
 *    No PC: alGenSources por canal lógico, ou SDL_mixer channel.
 *
 *  Rumble → XInput / SDL2:
 *    SetRumble / SetRumbleMode / GetRumbleState substituem-se por:
 *        XInputSetState(0, &XINPUT_VIBRATION{lo, hi});  // Win32
 *        SDL_GameControllerRumble(ctrl, lo, hi, ms);    // SDL2
 *    O scratchpad em RUMBLE_SCRATCHPAD_ADDR pode ser descartado no port.
 */

#endif /* AUDIO_H */
