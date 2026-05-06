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
0x000000B0  BIOS Table B           (usado por Audio_SetChannel via t1)
0x000000C0  BIOS Table C           (usado por Video_SetMode via t1)
0x1F800000  Scratchpad base        (1 KB; 0x2A0 usado — ver scratchpad.h)
0x1F8001A8  Scratchpad: SPU buffer (DAT_801eb55c)
0x1F8001C8  Scratchpad: SPU DMA    (DAT_801eb558)
0x1F8001E8  Scratchpad: SPU buffer (DAT_801eb554)
0x1F800208  Scratchpad: SPU DMA    (DAT_801e2588)
0x000000A0  BIOS Table A           (CD/hardware; índice 0x27 = CdGetSector — hardware CD access)
0x80010000  Código do jogo (SLUS_010.99)
0x8011A14C  s_ModelCDB             ("MODEL.CDB")
0x8011A158  s_ModuleBin            ("MODULE.BIN")
0x8011c0dc  g_CDConfig             (CD driver config block)
0x8011c0e4  g_CDDefaultPath        (default CD path string)
0x8011a494  OT_TERMINATOR = 0xFFFFFFFC
0x8011b958  String PsyQ "$Id: sys.c,v_1.140 1998/01/12"
0x80193E30  g_ScreenWidth          (halved para Video_SetResolution)
0x80193E34  g_ScreenHeight
0x801E2380  g_SPUChannelBankA      (SPU voices 0-15)
0x801E2470  g_SPUChannelBankB      (SPU voices 16-23)
0x801932b4  g_OTBuffer_2   Ordering Table buffer 2
0x80193330  g_OTBuffer_0   Ordering Table buffer 0 (main gameplay OT)
0x801933ac  g_OTBuffer_1   Ordering Table buffer 1 (secondary OT)
0x80193428  g_OTBuffer_3   Ordering Table buffer 3
0x801934a4  g_OTBuffer_4   Ordering Table buffer 4
0x80193520  g_OTBuffer_5   Ordering Table buffer 5 — Terminator: 0xFFFFFFFC
0x80190e0c  g_FileTableA              (10 × {dest,name} - Disc 1)
0x80190e5c  g_FileTableB              (7 × {dest,name} - Disc 2)
0x80194078  SPU_DMA_Table_A           (transfers para SPU RAM)
0x80194b48  g_StateChannelFlags
0x80194b4c  g_StateChannelTable
0x80195bd8  g_AudioStateMask (ptr)
0x80195C10  g_FrameCounter            (uint32_t, ++ a cada VBlank)
0x80195FD0  SPU_DMA_Table_B
0x8011A164  Sound tables base         (lido por Sound_Manager @ 0x8011d198)
0x8011A174  g_SFXTable                (164-entry SFX index)
0x8011B07C  Sound tables end
0x801AC830  g_AudioVtable             (void*[7+], dispatch de áudio)
0x801ACEA8  g_SoundCDB_Base           (SOUND.CDB RAM base)
0x801ACF60  g_MenuCDB_Base            (MENU.CDB RAM base)
0x801ACFD8  g_ItemTimCDB_Base         (ITEMTIM.CDB RAM base)
0x801AD050  g_ModelCDB_Base           (MODEL.CDB RAM base)
0x801AD0C8  g_DisplayCDB_Base         (DISPLAY.CDB RAM base)
0x801AD140  MODULE.BIN load address   (overlay code base)
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
0x8019b5d8  g_DMAStatusPtr      (void* — pointer to DMA status)
0x8019b5e4  g_GPUStatusPtr      (void* — pointer to GPU status)
0x8019b5f8  g_RenderQueueWrite  (uint — render queue write ptr)
0x8019b5fc  g_RenderQueueRead   (uint — render queue read ptr)
0x801d04a0  g_DrawOTagBase      (void* — OT base; Ghidra: PTR_PsyQ_DrawOTag_801d04a0)
0x801bfae8  g_SEQ_DataPtr        (PTR → 0x80037ae0 — pointer to active SEQ in RAM; XREF: FUN_80130764 @ 0x801307ac)
0x801bfaf0  g_SEQ_Size           (SEQ size in bytes, e.g. 0x00000DF8 = 3576; XREF: FUN_8012f3bc, FUN_8012f4dc)
0x801bfaf4  g_SEQ_LoopMarker     (0xFFFFFFFF = loop/end sentinel; XREF: FUN_8012f3bc, FUN_8012f4dc)
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

### Sistema de Renderer (COMPLETE ✅)

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x8017b06c` | `Frame_Flip` | ✅ | Writes GPU Display Mode to 0x1F8010A8 (PutDispEnv); calls PsyQ DrawOTag via PTR_PsyQ_DrawOTag_801d04a0; clears GPU DMA control register (0x1F801814 = 0) |
| `0x8017b114` | `DrawSync(mode)` | ✅ | param_1=0: blocking wait (VSync + GPU drain); param_1≠0: non-blocking status check. Returns: 0=success, 0xFFFFFFFF=error, 1=pending |
| `0x8017b8a8` | `PsyQ_DrawOTag` / `PutDispEnv` | ✅ | Called with different args for each operation; PTR_PsyQ_DrawOTag_801d04a0 = OT base pointer (auto-labeled by Ghidra) |
| `0x80183a00` | `RenderQueue_Dispatch` | ✅ | Sends GPU command queue |
| `0x8017b284` | `GPU_CheckTimeout` | ✅ | Monitors GPU hang |
| `0x8014d2cc` | `BattleTransition_Render(ot_case, param_2)` | ✅ | Renders combat transition/result screen; iterates 4 camera slots, 6 OT buffer cases; ends with Trigger_GameOver + State_ResetBeforeDeath |
| `0x80178d1c` | `Renderer_Flush` | ✅ | DrawSync via vtable, log em debug |
| `0x8017d150` | `Texture_Load` | ✅ | Carrega textura VRAM (4/8/16bpp) |
| `0x8017d240` | `SetTPage` | 🟡 | Configura página de textura |
| `0x8017afc4` | `ClearImage` | 🟡 | ResetGraph / limpa framebuffer |
| `0x8017a1bc` | `SetDispMask` | 🟡 | Controle de visibilidade do display |
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

## 🧠 Engine Boot — Engine_Init

`Engine_Init` (`0x80187DF4`) é o ponto de entrada da engine inteira.
Inicialmente rotulado como `SPU_CoreDriver` por causa dos 22 acessos a
registradores SPU, mas o mapeamento completo mostrou que:

1. **Zera o scratchpad** (`0x1F800000`, `0x2A0` bytes em uso).
2. **Mapeia os subsistemas** preenchendo ponteiros (engine state,
   câmera, SPU) para blocos dentro do scratchpad.
3. **Configura o renderer**:
   - `GTE_SetScreenCenter(g_ScreenWidth/2, g_ScreenHeight/2)`
   - `Display_Enable(1)`
   - `SetDrawEnv(...)`
4. **Obtém os bancos de voz do SPU** via `SPU_GetChannelBank`:
   - Bank A (`0x801E2380`) → voices 0-15
   - Bank B (`0x801E2470`) → voices 16-23
5. **Inicializa voices/buffers SPU** — `SPU_VoiceInit` e `SPU_BufferClear`
   são chamados com ponteiros para dentro do scratchpad (init de voice
   defaults e zeragem de buffer/reverb, respectivamente).
6. **Dispara o primeiro VSync** via `Frame_First` (`0x8018669C`).

### Funções do Boot

| Endereço | Nome | Confiança | Descrição |
|---|---|---|---|
| `0x80187DF4` | `Engine_Init`          | ✅ | Zera scratchpad, mapeia globals, init renderer, primeiro VSync |
| `0x8018c008` | `GTE_SetScreenCenter`  | ✅ | Escreve GTE control regs OFX (`0xC000`) e OFY (`0xC800`) com os argumentos deslocados `<< 16` (formato fixo Q15.16 do GTE; equivalente a `<< 0x10`). Chamado com `(g_ScreenWidth/2, g_ScreenHeight/2)`. **Chave para widescreen.** |
| `0x80178c84` | `Display_Enable`       | ✅ | `Display_Enable(1)` habilita saída de vídeo |
| `0x80178ea0` | `SetDrawEnv`           | ✅ | Configura drawing environment (OT, clip, bg color) |
| `0x8018669C` | `Frame_First`          | ✅ | Primeiro frame após init |
| `0x80186D38` | `SPU_GetChannelBank`   | ✅ | Retorna ptr para bank A (`0x801E2380`) ou B (`0x801E2470`) |
| `0x80187420` | `SPU_VoiceInit`        | ✅ | Zera 32 bytes do voice block; seta pitch e volume default = `0x1000` (44100 Hz). Chamada 2× em Engine_Init |
| `0x80187450` | `SPU_BufferClear`      | ✅ | `memset(buffer, 0, 32)`. Usado para SPU output buffer / reverb |

---

## 📦 Scratchpad Layout

**Endereço:** `0x1F800000`  
**Tamanho:** 1 KB (0x400 bytes) — hardware  
**Usado:** 0x2A0 bytes pelo jogo

A scratchpad é 1 KB alocada no data cache do R3000A (~1 ciclo vs. ~4
ciclos para main RAM). Toda a região é zerada por `Engine_Init` e
depois preenchida com shadows dos subsistemas. Ver
`src/include/scratchpad.h` para a struct C completa.

```
SCRATCHPAD (0x1F800000, 1KB total, 0x2A0 bytes used):
 0x1F800000  Camera history buffer (stride 0x20 per frame, ~10 frames)
 0x1F800144  DAT_801e2590 (unknown, 4 bytes)
 0x1F800148  g_EngineStatePtr target (shadow de DAT_801eb574, 0x20)
 0x1F800168  g_CameraBuffer / GTE camera active (DAT_801eb560, 0x20)
 0x1F800188  DAT_801eb56c (unknown, 0x20)
 0x1F8001A8  DAT_801eb55c → passado para SPU_BufferClear (0x20)
 0x1F8001C8  DAT_801eb558 → passado para SPU_VoiceInit  (0x20)
 0x1F8001E8  DAT_801eb554 → passado para SPU_BufferClear (0x20)
 0x1F800208  DAT_801e2588 → passado para SPU_VoiceInit  (0x20)
 0x1F800228  10 globals mapeados (propósito desconhecido) — até 0x1F80029C
 0x1F8002A0  unused (resto do KB)
```

### Ponteiros que apontam para dentro do scratchpad

| Global (main RAM) | Scratchpad target | Via |
|---|---|---|
| `0x801eb560`  `g_CameraBuffer`    | `0x1F800168` | Camera_LoadToGTE |
| `0x801eb574`  `g_EngineStatePtr`  | `0x1F800148` | Engine_Init      |
| `0x801eb56c`  `DAT_801eb56c`      | `0x1F800188` | Engine_Init      |
| `0x801eb55c`  `DAT_801eb55c`      | `0x1F8001A8` | SPU_BufferClear  |
| `0x801eb558`  `DAT_801eb558`      | `0x1F8001C8` | SPU_VoiceInit    |
| `0x801eb554`  `DAT_801eb554`      | `0x1F8001E8` | SPU_BufferClear  |
| `0x801e2588`  `DAT_801e2588`      | `0x1F800208` | SPU_VoiceInit    |
| `0x801e2590`  `DAT_801e2590`      | `0x1F800144` | Engine_Init      |

**Port note:** A scratchpad não existe em hardware PC. Alocar como
struct global estática (ou `malloc(ScratchpadLayout)`) e fazer os
ponteiros `DAT_801eb*` / `DAT_801e2*` apontarem para campos da struct.

---

## Audio System

### Architecture

O áudio é despachado por uma vtable de 7+ entradas em `g_AudioVtable`
(`0x801AC830`). As funções de entrada `Audio_SetChannel` (`0x8017e050`)
e `Video_SetMode` (`0x8017e040`) são wrappers para as **tabelas B/C do
BIOS PS1** (`0x000000B0` / `0x000000C0`): o caller carrega o registrador
**t1** com o índice da função BIOS desejada e chama o wrapper.

`PsyQ_VSync` é o caller comum de `FUN_8017e040` e `FUN_8017e050` — é
desse contexto que vem a confirmação da semântica dos 4 canais lógicos.

### g_AudioVtable @ 0x801AC830 — entradas confirmadas

| Index | Address      |
|-------|--------------|
| [0]   | `0x80186938` |
| [1]   | `0x80186A84` |
| [2]   | `0x80186D00` |
| [3]   | `0x80186D38` |
| [4]   | `0x80186D90` |
| [5]   | `0x801871E4` |
| [6+]  | (a dumpar)   |

### Known State Machine Slots

| Slot | System    | Init Function |
|------|-----------|---------------|
| 0    | FMV/Video | `Video_SetMode(0)` (0x8017e040) + `Audio_SetChannel(3,1)` |
| 4    | BGM Music | `Audio_SetChannel(0,1)` at `0x8017e050` |
| 5    | SFX       | `Audio_SetChannel(1,1)` at `0x8017e050` |
| 6    | Voice     | `Audio_SetChannel(2,1)` at `0x8017e050` |

### Confirmed Functions

| Address      | Name                | Confiança | Notes |
|--------------|---------------------|:---------:|-------|
| `0x8017e040` | `Video_SetMode`     | ✅ | BIOS table C wrapper (mode arg) |
| `0x8017e050` | `Audio_SetChannel`  | ✅ | BIOS table B wrapper (channel, enable); channels 0=BGM, 1=SFX, 2=Voice, 3=FMV |
| `0x80183A00` | `SPU_SetVoiceField` | ✅ | escreve `voice+0x28 = data`, `voice+0x34 = flag` (shadow state em RAM) |
| `0x80187420` | `SPU_VoiceInit`     | ✅ | Zera 32 bytes do voice block; seta pitch+volume = `0x1000` (default 44100 Hz) |
| `0x80187450` | `SPU_BufferClear`   | ✅ | `memset(buffer, 0, 32)` — limpa SPU output/reverb buffer |
| `0x8011d198` | `Sound_Manager`     | ✅ | **Carregador central de assets de áudio.** Lê todas as sound tables a partir de `0x8011A164`. Offset interno de leitura em `0x8011d208`. |
| `0x80130764` | `SEQ_Player`        | ✅ | Confirmed SEQ player. Reads `g_SEQ_DataPtr` (`0x801bfae8`) at offset `0x801307ac` to get active SEQ at `0x80037ae0`. |
| `0x8012f3bc` | `SEQ_Reader_A`      | 🟡 | Reads `g_SEQ_Size` (`0x801bfaf0`) and `g_SEQ_LoopMarker` (`0x801bfaf4`). |
| `0x8012f4dc` | `SEQ_Reader_B`      | 🟡 | Reads `g_SEQ_Size` (`0x801bfaf0`) and `g_SEQ_LoopMarker` (`0x801bfaf4`). |
| `0x80184898` | `SPU_SetVolume`     | 🟡 | 3 SPU refs |
| `0x801834B4` | `SPU_SetADSR`       | 🟡 | 2 SPU refs |
| `0x80177328` | `SPU_KeyOnOff`      | 🟡 | 4 SPU refs (provavelmente `SPU_KEY_ON/OFF`) |
| `0x8017D558` | `Debug_Print`       | ✅ | Confirmado pela string `"VSync: timeout"` |
| `0x80182bdc` | `SetRumble`         | 🟡 | DualShock motor write |
| `0x80182b5c` | `SetRumbleMode`     | 🟡 | DualShock config |
| `0x8018281c` | `GetRumbleState`    | 🟡 | DualShock state read |

### PS1 SPU Hardware

- SPU base: `0x1F801C00` (registradores de voz)
- 24 voice channels, stride `0x10` (`pitch`, `ADSR`, `volume`, `start addr`)
- Audio RAM: 512 KB dedicada em `0x1F800000` (Sound RAM)
- ADPCM compression: 4-bit, 28 samples per block
- DMA tables (transfers para SPU RAM): `0x80194078` e `0x80195FD0`

### SOUND.CDB — Confirmed Format

| Property | Value |
|----------|-------|
| File size | 15,919,104 bytes (15.18 MB, 7773 sectors) |
| compression_flag | `0x00000000` — RAW (no LZSS) |
| RAM base | `0x801ACEA8` (`g_SoundCDB_Base`) |
| Sub-files | 95 SEQ files (pQES magic `70 51 45 53`) |
| SEQ Player | `SEQ_Player` @ `0x80130764` ✅ |

SEQ (pQES) = PS1 sequencer format (analogous to MIDI). Drives SPU hardware
with note events + timing. Sample data lives in a separate SPU bank —
source **unknown** (`MOT.CDB` was a prior candidate but is confirmed animation data, not audio).

### SEQ Data Structure in RAM — CONFIRMED ✅

| Address | Value | Role | XREFs |
|---------|-------|------|-------|
| `0x801bfae8` | PTR → `0x80037ae0` | `g_SEQ_DataPtr` — pointer to active SEQ payload | `FUN_80130764` reads at `0x801307ac` |
| `0x801bfaf0` | `0x00000DF8` (3576) | `g_SEQ_Size` — current SEQ size in bytes | `FUN_8012f3bc`, `FUN_8012f4dc` |
| `0x801bfaf4` | `0xFFFFFFFF` | `g_SEQ_LoopMarker` — loop/end sentinel | `FUN_8012f3bc`, `FUN_8012f4dc` |

> ⚠️ `0x801bfae8` is **data**, not a function. Earlier notes incorrectly identified it as `SEQ_Player`.

Confirmed SEQ Player: **`FUN_80130764`** ✅

Confirmed audio pipeline:
```
SOUND.CDB → CD_LoadFile → 0x80037ae0 (ptr @ g_SEQ_DataPtr 0x801bfae8) → FUN_80130764 (SEQ_Player)
```

> **Next:** Analyze `FUN_80130764` in Ghidra to confirm SEQ parsing logic and SPU dispatch.

### Sound Tables — `0x8011A164` – `0x8011B07C`

**Base:** `0x8011A164` (lida por `Sound_Manager` @ `0x8011d198`).
**SFX index (164 entradas):** `0x8011A174`.

5 arrays de ponteiros consecutivos. Layout em ordem de endereço:

| Index | Entries | Address      |
|-------|---------|--------------|
| 0     | 164     | `0x8011A174` |
| 1     | 111     | (a confirmar)|
| 2     | 102     | (a confirmar)|
| 3     | 100     | (a confirmar)|
| 4     | 92      | (a confirmar)|

Total ≈ 569 entradas. Candidatos fortes a tabelas de índice de SFX e
BGM. A divisão exata por canal lógico (BGM/SFX/Voice) ainda precisa ser
confirmada via XREFs no Ghidra. VAG headers e XA markers não estão
presentes no dump de main-RAM — requerem dump de SPU RAM e captura no
meio de uma cutscene, respectivamente.

#### Sound Table Entry — 16 bytes

```c
typedef struct {
    void     *audio_data;     // +0x00 ponteiro para VAG (sample data)
    uint32_t  vol_params;     // +0x04 volume / params (observado: 0x7C)
    uint32_t  loop_data;      // +0x08 dados de loop
    uint32_t  flags;          // +0x0C flags
} SoundTableEntry;            // sizeof = 0x10
```

#### Assets Referenciados

| Endereço | Símbolo | Conteúdo |
|----------|---------|----------|
| `0x8011A14C` | `s_ModelCDB`  | `"MODEL.CDB"`  — arquivo de modelos |
| `0x8011A158` | `s_ModuleBin` | `"MODULE.BIN"` — bundle de módulos |

### Port Replacement Plan

| PS1                                   | PC Equivalent |
|---------------------------------------|---------------|
| `Audio_SetChannel`                    | `SDL_Mixer` channel enable/disable (`Mix_GroupChannel`/`Mix_HaltGroup`) |
| `Video_SetMode`                       | no-op (decoder de FMV PC gerencia próprio estado) |
| SPU voices shadow tick                | OpenAL `alGenSources` + `alSourcei` por voz; tick por frame propaga shadow → source |
| `SPU_KeyOnOff`                        | `alSourcePlay` / `alSourceStop` por bit de máscara |
| `SPU_SetVolume`                       | `alSourcef(src, AL_GAIN, vol)` |
| `SPU_SetADSR`                         | ADSR em software (curva sobre `AL_GAIN` por frame) |
| SPU voices (24 hardware)              | OpenAL `AL_SOURCE` por voz (24 sources) |
| SEQ (pQES) playback — 95 files        | Option A: decode → standard MIDI + FluidSynth |
|                                       | Option B: pQES interpreter with OpenAL backend |
|                                       | Option C: pre-render all 95 tracks to OGG (simplest) |
| XA.MXA streaming (85 MB)             | Decode XA-ADPCM sectors to PCM on the fly → SDL_Mixer / OpenAL streaming buffer |
| `MOT.CDB` 670 BIN animations + 8 HMD rigs | Implement MOT animation parser → drive HMD bone transforms per frame |
| SPU sample banks | Source **unknown** (MOT.CDB is animation data) — locate via SPU RAM dump |
| ADPCM decode                          | decoder offline; shipar OGG/WAV |
| SPU reverb                            | OpenAL EFX extension |
| Rumble via SPU/SIO                    | `SDL_GameController` rumble / `XInputSetState` |
| `g_FrameCounter`                      | PC frame counter via `QueryPerformanceCounter` |
| Sound tables em `0x8011A174`          | índice de assets PC (paths/IDs por categoria) |
| `Debug_Print`                         | `fprintf(stderr, ...)` ou logger do port |

---

## Audio System (COMPLETE)

### Functions
| Address | Name | Description |
|---------|------|-------------|
| `0x8012ee60` | `SEQ_MIDIVoiceAlloc(param_1, param_2, midi_cmd, voice_table, spu_ptr)` | Processes MIDI Control Change events (0xB0, 0xB2). Allocates SPU voice channels, stride 0x1C per voice entry. SPU Sound RAM limit: 0x19000 bytes (102,400 bytes ≈ 100 KB). Retry logic: up to 3 attempts per voice allocation. |
| `0x8012f3bc` | `Sound_GetEntry(param1, sound_index, param3)` | Returns ptr to `g_VoiceTable + g_VoiceIndex * 0x1C`. Uses `CDB_GetAsset` to load sound data from CDB. |
| `0x80130764` | `SPU_VoiceUpdate()` | Per-frame: processes pending KeyOn/KeyOff/Pitch for all voices. Reads g_VoiceTable entries, calls SPU_StopVoice/SetPitch/KeyOn. |
| `0x8011c454` | `CDB_GetAsset(cdb_handle, asset_index, out_ptr)` | Universal asset getter — 20+ callers. Dispatches to mode A or B reader. Mode selected by FUN_80126ce0() (region/disc check). FUN_8011c1a4 = CDB_GetAsset_ModeA, FUN_8011c2a4 = CDB_GetAsset_ModeB. |
| `0x8016d1cc` | `SPU_StopVoice(channel)` | Called by SPU_VoiceUpdate. |
| `0x8016c210` | `SPU_SetPitch(pitch)` | Called by SPU_VoiceUpdate. |
| `0x801682ac` | `SPU_KeyOn(pitch)` | Called by SPU_VoiceUpdate. |
| `0x8012ecb8` | `SPU_AllocChannel(...)` | Called by SEQ_MIDIVoiceAlloc. |
| `0x8012ee04` | `SPU_CalcEnvelope(...)` | Called by SEQ_MIDIVoiceAlloc. |

### Globals
| Address | Symbol | Description |
|---------|--------|-------------|
| `0x801bfa30` | `g_VoiceTable` | SPU voice entries, stride 0x1C (28 bytes) |
| `0x80191e88` | `g_VoiceIndex` | current voice slot index (short) |
| `0x80191e8c` | `g_SPURamPtr` | current write position in SPU Sound RAM |
| `0x80191e90` | `g_MaxVoiceCount` | maximum voice count (short) |
| `0x801bfc90` | `g_ActiveVoiceCount` | voices being processed this frame |
| `0x801bfbe8` | `g_VoiceTablePtr` | pointer to active voice table |
| `0x801bfaf0` | `g_SEQSize` | size of active SEQ in bytes (0xDF8 = 3576) |
| `0x801bfaf4` | `g_SEQLoopMarker` | 0xFFFFFFFF = loop/end sentinel |
| `0x801bfbd4` | `g_LastBGMVoice` | last BGM voice channel index |
| `0x801bf7dc` | `g_VoiceSlot` | current voice slot being allocated |
| `0x801bfae8` | `g_SEQPtr` | pointer to active SEQ data in RAM (→ 0x80037ae0) |
| `0x80037ae0` | `g_SEQData` | active SEQ file (pQES, 3576 bytes) |

### Voice Entry Layout (0x1C bytes, at g_VoiceTable + index × 0x1C)

| Offset | Type | Field | Notes |
|--------|------|-------|-------|
| `+0x00` | `short` | `midi_cmd` | MIDI command byte (0xB0 or 0xB2) |
| `+0x02` | `short` | `spu_channel` | SPU voice channel number |
| `+0x04` | `uint32` | `sample_ptr` | pointer to sample data |
| `+0x08` | `uint32` | `spu_addr` | address in SPU Sound RAM |
| `+0x0C` | `uint32` | `envelope_params` | ADSR envelope settings |
| `+0x10` | `int` | `active_flag` | 0=stopped, non-zero=playing |
| `+0x12` | `short` | `pad` | |
| `+0x14` | `short` | `pitch_value` | SPU pitch register value |
| `+0x16` | `short` | `trigger_flag` | >=0 = KeyOn pending |
| `+0x18` | `uint32` | `loop_addr` | loop point address in SPU Sound RAM |
| `+0x1A` | `short` | `pad2` | |

### XA Streaming State Machine
Location: `0x801acae0` area

| State | Address | Description |
|-------|---------|-------------|
| `"PAUSE"` | `0x801acb00` | CD drive paused |
| `"PLAY"` | `0x801acb08` | CD drive playing |
| `"READY"` | `0x801acb10` | CD drive ready |
| `"SLEEP"` | `0x801acb18` | CD drive sleeping |
| `"Sleep"` | `0x801c148` | XA audio sleeping |
| `"Ready"` | `0x801c13c` | XA audio ready |
| `"XaSeek"` | `0x801c130` | seeking XA sector on CD |
| `"XaWaitPly"` | `0x801c124` | waiting for XA playback start |
| `"XaPlaying"` | `0x801c118` | XA stream active (music playing) |
| `"Muting"` | `0x801c10c` | muting audio |
| `"Pausing"` | `0x801c100` | pausing audio |

Pipeline: `Sleep → Ready → XaSeek → XaWaitPly → XaPlaying`

### MIDI Commands Used
- `0xB0` = MIDI Control Change channel 0 (BGM)
- `0xB2` = MIDI Control Change channel 2 (SFX/voice)

### Audio Asset Locations in RAM
- SEQ data: `0x80037ae0` (active sequencer, pQES format)
- Sound RAM: SPU internal (0x1F800000 range, 512 KB hardware)
- SPU RAM limit used: `0x19000` bytes (102 KB of 512 KB)

### Port Replacement Plan (PC)

| PS1 | PC Replacement |
|-----|----------------|
| SEQ (pQES MIDI) | FluidSynth or pre-rendered OGG tracks |
| SPU voice alloc | OpenAL `alGenSources` (24 voices max) |
| `SPU_KeyOn` | `alSourcePlay` |
| `SPU_SetPitch` | `alSourcef(AL_PITCH, value/0x1000)` |
| `SPU_StopVoice` | `alSourceStop` |
| XA streaming | SDL_Mixer music with XA-ADPCM decode |
| Sound RAM | OpenAL buffer pool (malloc 100 KB equivalent) |

---

## 💾 Asset Loading

### Confirmed Functions

| Address      | Name                 | Confiança | Description |
|--------------|----------------------|:---------:|-------------|
| `0x8018e2c4` | `CD_LoadFile`        | ✅ | `CD_LoadFile(handle, filename)` — pipeline completo de leitura do CD |
| `0x8018f254` | `CD_CheckReady`      | ✅ | Retorna 0 se o drive ainda está ocupado |
| `0x80185a7c` | `CD_Yield`           | ✅ | Scheduler yield — chamado em loop enquanto aguarda o drive |
| `0x8017d590` | `CD_SetFilename`     | ✅ | `CD_SetFilename(desc, name)` — preenche file descriptor |
| `0x8017d5c0` | `CD_CopyDesc`        | ✅ | `CD_CopyDesc(dest, src)` — copia file descriptor |
| `0x8018e12c` | `CD_Open`            | ✅ | `CD_Open(handle)` — abre handle de arquivo |
| `0x8018e1f4` | `CD_GetNextSector`   | ✅ | `CD_GetNextSector(handle, sectors, 1)` — avança para o próximo setor |
| `0x801855b4` | `malloc`             | ✅ | Alocador interno (PsyQ malloc) |
| `0x8018e234` | `CD_Read`            | ✅ | `CD_Read(handle, buf, sectors, size)` — lê em chunks de `0x800` bytes |
| `0x8018ea20` | `CD_ReadSector`      | ✅ | `CD_ReadSector(sectors, buf, size)` — leitor de setor raw; chama BIOS Table A [0x27] (CdGetSector) — nível mais baixo possível de acesso a CD no PS1 |
| `0x80165528` | `CD_FinishLoad`      | ✅ | Finaliza/aguarda conclusão do load do CD |

### CD Pipeline — 100% MAPPED ✅

Hierarquia completa confirmada (do topo ao hardware):

```
AssetLoader_Init (0x8011ce48)   — carrega todos os CDBs no startup
  └→ CD_LoadFile (0x8018e2c4)   — leitura completa + descompressão LZSS
       └→ CD_Read (0x8018e234)  — loop de chunks de 0x800 bytes
            └→ CD_ReadSector (0x8018ea20) — wrapper de setor único
                 └→ BIOS Table A [0x27] = CdGetSector (hardware)
```

Implementação de `CD_ReadSector` (MIPS):
```asm
li  t2, 0xa0   ; base da BIOS Table A
jr  t2         ; salta para BIOS
li  t1, 0x27   ; função: CdGetSector
```

Este é o nível mais baixo possível de acesso ao CD no hardware PS1.

### Fixed Load Addresses

| Address      | Asset        | Notes |
|--------------|--------------|-------|
| `0x801AD140` | `MODULE.BIN` | Endereço fixo de carregamento do bundle de módulos (2.4 MB overlay code) |
| `0x801AD050` | `MODEL.CDB`  | RAM base — 3D models (~4.8 MB) |
| `0x801AD0C8` | `DISPLAY.CDB`| RAM base — UI textures (~2.5 MB) |
| `0x801ACEA8` | `SOUND.CDB`  | RAM base — audio data (~15.9 MB) |
| `0x801ACFD8` | `ITEMTIM.CDB`| RAM base — item textures (~14.5 MB) |
| `0x801ACF60` | `MENU.CDB`   | RAM base — menu textures (~2.6 MB) |

### Load Tables

| Symbol             | Address      | Entries | Description |
|--------------------|--------------|:-------:|-------------|
| `g_FileTableA`     | `0x80190e0c` | 10      | Load Table A — 10 `{dest,name}` pairs (Disc 1) |
| `g_FileTableB`     | `0x80190e5c` | 7       | Load Table B — 7 `{dest,name}` pairs (Disc 2) |

### CD Driver Globals

| Address      | Symbol            | Description |
|--------------|-------------------|-------------|
| `0x8011c0dc` | `g_CDConfig`      | CD driver config block |
| `0x8011c0e4` | `g_CDDefaultPath` | Default CD path string |

### Disc / Version Flag

| Address      | Symbol         | Description |
|--------------|----------------|-------------|
| `0x80193e08` | `DAT_80193e08` | Flag de disco/versão (controla qual tabela/disco é usado no load) |

### CDB Container Format (confirmado)

Header de **8 bytes** (`CD_GetSize` @ `0x8018e1f4`); o corpo (LZSS ou raw) começa em `+0x08`:

| Offset | Size | Campo               | Notas                                                       |
|--------|------|---------------------|-------------------------------------------------------------|
| `0x00` | 4    | `sector_count`      | Setores CD do payload comprimido (ex.: `108` em MODEL.CDB). |
| `0x04` | 4    | `compression_flag`  | `0` = raw; non-zero = LZSS. MODEL.CDB = `0x00130001`.       |
| `0x08` | …    | body                | LZSS stream ou bytes raw até o fim do arquivo.              |

> Não existe `decompressed_size` nem tabela de sub-arquivos no header — o stream LZSS roda até o fim natural (~14 MB para MODEL.CDB) e os sub-arquivos são localizados por scan de magic bytes word-aligned no payload decomprimido.

LZSS PS1: window `0x1000`, lookahead `0x12`, flag byte LSB-first (`1`=literal, `0`=back-ref).
Ferramenta: `tools/cdb_extractor.py` (1050 sub-files extraídos de MODEL.CDB → 366 TIM + 676 TMD + 8 HMD).

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
| `0x8019b5d8` | `g_DMAStatusPtr` | `void*` | ✅ |
| `0x8019b5e4` | `g_GPUStatusPtr` | `void*` | ✅ |
| `0x8019b5f8` | `g_RenderQueueWrite` | `uint32_t` | ✅ |
| `0x8019b5fc` | `g_RenderQueueRead` | `uint32_t` | ✅ |
| `0x801d04a0` | `g_DrawOTagBase` | `void*` | ✅ |
| `0x801932b4` | `g_OTBuffer_2` | `uint32_t[]` | ✅ |
| `0x80193330` | `g_OTBuffer_0` | `uint32_t[]` (main gameplay OT) | ✅ |
| `0x801933ac` | `g_OTBuffer_1` | `uint32_t[]` (secondary OT) | ✅ |
| `0x80193428` | `g_OTBuffer_3` | `uint32_t[]` | ✅ |
| `0x801934a4` | `g_OTBuffer_4` | `uint32_t[]` | ✅ |
| `0x80193520` | `g_OTBuffer_5` | `uint32_t[]` | ✅ |
| `0x801BFD10` | `g_ActiveCameraMatrix` | `CameraEntry*` | ✅ |
| `0x801C1778` | `g_CameraSlots` | `CameraSlot[4]` (stride `0xC60`) | ✅ |
| `0x801C2FF4` | `g_CameraFuncPtr` | `void*` | ✅ |
| `0x801C3200` | `g_CameraTable` | `CameraEntry*` | ✅ |
| `0x801eb544` | `g_CameraHistoryPtr` | `void*` | ✅ |
| `0x801eb560` | `g_CameraBuffer` | `uint32_t[8]` | ✅ |
| `0x80195C10` | `g_FrameCounter` | `uint32_t` | ✅ |
| `0x801AC830` | `g_AudioVtable` | `void*[7+]` | ✅ |
| `0x80194078` | `SPU_DMA_Table_A` | `uint32_t*` | 🟡 |
| `0x80195FD0` | `SPU_DMA_Table_B` | `uint32_t*` | 🟡 |
| `0x8011A174` | `g_SFXTable` | `void*[164]` | ✅ |
| `0x801E2380` | `g_SPUChannelBankA` | `void**` | ✅ |
| `0x801E2470` | `g_SPUChannelBankB` | `void**` | ✅ |
| `0x8011A14C` | `s_ModelCDB` | `const char[]` | ✅ |
| `0x8011A158` | `s_ModuleBin` | `const char[]` | ✅ |
| `0x80193E30` | `g_ScreenWidth` | `int` | ✅ |
| `0x80193E34` | `g_ScreenHeight` | `int` | ✅ |
| `0x801AD050` | `g_ModelCDB_Base` | `void*` | ✅ |
| `0x801AD0C8` | `g_DisplayCDB_Base` | `void*` | ✅ |
| `0x801ACEA8` | `g_SoundCDB_Base` | `void*` | ✅ |
| `0x801ACFD8` | `g_ItemTimCDB_Base` | `void*` | ✅ |
| `0x801ACF60` | `g_MenuCDB_Base` | `void*` | ✅ |
| `0x80190e0c` | `g_FileTableA` | `pair[10]` `{dest,name}` | ✅ |
| `0x80190e5c` | `g_FileTableB` | `pair[7]` `{dest,name}` | ✅ |
| `0x8011c0dc` | `g_CDConfig` | CD driver config block | ✅ |
| `0x8011c0e4` | `g_CDDefaultPath` | `char*` | ✅ |

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

## 🖥️ Renderer — Pipeline PsyQ (COMPLETE ✅)

### Resolução Nativa (Confirmada)

- `g_ScreenWidth`  (`0x80193E30`) = `0x140` (320 px)
- `g_ScreenHeight` (`0x80193E34`) = `0x0F0` (240 px)
- **Frame timing:** lógica do jogo a 30 FPS, saída de vídeo a 60 FPS.
- **PORT NOTE (FPS Unlock):** desacoplar `update` (fixo a 30 Hz) do `render` (ilimitado) com interpolação entre estados.

### Suporte a Widescreen (Confirmado)

- Alterar valores em `0x80193E30` / `0x80193E34` **antes** de `Engine_Init`.
- O centro de projeção do GTE é atualizado automaticamente via `GTE_SetScreenCenter`
  (`0x8018c008`), que escreve OFX/OFY a partir de `g_ScreenWidth/2` e `g_ScreenHeight/2`.

```
SDK: PsyQ v1.140 — Sony SCEE — 12 Janeiro 1998

Pipeline por frame:
  1. ClearOTag()         inicializa Ordering Table
  2. [primitivas adicionadas à OT via AddPrim]
  3. DrawOTag()          envia OT ao GPU (linked list de primitivas)
  4. DrawSync()          aguarda GPU finalizar
  5. VSync()             aguarda VBlank
  6. Frame_Flip()        PutDispEnv + DrawOTag + clears GPU DMA register

Ordering Table — 6 buffers (COMPLETE):
  g_OTBuffer_0: 0x80193330  (main gameplay OT)
  g_OTBuffer_1: 0x801933ac  (secondary OT)
  g_OTBuffer_2: 0x801932b4
  g_OTBuffer_3: 0x80193428
  g_OTBuffer_4: 0x801934a4
  g_OTBuffer_5: 0x80193520
  Terminador: 0xFFFFFFFC

GPU Hardware Registers (confirmed):
  0x1F8010A8  GPU Display Mode register   → g_GPUStatusPtr (0x8019b5e4)
  0x1F801814  GPU DMA Control register    → cleared on frame flip
  bit 24 of GPU status = GPU busy (drawing)
  bit 26 of GPU status = DMA transfer active

Vtable do Renderer (base 0x8019b4c8):
  -0x0C  ClearImage / ResetGraph  (0x8017afc4)
  -0x08  SetDispMask              (0x8017a1bc)
  -0x04  DrawSync direto          (0x8017b114)
  +0x3C  DrawSync via callback    ← usado por Renderer_Flush

PsyQ DrawOTag base: PTR_PsyQ_DrawOTag_801d04a0 (g_DrawOTagBase)

Formatos de Textura:
  0 = 4bpp  (paleta 16 cores)
  1 = 8bpp  (paleta 256 cores)
  2 = 16bpp (cor direta)
```

### Port Replacement (PC)

| PS1 | PC Equivalent |
|-----|---------------|
| `Frame_Flip` | `glSwapBuffers()` / `vkQueuePresentKHR()` |
| `DrawOTag` | iterate OT linked list → convert prims → draw calls |
| `DrawSync` | `glFinish()` / `vkWaitForFences()` |
| `PutDispEnv` | `glViewport()` + framebuffer bind |
| `RenderQueue_Dispatch` | OpenGL command buffer / Vulkan command buffer |

---

## 🚧 Questões Abertas

- [ ] **Relação HP:** `EngineState.rion_hp_mirror` vs `GlobalCombatState->hp` — são o mesmo dado ou cópias sincronizadas?
- [ ] **Slot de Gameplay:** qual índice (0-15) contém `Handle_Combat_State`?
- [ ] **Tamanho total da EngineState:** gap entre `0x801ACB66` e `0x801AE158` ainda não mapeado
- [x] **Sistema de câmera:** pipeline completo mapeado — `Camera_LoadToGTE`, `Camera_Manager` e `Camera_RecordFrame` confirmados
- [x] **Camera_RecordFrame (0x80187350):** ring buffer de histórico de frames confirmado; `g_CameraHistoryPtr` (0x801eb544) e stride 0x20
- [ ] **CameraSlot interno (0x004 – 0xC3D):** payload não mapeado entre `func_ptr` e `active_flag`
- [ ] **Relação `g_CameraTable` ↔ `g_CameraSlots`:** 0x801C3200 cai dentro do range dos slots (0x801C1778 + 0x3180) — verificar sobreposição
- [x] **DrawOTag:** PTR_PsyQ_DrawOTag_801d04a0 confirmed (g_DrawOTagBase); called via Frame_Flip (0x8017b06c)
- [ ] **ClearOTag:** endereço não confirmado
- [ ] **Engine_EnqueueLoadImage:** endereço a confirmar
- [ ] **FUN_80184f24 / FUN_801850c4 callers adicionais:** `FUN_80184f24` e `FUN_801850c4` têm outros XREFs a analisar

---

## 🗓️ Roadmap do Port Windows

### Fase 1 — Mapeamento (Em Andamento)
- [x] Game Loop / Scheduler
- [x] Sistema de Combate
- [x] Sistema de Input
- [x] Sistema de Corrotinas
- [x] Renderer (completo — DrawOTag, Frame_Flip, 6 OT buffers, GPU registers all confirmed)
- [x] Sistema de Câmera (completo — `Camera_LoadToGTE`, `Camera_Manager`, `Camera_RecordFrame`)
- [x] Sistema de Áudio (SOUND.CDB: 95 pQES SEQ files confirmados; SPU bank candidates identificados)
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
- [ ] Widescreen (ajuste de FOV horizontal — via `GTE_SetScreenCenter` @ `0x8018c008`, OFX/OFY)
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