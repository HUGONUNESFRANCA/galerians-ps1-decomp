# Galerians PS1 Decompilation Project

## Sobre o Projeto
Decompilação e port nativo para Windows do jogo Galerians (PS1, 1999).
SDK original: PsyQ v1.140 (Sony SCEE, Janeiro 1998).
CPU: MIPS R3000A, Little Endian.

## Estrutura
- include/   → headers com structs e endereços mapeados
- src/       → código C reconstruído do Ghidra
- docs/      → memory_map.md e documentação de análise
- tools/     → scripts Python/Batch auxiliares

## Endereços Críticos
- Game Loop:    0x80185170
- Rion Stats:   0x801C2F9C
- Engine Base:  0x801AC8D8
- Input System: 0x801AC900

## Convenções
- Funções confirmadas: nomes descritivos (ex: GameLoop_Main)
- Funções suspeitas: prefixo FUN_ mantido até confirmação
- Variáveis globais: prefixo g_
- Confiança: ✅ Confirmado / 🟡 Alta / 🟠 Média / 🔴 Baixa

## Próximas Tarefas
- Priority 1: Analyze FUN_80130764 in Ghidra (confirmed SEQ Player)
- Priority 2: Find MOT binary format magic — search Ghidra for code that reads 0x80037ae0 area or MOT.CDB data
- Priority 3: Identify the 8 HMD character names (slots 0-7 in MODEL.CDB / MOT.CDB)
- Priority 4: Check XA.MXA structure for XA sector headers
- Priority 5: Analyze FUN_8011d198 (Sound Manager) - paste pseudo-C
- Priority 6: Analyze FUN_8018669c (Frame_First after Engine_Init)
- Priority: Implement GTE_H (COP2 0xE800) adjustment for true widescreen FOV to prevent 16:9 stretching.
- Mapear gap da EngineState (0x801ACB66 – 0x801AE158)
- Identificar slot de gameplay na state machine