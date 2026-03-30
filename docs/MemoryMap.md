# Galerians PS1 — Memory Map

> Última atualização: sessão de reverse engineering — game loop encontrado

## Structs Mapeadas

| Endereço Base | Struct | Status |
|---|---|---|
| `0x801AC900` | `EngineState` (g_EngineBase) | Parcial |
| `0x801C2F9C` | `GlobalCombatState` (Rion) | Confirmado |

## Variáveis Globais

| Endereço | Nome | Tipo | Status |
|---|---|---|---|
| `0x801AC900` | `g_EngineBase` | `EngineState*` | ✅ |
| `0x801ACB5E` | `g_ActiveStateIndex` | `uint16_t` | ✅ |
| `0x801ACB5C` | `g_StateSlotsActive` | `uint16_t` | ✅ |
| `0x801ACB64` | `g_StateSlotMask` | `uint16_t` | ✅ |
| `0x801E2218` | `g_StateTable_EntryPtrs` | `void*[16]` | ✅ |
| `0x801E21D8` | `g_StateTable_RetAddrs` | `void*[16]` | ✅ |
| `0x801CB3C0` | `g_DeathCount` | `uint32_t` | ✅ |
| `0x801CB3C4` | `g_ContinuesLeft` | `uint16_t` | ✅ |
| `0x80193250` | `g_ContinuesCount` | `int32_t` | ✅ |
| `0x801C2F9C` | `g_RionStats` | `GlobalCombatState` | ✅ |
| `0x801E2198` | `g_StateTable_VsyncData` | `uint32_t[16]` | ✅ Novo |
| `0x80185510` | `FrameCycle_Orchestrator` | — | 🔴 Analisar |


## Funções Mapeadas

| Endereço | Nome | Status |
|---|---|---|
| `0x80185170` | `GameLoop_Main` | ✅ Confirmado |
| `0x8017e0fc` | `VSync_WaitFrameSync` | ✅ Confirmado |
| `0x80172bec` | `StateMachine_Dispatch` | ✅ Confirmado |
| `0x801854d4` | `StateSlot_Execute` | ✅ Confirmado |
| `0x8016019c` | `StateSlot_Allocate` | ✅ Confirmado |
| `0x8018539c` | `Trigger_GameOver` | ✅ Confirmado |
| `0x8013a704` | `Death_IncrementAndTrigger` | ✅ Confirmado |
| `0x80128df8` | `Engine_ResetDeathState` | 🟡 Alta |
| `0x8017d610` | `AP_CriticalEffect` | 🟡 Alta |
| `0x80177f90` | `Check_BattleEvent` | 🟡 Alta |
| `0x8017d2e0` | `Toggle_EntityState` | 🟡 Alta |
| `0x80165f0c` | `Calc_TileIndex` | 🟠 Média |
| `0x80178d1c` | `Renderer_Flush` | 🟠 Média |
| `0x8017e040` | `Video_SetMode` | 🟠 Média |
| `0x8017e050` | `Audio_SetChannel` | 🟡 Alta |
| `0x8016cbe0` | `Engine_FrameCleanup_A` | 🟠 Média |
| `0x8016d4d0` | `Engine_FrameCleanup_B` | 🟠 Média |
| `0x8014ced4` | `GameOver_Screen` | 🟡 Alta |
| `0x80185510` | `Context_Save` | Coroutine context switcher — scheduler cooperativo |
| `0x80184f24` | `FUN_80184f24` | 🔴 Analisar — chama Context_Save |
| `0x801850c4` | `FUN_801850c4` | 🔴 Analisar — chama Context_Save |


## State Machine — Slots Conhecidos

| Slot | Sistema | Função Init |
|---|---|---|
| 0 | Vídeo / FMV | `Video_SetMode` + `Audio_SetChannel(3,1)` |
| 4 | Música BGM | `Audio_SetChannel(0,1)` |
| 5 | SFX | `Audio_SetChannel(1,1)` |
| 6 | Voz / Diálogo | `Audio_SetChannel(2,1)` |
| ? | Gameplay | Contém `Handle_Combat_State` |

## Tabelas da State Machine (4 confirmadas)

| Endereço    | Nome                    | Tipo          | Status      |
|-------------|-------------------------|---------------|-------------|
| 0x801E2198  | g_StateTable_VsyncData  | uint32_t[16]  | ✅ Confirmado|
| 0x801E21D8  | g_StateTable_FuncPtrs   | void*[16]     | ✅ Confirmado|
| 0x801E2218  | g_StateTable_EntryPtrs  | void*[16]     | ✅ Confirmado|
| 0x801E2258  | g_LastSlot_A0           | uint32_t      | ✅ Confirmado|
| 0x801E225C  | g_LastSlot_A1           | uint32_t      | ✅ Confirmado|


## Questões Abertas

- [ ] Relação entre `EngineState+0x1858` e `GlobalCombatState->hp`
- [ ] Completar disassembly de `GameLoop_Main` via dump de RAM
- [ ] Identificar slot de gameplay na state table
- [ ] Mapear campos desconhecidos de `EngineState`

## Funções do Scheduler

| Endereço    | Nome                    | Status        |
|-------------|-------------------------|---------------|
| 0x80185170  | StateSlot_Scheduler     | ✅ Confirmado  |
| 0x8017e0fc  | VSync_WaitFrameSync     | ✅ Confirmado  |
| 0x80185510  | Context_Restore         | ✅ Confirmado  |
| 0x80185548  | Context_SaveReturnParams| 🟡 Analisar   |
| 0x80184f24  | FUN_80184f24            | 🔴 Analisar   |
| 0x801850c4  | FUN_801850c4            | 🔴 Analisar   |