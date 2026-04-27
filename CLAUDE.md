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
- Localizar sistema de câmera
- Mapear gap da EngineState (0x801ACB66 – 0x801AE158)
- Identificar slot de gameplay na state machine
- Check values at 0x80193E30 and 0x80193E34 in DuckStation to confirm native resolution (expected: 320 and 240).
- Analyze FUN_8018669c (Frame_First) - last unknown in Engine_Init.
- Analyze 0x8011A174 XREFs to find Sound Manager.