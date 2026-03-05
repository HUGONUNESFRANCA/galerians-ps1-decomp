#ifndef ENTITY_H
#define ENTITY_H

// Estrutura base de qualquer personagem/entidade do jogo
// Tamanho Total: 56 bytes (0x38)
typedef struct {
    short ID;             // Offset 0x00: Identificador da entidade
    short Coord_X;        // Offset 0x02: Posição X no mundo 3D
    short Coord_Y;        // Offset 0x04: Posição Y no mundo 3D
    short Coord_Z;        // Offset 0x06: Posição Z no mundo 3D
    short Entity_Type;    // Offset 0x08: Filtro/Tipo da entidade
    
    char padding1[18];    // Offset 0x0A ao 0x1B: (Ainda não documentado)
    
    short HP;             // Offset 0x1C: Pontos de vida atuais
    short MaxHP;          // Offset 0x1E: Pontos de vida máximos
    
    char padding2[24];    // Offset 0x20 ao 0x37: (Ainda não documentado)
} PlayerObject;

#endif