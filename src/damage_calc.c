#include "ghidra_types.h"
#include "rion_stats.h" // Importando a nossa nova estrutura

// ... (variaveis extern e funcoes) ...

void Calculate_Entity_Damage(void) {
  short attack_val1;
  short attack_val2;
  GlobalCombatState *rion_stats; // Agora o ponteiro sabe o formato exato dos dados!
  short modifier;
  
  // A formula de dano atualizando a propriedade HP diretamente:
  rion_stats->HP = rion_stats->HP + attack_val1 + (modifier - attack_val2) * -3;
  
  // Aquela checagem bizarra do hp_pointer[1] vira isso:
  if (rion_stats->AP < 0x11) {
      // Rotinas de reacao ao AP baixo ou Game Over
      FUN_8017d610(); 
  }
}