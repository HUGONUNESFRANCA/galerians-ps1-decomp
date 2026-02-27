1. Arquitetura Geral do Jogo

Sistema de Entidades: O jogo gerencia até 24 entidades simultâneas (Rion, inimigos, NPCs) na memória.

Tamanho do Objeto: Cada entidade ocupa exatos 56 bytes (0x38) na memória RAM.

Endereço Base Principal: O array de entidades (onde o Rion costuma ficar no índice 0) começa no endereço 0x801cd4a8.

Overlays (Código Dinâmico): A lógica de combate e os textos do jogo não ficam no executável principal (SLUS). O jogo usa "Portais" (Thunk Functions) para carregar arquivos extras (como .BIN ou .CDB) do CD direto para a RAM em momentos de transição ou combate.

2. Variáveis Globais Descobertas

0x801cdce0 -> Global_Active_Entity_ID: Armazena o ID (0, 1, 2...) da entidade que está sendo processada no momento.

0x801eb6c8 -> Global_Game_State: Controla o estado da engine (0 = Init, 1 = Normal, 2 = Início de Transição, 3 = Fim de Transição).

3. Funções do Executável (SLUS) Mapeadas

FUN_8015c230 -> Init_Entities: Chamada ao carregar uma sala. Zera os dados e o HP de até 24 slots de memória para evitar lixo da sala anterior.

FUN_8015e200 -> Update_Audio_Volumes (ou Play_Spatial_Sound): Lê as coordenadas X, Y e Z das entidades para calcular o volume posicional/3D e o pan (L/R) do áudio (limitado a 127/0x7F).

FUN_8017c6d4 -> Update_Game_State (ou Main_Engine_Loop): O "coração" do motor. Controla o relógio de 30 FPS, efeitos de Fade In/Out da tela e aciona os arquivos dinâmicos (Overlays).

Funções Thunk (Portais para os Overlays):

8ffc0014 -> Call_Overlay_Render: Portal geralmente focado na atualização gráfica contínua.

8ffc0034 -> Call_Overlay_Logic: Portal engatilhado para eventos dinâmicos ou checagem de controle.