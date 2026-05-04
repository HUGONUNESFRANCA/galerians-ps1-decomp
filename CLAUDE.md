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
- Priority 1: Run cdb_extractor.py on MDT.CDB (check for VH/VB audio bank headers)
- Priority 2: Analyze XA.MXA structure (XA sectors interleaved with data?)
- Priority 3: Find SEQ player function in Ghidra (search for pQES magic: 70 51 45 53)
- Priority 4: Analyze FUN_8011d198 (Sound Manager) - paste pseudo-C
- Priority 5: Analyze FUN_8018669c (Frame_First after Engine_Init)
- Priority: Implement GTE_H (COP2 0xE800) adjustment for true widescreen FOV to prevent 16:9 stretching.
- Mapear gap da EngineState (0x801ACB66 – 0x801AE158)
- Identificar slot de gameplay na state machine