<div align="center">
  <h1> Galerians (PS1) - Reverse Engineering & Decompilation</h1>
  
  <p>
    <b>Um projeto educacional focado em entender a arquitetura do PlayStation 1 e a lógica por trás do clássico de 1999.</b>
  </p>

  <img src="https://img.shields.io/badge/Platform-PlayStation%201-lightgrey?style=for-the-badge&logo=playstation" alt="PS1">
  <img src="https://img.shields.io/badge/Language-C%20%2F%20C%2B%2B-blue?style=for-the-badge&logo=c%2B%2B" alt="C/C++">
  <img src="https://img.shields.io/badge/Status-Work%20in%20Progress-orange?style=for-the-badge" alt="WIP">
</div>

---

## 🎯 Sobre o Projeto

Este repositório documenta a minha jornada pessoal desconstruindo o jogo **Galerians (PS1)**. 

O objetivo principal aqui **não é** criar um "port" jogável imediato, mas sim utilizar este desafio como um laboratório prático para consolidar conceitos de:
- Arquitetura de Computadores (MIPS, CPU do PS1).
- Engenharia Reversa de binários compilados.
- Análise de memória e manipulação de hexadecimais.
- Estruturas de dados de jogos clássicos.

> **⚠️ Aviso Legal (Disclaimer):** Este projeto tem fins estritamente educacionais e de pesquisa. **Nenhum arquivo original do jogo (ROM, BIN, CUE, ISO), código proprietário vazado ou asset protegido por direitos autorais (áudio, texturas, modelos 3D) é fornecido neste repositório.** Para utilizar as ferramentas ou testar o código aqui presente, é necessário possuir uma cópia legal e original do jogo.

---

## 🛠️ Ferramentas Utilizadas

Meu fluxo de trabalho combina ferramentas de emulação e análise de software:

- **[Ghidra](https://ghidra-sre.org/):** Para a análise estática e decompilação dos binários do jogo (arquivos executáveis do PS1).
- **[DuckStation](https://github.com/stenzek/duckstation):** Emulador focado em precisão, utilizado para testes em tempo real e debug.
- **[Cheat Engine](https://www.cheatengine.org/):** Acoplado ao emulador para análise dinâmica de memória (mapeamento de variáveis de estado, HP, AP, inventário).
- **[CDMage](https://www.videohelp.com/software/CDMage):** Para extração e manipulação dos setores dos arquivos de imagem do CD original.

---

## 🗺️ Mapeamento de Memória (Exemplo Inicial)

*Esta seção será atualizada conforme novas descobertas forem feitas.*

| Endereço RAM | Tamanho | Tipo | Descrição |
| :--- | :--- | :--- | :--- |
| `0x800XXXXX` | 2 bytes | `short` | HP atual do Rion. |
| `0x800XXXXX` | 1 byte | `byte` | AP (Aura Point) gauge. |
| `0x800XXXXX` | 4 bytes | `int` | Ponteiro para o inventário atual. |

---

## 📁 Estrutura do Repositório

```text
galerians-ps1-decomp/
├── docs/           # Notas de pesquisa, mapas de memória e documentação
├── src/            # Código C/C++ reconstruído a partir da decompilação
├── tools/          # Scripts em Python/Batch que criei para ajudar na extração
└── README.md       # Este arquivo
