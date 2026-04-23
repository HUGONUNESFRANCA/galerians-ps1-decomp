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
 *   0x8017e040  Audio_SetMode      🟠  (pode ser Video_SetMode)
 *   0x8017e050  Audio_SetChannel   🟡
 *   0x80182bdc  SetRumble          🟡
 *   0x80182b5c  SetRumbleMode      🟡
 *   0x8018281c  GetRumbleState     🟡
 *
 * Contexto de inicialização — StateSlot_Allocate:
 *   Audio_SetMode()               → slot 0 (FMV/Vídeo)
 *   Audio_SetChannel(AUDIO_CH_FMV,   1) → slot 0
 *   Audio_SetChannel(AUDIO_CH_BGM,   1) → slot 4
 *   Audio_SetChannel(AUDIO_CH_SFX,   1) → slot 5
 *   Audio_SetChannel(AUDIO_CH_VOICE, 1) → slot 6
 *
 * Hardware:
 *   SPU: 24 vozes ADPCM, RAM 512 KB, base 0x1F801C00.
 *   Rumble DualShock: pacote SIO montado em g_Controllers[2] (0x801AC980).
 * ──────────────────────────────────────────────────────────────
 */


/* ────────────────────────────────────────────────────────────────
 * Audio_SetMode — 0x8017e040  🟠 Nome incerto
 *
 * Chamada durante StateSlot_Allocate para o slot 0 (FMV/Vídeo).
 * A incerteza do nome existe porque o slot 0 concentra tanto a
 * decodificação MDEC de vídeo quanto o canal XA-ADPCM de áudio.
 *
 * Hipóteses (mutuamente exclusivas até confirmar no Ghidra):
 *   A) Configura o modo MDEC para streaming de FMV (Video_SetMode).
 *   B) Inicializa o canal XA-ADPCM do CD-ROM (Audio_SetMode).
 *   C) Setup conjunto: inicializa os dois sistemas para FMV.
 *
 * TODO:
 *   - Identificar se o primeiro call interno é libmdec (DecDCTReset)
 *     ou libcd (CdInit / CdlSetMode com FLAG_XA).
 *   - Verificar XREFs — se outros callers existirem fora do slot 0,
 *     é provável que seja uma função de áudio pura.
 *
 * PORT NOTE (PC):
 *   Se Video_SetMode: inicializar decoder de FMV
 *       (ex: avcodec_find_decoder para o codec MDEC/MPEG-like do .STR).
 *   Se Audio_SetMode puro: criar o stream OpenAL para XA-ADPCM e
 *       iniciar o thread de decodificação de setor.
 * ─────────────────────────────────────────────────────────────── */
void Audio_SetMode(void)
{
    /* TODO: reconstrução pendente. */
}


/* ────────────────────────────────────────────────────────────────
 * Audio_SetChannel — 0x8017e050  🟡 Alta confiança
 *
 * Registra e ativa um dos 4 canais de áudio lógicos do jogo.
 * O padrão de chamada confirma os 4 canais: BGM(0), SFX(1),
 * Voice(2), FMV(3) — cada um corresponde a um slot da state machine.
 *
 * Padrão de chamada observado em StateSlot_Allocate:
 *   Audio_SetChannel(AUDIO_CH_FMV,   1)  → slot 0
 *   Audio_SetChannel(AUDIO_CH_BGM,   1)  → slot 4
 *   Audio_SetChannel(AUDIO_CH_SFX,   1)  → slot 5
 *   Audio_SetChannel(AUDIO_CH_VOICE, 1)  → slot 6
 *
 * Hipótese: param=1 ativa o canal (aloca vozes SPU / habilita o
 * mixer track). param=0 provavelmente desativa/libera o canal.
 * O canal indexa provavelmente uma tabela interna de configuração
 * com: base na SPU RAM, número de vozes reservadas, volume inicial.
 *
 * TODO:
 *   - Confirmar se mantém um array global de estado por canal.
 *   - Verificar se param pode ser um volume inicial em vez de
 *     simples on/off.
 *   - Mapear a tabela de configuração que provavelmente indexa.
 *
 * PORT NOTE (PC):
 *   Equivale a criar um mixer track por canal lógico:
 *       channel_sources[channel] = al_create_source();  // OpenAL
 *   ou registrar um canal em SDL_mixer / miniaudio.
 * ─────────────────────────────────────────────────────────────── */
void Audio_SetChannel(int channel, int param)
{
    /* TODO: reconstrução pendente. */
    (void)channel;
    (void)param;
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
 * Formato do pacote SIO de rumble (hipótese PsyQ):
 *   byte[0] = intensidade motor pequeno (0x00 / 0xFF)
 *   byte[1] = intensidade motor grande  (0x00–0xFF)
 *
 * TODO:
 *   - Confirmar layout exato do pacote no scratchpad.
 *   - Verificar se 'mode' seleciona entre rumble one-shot vs.
 *     contínuo, ou alterna entre os dois motores.
 *   - Mapear relação com SetRumbleMode (0x80182b5c).
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
 * Relação com SetRumble: pode ser a versão "parametrizada", onde
 * SetRumble é um wrapper de conveniência para casos simples.
 *
 * TODO:
 *   - Mapear a struct apontada por ptr. Campos esperados:
 *       uint8_t  motor_small;   // 0x00 / 0xFF
 *       uint8_t  motor_large;   // 0x00–0xFF
 *       uint16_t duration;      // em frames (hipótese)
 *   - Verificar se escreve diretamente em g_Controllers[2]
 *     ou usa uma tabela de agendamento de eventos de rumble.
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
 * Usado para: verificar se um efeito de rumble ainda está ativo
 * antes de disparar outro, ou para sincronizar animações com
 * o feedback háptico (ex: tiro, dano).
 *
 * Retorno: 0 = motor parado, != 0 = motor ativo.
 *          Tipo real provavelmente uint8_t promovido a int.
 *
 * TODO:
 *   - Confirmar se consulta g_Controllers[2] (scratchpad SPU/rumble)
 *     ou g_Controllers[canal físico do controle do jogador].
 *   - Verificar se retorna diretamente o byte do campo ou um bool.
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
