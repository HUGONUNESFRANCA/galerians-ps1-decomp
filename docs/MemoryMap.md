# Galerians (PS1) — Memory Map & Reverse Engineering Documentation

> **SDK:** PsyQ v1.140 — Sony SCEE, compilado em 12 Janeiro 1998  
> **CPU:** MIPS R3000A (Little Endian, 32-bit)  
> **RAM PS1:** 0x80000000 – 0x801FFFFF (2MB)  
> **Última atualização:** Sessão de mapeamento do Sistema de Câmera

---

## 📐 Arquitetura Geral

```
MAPA DE MEMÓRIA GLOBAL
──────────────────────────────────────────────────────────────
0x80000000  BIOS / vetores de exceção
0x80010000  Código do jogo (SLUS_010.99)
0x8011a494  OT_TERMINATOR = 0xFFFFFFFC
0x8011b958  String PsyQ "$Id: sys.c,v_1.140 1998/01/12"
0x80193358  Ordering Table buffer 0 (renderer)
0x80193888  Ordering Table buffer 1 (renderer)
0x80194b48  g_StateChannelFlags
0x80194b4c  g_StateChannelTable
0x80195bd8  g_AudioStateMask (ptr)
0x801AC8D0  g_ControllerFlags[16]
0x801AC8D8  EngineState — INÍCIO REAL DA STRUCT
0x801AC900  EngineState.channel_table[0]  ← g_EngineBase aponta aqui (+0x28)
0x801AC980  g_Controllers[2] / DAT_801ac980 (usado no rumble)
0x801ACB54  g_CoroutineSlotMask
0x801ACB58  g_CurrentCoroutineCtx
0x801ACB5C  g_StateSlotsActive
0x801ACB5E  g_ActiveStateIndex
0x801ACB60  g_SchedulerStack
0x801ACB64  g_StateSlotMask
0x801C2F9C  g_RionStats (GlobalCombatState)
0x801CB3C0  g_DeathCount
0x801CB3C4  g_ContinuesLeft
0x801D2158  g_CoroutineContextTable
0x801D2198  Coroutine Pool Base (COROUTINE_POOL_BASE)
0x801D3164  PTR_PTR → 0x801AC900
0x801E21D8  g_StateTable_RetAddrs    / g_StateTable_FuncPtrs
0x801E2198  g_StateTable_VsyncData
0x801E2218  g_StateTable_EntryPtrs
0x801E2258  g_LastSlot_A0
0x801E225C  g_LastSlot_A1
0x801AE158  EngineState.rion_hp_mirror  (offset +0x1880 de 0x801AC8D8)
0x801AE170  EngineState.death_field_1
0x801AE174  EngineState.death_field_2
0x801AE178  EngineState.death_field_3
0x80193250  g_ContinuesCount
0x801eb574  g_EngineStatePtr
0x8019b4c8  g_RendererVtable
0x8019b4d2  g_RendererDebugLevel
0x801BFD10  g_ActiveCameraMatrix (matriz viva, muda no movimento)
0x801C1768  PTR_FUN_801c1768     (ponteiro de função global de câmera)
0x801C1778  g_CameraSlots        (4 × stride 0xC60)
0x801C2FF4  g_CameraFuncPtr      (callback da câmera ativa)
0x801C3200  g_CameraTable        (tabela pré-definida por sala)
0x801eb544  g_CameraHistoryPtr   (ptr para ring buffer de histórico; +2 = frame_index int16)
0x801eb560  g_CameraBuffer       (uint32[8] — fonte do upload GTE)
0x1F800168  GTE scratchpad       (destino de Camera_LoadToGTE)
──────────────────────────────────────────────────────────────
```

---

## 🏗️ Structs Mapeadas

### `GlobalCombatState` — Stats de Combate do Rion
**Base:** `0x801C2F9C`  
**Tamanho:** 14 bytes (0x0E)

```c
typedef struct {
    int16_t  hp;             // +0x00 (0x801C2F9C) — HP atual
    int16_t  ap;             // +0x02 (0x801C2F9E) — Addiction Points
    int16_t  hp_max;         // +0x04 (0x801C2FA0) — HP máximo
    int16_t  ap_max;         // +0x06 (0x801C2FA2) — AP máximo
    int16_t  action_state;   // +0x08 (0x801C2FA4) — CUIDADO: valor errado afunda Rion
    int16_t  attack_charge;  // +0x0A (0x801C2FA6) — tiro infinito quando alterado
    int16_t  unknown_timer;  // +0x0C (0x801C2FA8) — hipótese: i-frames
} GlobalCombatState;

#define AP_CRITICAL_THRESHOLD 0x11  // 17 — nível de Shorting/Addiction
#define RION_STATS  ((GlobalCombatState *)0x801C2F9C)
```

---

### `EngineState` — Struct Mestre da Engine
**Início real:** `0x801AC8D8`  
**g_EngineBase** (usado no código) aponta para `+0x28` = `0x801AC900`

```c
typedef struct {
    uint8_t        _unknown_0x00[0x28];   // 0x801AC8D8 – 0x801AC8FF: desconhecido

    // +0x28 — array de canais de input (g_EngineBase aponta aqui)
    ControllerChannel channel_table[16];  // 0x801AC900 – 0x801ACDFF (16 × 0x40)

    // +0x428 — scheduler
    uint16_t       state_slots_active;    // 0x801ACB5C
    uint16_t       active_state_index;    // 0x801ACB5E
    uint8_t        *scheduler_stack;      // 0x801ACB60
    uint16_t       state_slot_mask;       // 0x801ACB64

    uint8_t        _unknown_gap[];        // 0x801ACB66 – 0x801AE157: desconhecido

    // +0x1880 — combate
    int32_t        rion_hp_mirror;        // 0x801AE158 — HP negativo = morte
    uint8_t        _unknown_0x1884[0x14];
    uint32_t       death_field_1;         // 0x801AE170 — zerado por Engine_ResetDeathState
    uint32_t       death_field_2;         // 0x801AE174
    uint32_t       death_field_3;         // 0x801AE178
} EngineState;

#define ENGINE_STATE_BASE  ((EngineState *)0x801AC8D8)
#define ENGINE_BASE        ((ControllerChannel *)0x801AC900)
#define g_ControllerFlags  ((uint8_t *)0x801AC8D0)
```

> ⚠️ **QUESTÃO ABERTA:** Relação entre `EngineState.rion_hp_mirror` e `GlobalCombatState->hp`.  
> Verificar com watchpoint duplo no DuckStation.

---

### `ControllerChannel` — Porta de Controle PS1
**Base:** `0x801AC900`  
**Stride:** `0x40` bytes por canal (16 canais)

```c
typedef struct {
    // Botões Digitais
    uint16_t  buttons_current;     // +0x00 estado atual (1=pressionado)
    uint16_t  _pad_02;
    uint16_t  buttons_pressed;     // +0x04 recém pressionado (rising edge)
    uint16_t  _pad_06;
    uint16_t  buttons_released;    // +0x08 recém solto (falling edge)
    uint16_t  _pad_0A;
    uint16_t  buttons_prev;        // +0x0C frame anterior
    uint16_t  button_mask;         // +0x0E máscara de botões ativos
    // Turbo / Repeat
    uint8_t   _pad_10;
    uint8_t   turbo_limit;         // +0x11 frames máx até auto-repeat
    uint8_t   repeat_counter;      // +0x12 contador de frames pressionado
    uint8_t   repeat_timer;        // +0x13 timer interno do turbo
    // Analógicos (DualShock tipo 7 apenas)
    uint8_t   analog_left_x;       // +0x14
    uint8_t   _pad_15[3];
    uint8_t   analog_left_y;       // +0x18
    uint8_t   _pad_19[3];
    uint8_t   analog_right_x;      // +0x1C threshold: <0x40=esq, >0xC0=dir
    uint8_t   _pad_1D[3];
    uint8_t   analog_right_y;      // +0x20 threshold: <0x40=cima, >0xC0=baixo
    uint8_t   _pad_21[3];
    // Rumble
    uint32_t  rumble_data;         // +0x24
    uint8_t   _pad_28[4];
    // Botões Analógicos
    uint16_t  analog_buttons;      // +0x2C
    uint16_t  _pad_2E;
    uint16_t  analog_pressed;      // +0x30
    uint16_t  _pad_32;
    uint16_t  analog_released;     // +0x34
    uint16_t  _pad_36;
    // Flags DualShock
    uint8_t   vibration_active;    // +0x38
    uint8_t   analog_repeat;       // +0x39
    uint8_t   analog_timer;        // +0x3A
    uint8_t   combine_flag;        // +0x3B OR digital+analógico se ativo
    uint16_t  analog_buttons_prev; // +0x3C
    uint16_t  _pad_3E;
} ControllerChannel;  // sizeof = 0x40

// Tipos de canal (g_ControllerFlags[]):
//   0x00 = porta vazia
//   0x04 = controle digital (tipo 4)
//   0x10 = DualShock analógico com rumble (tipo 7)

#define g_Controllers     ((ControllerChannel *)0x801AC900)
#define BTN_SELECT        0x0001
#define BTN_L3            0x0002
#define BTN_R3            0x0004
#define BTN_START         0x0008
#define BTN_UP            0x0010
#define BTN_RIGHT         0x0020
#define BTN_DOWN          0x0040
#define BTN_LEFT          0x0080
#define BTN_L2            0x0100
#define BTN_R2            0x0200
#define BTN_L1            0x0400
#define BTN_R1            0x0800
#define BTN_TRIANGLE      0x1000
#define BTN_CIRCLE        0x2000
#define BTN_CROSS         0x4000
#define BTN_SQUARE        0x8000
```

---

### `CoroutineContext` — Contexto de Corrotina
**Base:** `0x801D2198`  
**Stride:** `0x240` bytes por corrotina  

```c
// Capacidade: 16 slots × 7 corrotinas = 112 corrotinas simultâneas
// Slot stride: 0x1000 bytes (4096) por slot
// Corrotina stride: 0x240 bytes (576) por contexto
// Navegação yield: 0x120 bytes (288) — stride interno

// Offsets dentro do bloco (negativos a partir do topo):
// -0x238  RA salvo (entry function)
// -0x23C  SP salvo (0 = slot livre)
// -0x034  fp
// -0x030 a -0x014  s7 a s0
// -0x010 a -0x004  a3 a a0 (args de entrada da corrotina)
// -0x11E  ponteiro para próxima corrotina (NULL = fim da cadeia)
// -0x11C  função da próxima corrotina
```

---

### `CameraEntry` — Entrada de Câmera (tabela + ativa)
**Ativa:** `0x801BFD10` (`g_ActiveCameraMatrix`)  
**Tabela:** `0x801C3200` (`g_CameraTable`)

```c
typedef struct {
    int16_t  rot_matrix[9];  // +0x00 matriz 3×3 (int16, 4096=1.0)
    int16_t  _pad;           // +0x12 alinhamento PsyQ MATRIX
    int32_t  pos_x;          // +0x14 translação X
    int32_t  pos_y;          // +0x18 translação Y
    int32_t  pos_z;          // +0x1C translação Z
    int16_t  active_flag;    // +0x20 -1 = válido / 0 = inativo
} CameraEntry;
```

---

### `CameraSlot` — Slot Residente de Câmera
**Base:** `0x801C1778` (`g_CameraSlots`)  
**Stride:** `0xC60` bytes por slot (4 slots)

```c
// Layout confirmado (apenas dois campos):
//   +0x000  void *func_ptr      — callback/handler do slot
//   +0xC3E  int16_t active_flag — 0 = inativo (resetado por Camera_Manager)
//
// Restante do slot (0x004 – 0xC3D, 0xC40 – 0xC5F) ainda não mapeado.
```

**Constantes:**
```c
#define CAMERA_SLOT_COUNT    4
#define CAMERA_SLOT_STRIDE   0xC60
#define GTE_SCRATCHPAD_ADDR  0x1F800168
```

---

## ⚙️ Funções Mapeadas

### Sistema de Game Loop / Scheduler

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x80185170` | `StateSlot_Scheduler` | ✅ | Loop principal — processa 1 slot por chamada |
| `0x8017e0fc` | `VSync_WaitFrameSync` | ✅ | Aguarda VBlank, param: `0xf2000001` |
| `0x80172bec` | `StateMachine_Dispatch` | ✅ | Dispatcher da state machine |
| `0x801854d4` | `StateSlot_Execute` | ✅ | Executor de slot (indirect jump) |
| `0x8016019c` | `StateSlot_Allocate` | ✅ | Aloca slot na state machine |
| `0x80185510` | `Context_Restore` | ✅ | Restaura registradores a0-a3, s0-s8 da stack |
| `0x801850c4` | `FrameCycle_Start` | ✅ | Entry point de cada ciclo de frame |
| `0x80184f24` | `Coroutine_Yield` | ✅ | Cede controle ao próximo slot |
| `0x80184f4c` | `CoroutineSlot_Init` | ✅ | Inicializa pool de corrotinas do slot |
| `0x80184dc4` | `Coroutine_Spawn` | ✅ | Cria nova corrotina no canal |
| `0x8018549c` | `GetFreeMemory` | ✅ | Retorna bytes livres para pool de corrotinas |

### Sistema de Combate / Morte

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x8018539c` | `Trigger_GameOver` | ✅ | Dispatcher de morte por tabela de função |
| `0x8018536c` | `State_ResetBeforeDeath` | 🟡 | Chamada antes de GameOver |
| `0x8013a704` | `Death_IncrementAndTrigger` | ✅ | Incrementa g_DeathCount e aciona GameOver |
| `0x80128df8` | `Engine_ResetDeathState` | 🟡 | Zera death_field 1/2/3 |
| `0x8017d610` | `AP_CriticalEffect` | 🟡 | Efeito quando AP < 17 (Shorting) |
| `0x80177f90` | `Check_BattleEvent` | 🟡 | Retorna char de evento de batalha |
| `0x8017d2e0` | `Toggle_EntityState` | 🟡 | Ativa/desativa estado de entidade |
| `0x80177fa0` | `AP_OverflowHandler` | 🟠 | Recebe 2 buffers, AP overflow |
| `0x80177f40` | `Update_Combat_A` | 🔴 | Recebe engine ptr |
| `0x80177f70` | `Update_Combat_B` | 🔴 | Recebe engine ptr |
| `0x8016cbe0` | `Engine_FrameCleanup_A` | 🟠 | Final do frame |
| `0x8016d4d0` | `Engine_FrameCleanup_B` | 🟠 | Final do frame |
| `0x8016d490` | `GetCurrentObject` | 🟠 | Retorna ponteiro para objeto atual |
| `0x8014ced4` | `GameOver_Screen` | 🟡 | Lê/escreve g_DeathCount |
| `0x8014da18` | `Stats_Or_Save_Screen` | 🟠 | Lê/escreve g_DeathCount |

### Sistema de Input

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x80185cc8` | `Controller_Update` | ✅ | Gerencia canal de controle (reset/aloca) |
| `0x80185e14` | `Controller_UpdateDigital` | ✅ | Processa controle digital (tipo 4) |
| `0x80185f68` | `Controller_UpdateDualShock` | ✅ | Processa DualShock analógico (tipo 7) |
| `0x8018635c` | `Controller_UpdateRumble` | ✅ | Controla motor de vibração |
| `0x8018281c` | `GetRumbleState` | 🟡 | Estado atual do rumble |
| `0x80182bdc` | `SetRumble` | 🟡 | Define intensidade do rumble |
| `0x80182b5c` | `SetRumbleMode` | 🟡 | Define modo do rumble |

### Sistema de Renderer

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x80178d1c` | `Renderer_Flush` | ✅ | DrawSync via vtable, log em debug |
| `0x8017d150` | `Texture_Load` | ✅ | Carrega textura VRAM (4/8/16bpp) |
| `0x8017d240` | `SetTPage` | 🟡 | Configura página de textura |
| `0x8017afc4` | `ClearImage` | 🟡 | ResetGraph / limpa framebuffer |
| `0x8017albc` | `SetDispMask` | 🟡 | Controle de visibilidade do display |
| `0x8017b114` | `DrawSync` | 🟡 | Aguarda GPU finalizar |
| `0x80165f0c` | `Calc_TileIndex` | 🟠 | Retorna índice de tile via FUN_8017d150 |

### State Machine — Inicialização

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x8016019c` | `StateSlot_Allocate` | ✅ | Aloca slot — revela canais de áudio/vídeo |
| `0x80172bec` | `StateMachine_Dispatch` | ✅ | Dispatcher com DAT_801acb5e como índice |

### Sistema de Câmera

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x80187320` | `Camera_LoadToGTE` | ✅ | Copia 8 × uint32 de `g_CameraBuffer` para scratchpad GTE (`0x1F800168`) |
| `0x8013b584` | `Camera_Manager` | ✅ | Reseta 4 slots (stride `0xC60`) — zera `+0xC3E` e `func_ptr` |
| `0x80187350` | `Camera_RecordFrame` | ✅ | Copia 8 × uint32 de `g_CameraBuffer` → `g_CameraHistoryPtr + 4 + (idx × 0x20)`; incrementa `idx` |

---

## 📊 Variáveis Globais Confirmadas

| Endereço | Nome | Tipo | Confiança |
|---|---|---|---|
| `0x801AC8D8` | `ENGINE_STATE_BASE` | `EngineState*` | ✅ |
| `0x801AC900` | `g_EngineBase` | `ControllerChannel*` | ✅ |
| `0x801AC8D0` | `g_ControllerFlags` | `uint8_t[16]` | ✅ |
| `0x801ACB54` | `g_CoroutineSlotMask` | `uint16_t` | ✅ |
| `0x801ACB58` | `g_CurrentCoroutineCtx` | `uint8_t*` | ✅ |
| `0x801ACB5C` | `g_StateSlotsActive` | `uint16_t` | ✅ |
| `0x801ACB5E` | `g_ActiveStateIndex` | `uint16_t` | ✅ |
| `0x801ACB60` | `g_SchedulerStack` | `uint16_t*` | ✅ |
| `0x801ACB64` | `g_StateSlotMask` | `uint16_t` | ✅ |
| `0x801C2F9C` | `g_RionStats` | `GlobalCombatState` | ✅ |
| `0x801CB3C0` | `g_DeathCount` | `uint32_t` | ✅ |
| `0x801CB3C4` | `g_ContinuesLeft` | `uint16_t` | 🟡 |
| `0x80193250` | `g_ContinuesCount` | `int32_t` | 🟡 |
| `0x801D2158` | `g_CoroutineContextTable` | `uint32_t*` | ✅ |
| `0x801E2198` | `g_StateTable_VsyncData` | `uint32_t[16]` | ✅ |
| `0x801E21D8` | `g_StateTable_FuncPtrs` | `void*[16]` | ✅ |
| `0x801E2218` | `g_StateTable_EntryPtrs` | `void*[16]` | ✅ |
| `0x801E2258` | `g_LastSlot_A0` | `uint32_t` | ✅ |
| `0x801E225C` | `g_LastSlot_A1` | `uint32_t` | ✅ |
| `0x8019b4c8` | `g_RendererVtable` | `void**` | ✅ |
| `0x8019b4d2` | `g_RendererDebugLevel` | `uint16_t` | ✅ |
| `0x80193358` | `g_OrderingTable[0]` | `uint32_t[]` | ✅ |
| `0x80193888` | `g_OrderingTable[1]` | `uint32_t[]` | ✅ |
| `0x801BFD10` | `g_ActiveCameraMatrix` | `CameraEntry*` | ✅ |
| `0x801C1778` | `g_CameraSlots` | `CameraSlot[4]` (stride `0xC60`) | ✅ |
| `0x801C2FF4` | `g_CameraFuncPtr` | `void*` | ✅ |
| `0x801C3200` | `g_CameraTable` | `CameraEntry*` | ✅ |
| `0x801eb544` | `g_CameraHistoryPtr` | `void*` | ✅ |
| `0x801eb560` | `g_CameraBuffer` | `uint32_t[8]` | ✅ |

---

## 🔄 State Machine — Slots Conhecidos

| Slot | Sistema | Função de Init |
|---|---|---|
| 0 | Vídeo / FMV | `Video_SetMode` + `Audio_SetChannel(3,1)` |
| 4 | Música BGM | `Audio_SetChannel(0,1)` |
| 5 | SFX | `Audio_SetChannel(1,1)` |
| 6 | Voz / Diálogo | `Audio_SetChannel(2,1)` |
| ? | Gameplay | Contém `Handle_Combat_State` — slot a identificar |

**Como identificar o slot de gameplay:**
```
DuckStation → Execute breakpoint em Handle_Combat_State
Quando disparar → olhar valor de g_ActiveStateIndex (0x801ACB5E)
```

---

## 🎮 Coroutine Scheduler — Arquitetura Completa

```
FrameCycle_Start (0x801850c4)
  → salva SP em g_SchedulerStack
  → g_ActiveStateIndex = 0
  → busca primeiro slot ativo
  → Context_Restore → executa slot

[Slot executa sua lógica]
  → chama Coroutine_Yield quando quer ceder

Coroutine_Yield (0x80184f24)
  → salva RA + SP do slot atual
  → decrementa countdown timer
  → stride 0x120 → navega para próximo contexto
  → Context_Restore → executa próxima corrotina

StateSlot_Scheduler (0x80185170)
  → VSync_WaitFrameSync(0xf2000001)
  → salva resultado em g_StateTable_VsyncData[slot]
  → g_ActiveStateIndex++
  → se índice == 16 → Context_Restore → return
  → senão → busca próximo slot ativo → dispatch
```

---

## 🖥️ Renderer — Pipeline PsyQ

```
SDK: PsyQ v1.140 — Sony SCEE — 12 Janeiro 1998

Pipeline por frame:
  1. ClearOTag()         inicializa Ordering Table
  2. [primitivas adicionadas à OT via AddPrim]
  3. DrawOTag()          envia OT ao GPU (linked list de primitivas)
  4. DrawSync()          aguarda GPU finalizar
  5. VSync()             aguarda VBlank
  6. PutDispEnv()        alterna display buffer

Ordering Table:
  Buffer 0: 0x80193358 – início
  Buffer 1: 0x80193888 – início
  Terminador: 0xFFFFFFFC

Vtable do Renderer (base 0x8019b4c8):
  -0x0C  ClearImage / ResetGraph  (0x8017afc4)
  -0x08  SetDispMask              (0x8017albc)
  -0x04  DrawSync direto          (0x8017b114)
  +0x3C  DrawSync via callback    ← usado por Renderer_Flush

Formatos de Textura:
  0 = 4bpp  (paleta 16 cores)
  1 = 8bpp  (paleta 256 cores)
  2 = 16bpp (cor direta)
```

---

## 🚧 Questões Abertas

- [ ] **Relação HP:** `EngineState.rion_hp_mirror` vs `GlobalCombatState->hp` — são o mesmo dado ou cópias sincronizadas?
- [ ] **Slot de Gameplay:** qual índice (0-15) contém `Handle_Combat_State`?
- [ ] **Tamanho total da EngineState:** gap entre `0x801ACB66` e `0x801AE158` ainda não mapeado
- [x] **Sistema de câmera:** pipeline completo mapeado — `Camera_LoadToGTE`, `Camera_Manager` e `Camera_RecordFrame` confirmados
- [x] **Camera_RecordFrame (0x80187350):** ring buffer de histórico de frames confirmado; `g_CameraHistoryPtr` (0x801eb544) e stride 0x20
- [ ] **CameraSlot interno (0x004 – 0xC3D):** payload não mapeado entre `func_ptr` e `active_flag`
- [ ] **Relação `g_CameraTable` ↔ `g_CameraSlots`:** 0x801C3200 cai dentro do range dos slots (0x801C1778 + 0x3180) — verificar sobreposição
- [ ] **DrawOTag / ClearOTag:** endereços não confirmados
- [ ] **Engine_EnqueueLoadImage:** endereço a confirmar
- [ ] **FUN_80184f24 / FUN_801850c4 callers adicionais:** `FUN_80184f24` e `FUN_801850c4` têm outros XREFs a analisar

---

## 🗓️ Roadmap do Port Windows

### Fase 1 — Mapeamento (Em Andamento)
- [x] Game Loop / Scheduler
- [x] Sistema de Combate
- [x] Sistema de Input
- [x] Sistema de Corrotinas
- [x] Renderer (parcial)
- [x] Sistema de Câmera (completo — `Camera_LoadToGTE`, `Camera_Manager`, `Camera_RecordFrame`)
- [ ] Sistema de Áudio
- [ ] Sistema de FMV/MDEC
- [ ] Overlays de mapa/área

### Fase 2 — Port Base
- [ ] Substituir VSync por timer Windows (QueryPerformanceCounter)
- [ ] Substituir DrawOTag por OpenGL/Vulkan
- [ ] Substituir LoadImage por glTexImage2D
- [ ] Substituir input PS1 por SDL2/XInput
- [ ] Substituir áudio SPU por OpenAL/SDL_Mixer

### Fase 3 — Melhorias
- [ ] FPS desbloqueado (separar update/render com acumulador)
- [ ] Resolução aumentada (framebuffer escalado)
- [ ] Texturas HD (upscaling xBRZ ou texture packs)
- [ ] Widescreen (ajuste de FOV horizontal)
- [ ] Freecam (interceptar writes na struct de câmera)

---

## 📝 Notas de Ferramentas

### DuckStation — Comandos Úteis de Debug
```
Execute breakpoint:  Debug → Breakpoints → Add → Type: Execute
Write watchpoint:    Debug → Breakpoints → Add → Type: Write
Memory viewer:       Debug → Memory → endereço em hex sem 0x
Dump de RAM:         Debug → Memory → Save to File
                     Start: 0x80000000 / Size: 0x200000
```

### Ghidra — Atalhos Importantes
```
G           → Go to address
D           → Forçar disassembly no cursor
F           → Criar função no cursor
Ctrl+Shift+F → Find references (XREFs)
Ctrl+Shift+E → Search Memory (byte string)
; (ponto-vírgula) → Adicionar comentário
L           → Rename (label/função/variável)
```

### Problema: Instruções GTE no Ghidra
O Ghidra padrão não reconhece instruções COP2 (GTE) do PS1.
**Solução:** Usar o dump de RAM do DuckStation em vez do executável do disco.
O dump contém o código já expandido e sem instruções GTE problemáticas.