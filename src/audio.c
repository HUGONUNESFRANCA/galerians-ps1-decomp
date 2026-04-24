#include "include/audio.h"
#include "include/input.h"
#include "include/ghidra_types.h"

#include <stddef.h>

/*
 * ──────────────────────────────────────────────────────────────
 * Galerians PS1 — Sistema de Áudio
 * ──────────────────────────────────────────────────────────────
 *
 * Funções identificadas no dump de RAM (DuckStation → Ghidra).
 * Todos os corpos são stubs documentados — o código real será
 * reconstruído após disassembly completo de cada função.
 *
 * Dispatch:
 *   g_AudioVtable @ 0x801AC830 (7+ entradas) — tabela de despacho
 *   das funções de áudio do jogo. Audio_SetChannel e Video_SetMode
 *   são, na verdade, wrappers para as tabelas B/C do BIOS PS1: o
 *   caller carrega t1 com o índice da função BIOS e chama o wrapper.
 *
 * BIOS / Wrappers:
 *   0x8017e040  Video_SetMode    ✅  (BIOS table C @ 0x000000C0)
 *   0x8017e050  Audio_SetChannel ✅  (BIOS table B @ 0x000000B0)
 *
 * SPU Core:
 *   0x80183a00  SPU_SetVoiceField  ✅  escreve voice+0x28 e voice+0x34
 *   0x80184898  SPU_SetVolume      🟡  3 SPU refs
 *   0x801834B4  SPU_SetADSR        🟡  2 SPU refs
 *   0x80177328  SPU_KeyOnOff       🟡  4 SPU refs
 *
 * Nota: 0x80187DF4 foi inicialmente rotulado aqui como SPU_CoreDriver
 * (por contagem de acessos a registradores SPU). Mapeamento posterior
 * mostrou que é na verdade Engine_Init — ver engine_state.h / MemoryMap.md.
 *
 * Debug:
 *   0x8017d558  Debug_Print        ✅  ("VSync: timeout")
 *
 * Rumble:
 *   0x80182bdc  SetRumble          🟡
 *   0x80182b5c  SetRumbleMode      🟡
 *   0x8018281c  GetRumbleState     🟡
 *
 * Contexto de inicialização — StateSlot_Allocate:
 *   Video_SetMode(0)                    → slot 0 (FMV/Vídeo)
 *   Audio_SetChannel(AUDIO_CH_FMV,   1) → slot 0
 *   Audio_SetChannel(AUDIO_CH_BGM,   1) → slot 4
 *   Audio_SetChannel(AUDIO_CH_SFX,   1) → slot 5
 *   Audio_SetChannel(AUDIO_CH_VOICE, 1) → slot 6
 *
 * Hardware:
 *   SPU: 24 vozes ADPCM, RAM 512 KB, base 0x1F801C00.
 *   DMA tables em 0x80194078 e 0x80195FD0.
 *   Rumble DualShock: pacote SIO montado em g_Controllers[2] (0x801AC980).
 *
 * Globais relacionados:
 *   0x80195C10  g_FrameCounter (uint32_t) — incrementado por VBlank
 *   0x801AC830  g_AudioVtable  (void*[7+]) — dispatch table
 *   0x8011A174  Sound tables   — 5 arrays de ponteiros (164/111/102/100/92)
 * ──────────────────────────────────────────────────────────────
 */


/* ────────────────────────────────────────────────────────────────
 * Video_SetMode — 0x8017e040  ✅ Confirmado
 *
 * Wrapper para a tabela C do BIOS PS1 (0x000000C0). O caller carrega
 * o registrador t1 com o índice da função BIOS desejada e chama esta
 * rotina, que executa um indirect jump via tabela.
 *
 * Chamado pelo init do slot 0 (FMV/Vídeo) em StateSlot_Allocate.
 * Provavelmente programa o modo MDEC para streaming de FMV.
 *
 * PORT NOTE (PC):
 *   Inútil no port — substituir por no-op. O decoder de FMV do PC
 *   (avcodec) gerencia o próprio estado.
 * ─────────────────────────────────────────────────────────────── */
void Video_SetMode(int mode)
{
    /* TODO: reconstrução pendente. */
    (void)mode;
}


/* ────────────────────────────────────────────────────────────────
 * Audio_SetChannel — 0x8017e050  ✅ Confirmado
 *
 * Wrapper para a tabela B do BIOS PS1 (0x000000B0). O caller carrega
 * o registrador t1 com o índice da função BIOS de áudio (key on/off,
 * set volume, etc) e chama esta rotina.
 *
 * Padrão de chamada confirmado em StateSlot_Allocate (via PsyQ_VSync,
 * que é o caller comum de FUN_8017e040 e FUN_8017e050):
 *   Audio_SetChannel(AUDIO_CH_BGM,   1)  → slot 4
 *   Audio_SetChannel(AUDIO_CH_SFX,   1)  → slot 5
 *   Audio_SetChannel(AUDIO_CH_VOICE, 1)  → slot 6
 *   Audio_SetChannel(AUDIO_CH_FMV,   1)  → slot 0
 *
 * channel: AUDIO_CH_BGM(0) / SFX(1) / VOICE(2) / FMV(3)
 * enable:  1 = ativar, 0 = desativar
 *
 * PORT NOTE (PC):
 *   Mapear cada canal lógico para um channel group de SDL_Mixer:
 *       Mix_GroupChannel(ch, channel);   // enable
 *       Mix_HaltGroup(channel);          // disable
 * ─────────────────────────────────────────────────────────────── */
void Audio_SetChannel(int channel, int enable)
{
    /* TODO: reconstrução pendente. */
    (void)channel;
    (void)enable;
}


/* ────────────────────────────────────────────────────────────────
 * SPU_SetVoiceField — 0x80183a00  ✅ Confirmado
 *
 * Escreve dois campos de uma struct de voz mantida em RAM principal:
 *   voice_ptr + 0x28  ← data
 *   voice_ptr + 0x34  ← flag
 *
 * Os offsets (0x28 / 0x34) estão muito além do stride de 0x10 dos
 * registradores SPU de hardware — então voice_ptr aponta para uma
 * struct shadow em RAM, não para o hardware. O tick do SPU consome
 * esse shadow a cada frame.
 *
 * PORT NOTE (PC):
 *   Manter o shadow state como struct C; o equivalente OpenAL é
 *   aplicado no tick do SPU, não aqui.
 * ─────────────────────────────────────────────────────────────── */
void SPU_SetVoiceField(void *voice_ptr, uint32_t data, uint32_t flag)
{
    /* TODO: reconstrução pendente. */
    (void)voice_ptr;
    (void)data;
    (void)flag;
}


/* ────────────────────────────────────────────────────────────────
 * SPU_SetVolume — 0x80184898  🟡 Alta confiança
 *
 * 3 acessos a registradores SPU. Provavelmente programa vol_left /
 * vol_right de uma voz (offsets +0x00 / +0x02) ou o volume master.
 * Assinatura exata a confirmar — usando o caso per-voice como hipótese.
 *
 * PORT NOTE (PC):
 *   alSourcef(al_sources[voice], AL_GAIN, max(vl, vr) / 32767.f);
 *   ou usar OpenAL EFX para pan stereo se vl != vr.
 * ─────────────────────────────────────────────────────────────── */
void SPU_SetVolume(int voice, int vol_left, int vol_right)
{
    /* TODO: reconstrução pendente. */
    (void)voice;
    (void)vol_left;
    (void)vol_right;
}


/* ────────────────────────────────────────────────────────────────
 * SPU_SetADSR — 0x801834B4  🟡 Alta confiança
 *
 * 2 acessos a registradores SPU. Programa adsr_lo (+0x08) e adsr_hi
 * (+0x0A) de uma voz — o envelope ADSR do hardware.
 *
 * PORT NOTE (PC):
 *   OpenAL não tem ADSR nativo. Implementar em software:
 *       Aplicar uma curva sobre AL_GAIN por frame, com base nos
 *       parâmetros (attack rate, decay rate, sustain level, release).
 *       Pré-renderizar a curva PCM se a voz não muda de ADSR.
 * ─────────────────────────────────────────────────────────────── */
void SPU_SetADSR(int voice, uint16_t adsr_lo, uint16_t adsr_hi)
{
    /* TODO: reconstrução pendente. */
    (void)voice;
    (void)adsr_lo;
    (void)adsr_hi;
}


/* ────────────────────────────────────────────────────────────────
 * SPU_KeyOnOff — 0x80177328  🟡 Alta confiança
 *
 * 4 acessos a registradores SPU. Provavelmente bate em SPU_KEY_ON
 * (0x1F801D88, 24-bit mask) e SPU_KEY_OFF (0x1F801D8C). Cada bit da
 * máscara corresponde a uma voz.
 *
 * PORT NOTE (PC):
 *   for (int v = 0; v < SPU_VOICE_COUNT; v++) {
 *       if (key_on_mask  & (1u << v)) alSourcePlay(al_sources[v]);
 *       if (key_off_mask & (1u << v)) alSourceStop(al_sources[v]);
 *   }
 * ─────────────────────────────────────────────────────────────── */
void SPU_KeyOnOff(uint32_t key_on_mask, uint32_t key_off_mask)
{
    /* TODO: reconstrução pendente. */
    (void)key_on_mask;
    (void)key_off_mask;
}


/* ────────────────────────────────────────────────────────────────
 * Debug_Print — 0x8017d558  ✅ Confirmado
 *
 * Função de print de debug — confirmada porque PsyQ_VSync chama
 * Debug_Print("VSync: timeout") quando o VBlank wait excede o limite.
 * Tipo de saída (TTY do PsyQ / printf interno) ainda não determinado.
 *
 * PORT NOTE (PC):
 *   fprintf(stderr, "%s\n", string);
 *   ou redirecionar para o logger do port.
 * ─────────────────────────────────────────────────────────────── */
void Debug_Print(const char *string)
{
    /* TODO: reconstrução pendente. */
    (void)string;
}


/* ────────────────────────────────────────────────────────────────
 * SetRumble — 0x80182bdc  🟡 Alta confiança
 *
 * Controla os motores de vibração do DualShock.
 * O DualShock possui dois motores:
 *   Motor pequeno (canal 0): digital on/off — data = 0 ou 1
 *   Motor grande  (canal 1): analógico 0x00–0xFF (intensidade)
 *
 * Os bytes de controle são escritos no scratchpad
 * g_Controllers[2] (0x801AC980, campo rumble_data +0x24) e
 * enviados ao controle via SIO no próximo ciclo de poll.
 *
 * PORT NOTE (PC):
 *   XInput:
 *       XINPUT_VIBRATION vib = { lo_speed, hi_speed };
 *       XInputSetState(controller_index, &vib);
 *   SDL2:
 *       SDL_GameControllerRumble(ctrl, lo_freq, hi_freq, duration_ms);
 * ─────────────────────────────────────────────────────────────── */
void SetRumble(int channel, uint32_t data, int mode)
{
    /* TODO: reconstrução pendente. */
    (void)channel;
    (void)data;
    (void)mode;
}


/* ────────────────────────────────────────────────────────────────
 * SetRumbleMode — 0x80182b5c  🟡 Alta confiança
 *
 * Configura o comportamento do rumble via ponteiro para uma struct
 * de parâmetros (layout ainda não mapeado). Provavelmente oferece
 * controle mais fino do que SetRumble — duração, padrão pulsado,
 * ou seleção entre motores via campos da struct.
 *
 * PORT NOTE (PC):
 *   Implementar como wrapper sobre XInputSetState / SDL_GameControllerRumble
 *   com duração gerenciada por um timer do lado do port.
 * ─────────────────────────────────────────────────────────────── */
void SetRumbleMode(int channel, void *ptr)
{
    /* TODO: reconstrução pendente. */
    (void)channel;
    (void)ptr;
}


/* ────────────────────────────────────────────────────────────────
 * GetRumbleState — 0x8018281c  🟡 Alta confiança
 *
 * Retorna o estado atual do motor de rumble para um canal.
 * Provavelmente lê o campo vibration_active (+0x38) ou
 * rumble_data (+0x24) de g_Controllers[2] (0x801AC980).
 *
 * Retorno: 0 = motor parado, != 0 = motor ativo.
 *
 * PORT NOTE (PC):
 *   Manter uma variável de estado local atualizada por SetRumble /
 *   SetRumbleMode. XInput não expõe o estado do motor — o tracking
 *   precisa ser feito do lado da aplicação.
 * ─────────────────────────────────────────────────────────────── */
int GetRumbleState(int channel)
{
    /* TODO: reconstrução pendente. */
    (void)channel;
    return 0;
}
