# Galerians (PS1) - Reverse Engineering & Decompilation

Um projeto educacional de engenharia reversa focado em entender a arquitetura do PlayStation 1 e a lógica de programação por trás do clássico *Galerians* de 1999.

## 🎯 Sobre o Projeto

Este repositório documenta a desconstrução do jogo *Galerians* (PS1). O objetivo principal não é criar um "port" jogável imediato, mas utilizar este desafio como um laboratório prático e aprofundado para consolidar conceitos de:

* Arquitetura de Computadores (MIPS R3000A, CPU do PS1).
* Engenharia Reversa de binários em C e uso de SDKs de época (PsyQ v1.140).
* Análise de memória, manipulação de ponteiros e arquitetura de *Game Loops* clássicos.
* Máquinas de estado (*State Machines*) e agendadores de corrotinas (*Coroutines*).

> **⚠️ Aviso Legal (Disclaimer):** Este projeto tem fins estritamente educacionais e de pesquisa. Nenhum arquivo original do jogo (ROM, BIN, CUE, ISO), código proprietário vazado ou asset protegido por direitos autorais (áudio, texturas, modelos 3D) é fornecido neste repositório. Para utilizar as ferramentas ou testar o código aqui presente, é necessário possuir uma cópia legal e original do jogo.

## ⚙️ Ficha Técnica do Alvo
* **Plataforma:** PlayStation 1
* **SDK Original:** PsyQ v1.140 — Sony SCEE (Compilado em 12/01/1998)
* **Arquitetura:** MIPS R3000A (Little Endian, 32-bit)
* **RAM:** 0x80000000 – 0x801FFFFF (2MB)

## 🛠️ Ferramentas Utilizadas

O fluxo de trabalho combina análise estática e depuração dinâmica profunda:
* **Ghidra:** Para a análise estática, mapeamento de ponteiros e descompilação dos executáveis do PS1 gerando código em C.
* **DuckStation:** Emulador focado em precisão, utilizado com seu *CPU Debugger* interno para *Watchpoints*, *Breakpoints* de execução e *Dumps* de memória RAM.
* **CDMage:** Para extração e manipulação dos setores dos arquivos de imagem do CD original.

## 🗺️ Mapeamento de Memória (Destaques)

Abaixo estão algumas das principais estruturas já mapeadas na RAM do console. Para o mapeamento completo e offsets detalhados, consulte a documentação em `docs/MemoryMap.md`.

| Endereço (RAM) | Tamanho | Estrutura | Descrição |
| :--- | :--- | :--- | :--- |
| `0x801AC8D8` | - | `EngineState` | Struct Mestre da Engine. Gerencia ponteiros globais de estado. |
| `0x801C2F9C` | 14 bytes | `GlobalCombatState` | Status do Rion. Contém HP (`int16`), AP (`int16`) e nível de *Shorting*. |
| `0x801AC900` | 16x 0x40 | `ControllerChannel` | Array de portas de controle. Mapeia botões digitais, DualShock e *Rumble*. |
| `0x801D2198` | 112 slots| `CoroutineContext` | Pool do *Scheduler*. Gerencia o tempo de vida e *Yields* da lógica do jogo. |
| `0x80193358` | Buffer | `OrderingTable` | Fila de renderização principal (Buffer 0) enviada para a GPU. |

## 🗓️ Roadmap do Projeto

### Fase 1 — Mapeamento Estrutural (Em Andamento)
- [x] Game Loop / Scheduler
- [x] Sistema de Combate
- [x] Sistema de Input
- [x] Sistema de Corrotinas
- [x] Renderer (parcial)
- [ ] Sistema de Câmera
- [ ] Sistema de Áudio
- [ ] Sistema de FMV/MDEC
- [ ] Overlays de mapa/área

### Fase 2 — Port Base (C Puro)
- [ ] Substituir VSync do PS1 por timer nativo de OS (ex: *QueryPerformanceCounter*).
- [ ] Substituir *DrawOTag* por renderizador moderno (OpenGL/Vulkan).
- [ ] Substituir *LoadImage* por chamadas de carregamento de texturas (ex: *glTexImage2D*).
- [ ] Substituir hardware de input do PS1 por bibliotecas modernas.

### Fase 3 — Melhorias Opcionais
- [ ] FPS desbloqueado (separar update/render com acumulador de tempo).
- [ ] Resolução aumentada nativamente (framebuffer escalado).
- [ ] Suporte a Texturas HD.
- [ ] Ajuste para Widescreen (FOV horizontal expandido).

## 📁 Estrutura do Repositório

```text
galerians-ps1-decomp/
├── docs/           # Mapas de memória, tabelas de offsets e documentação da PsyQ
├── src/            # Código C puro reconstruído a partir da descompilação
├── tools/          # Scripts criados para extração e automação
└── README.md       # Este arquivo
