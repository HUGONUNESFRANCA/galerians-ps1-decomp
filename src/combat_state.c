#include "ghidra_types.h"
#include "rion_stats.h"

// --- VARIAVEIS GLOBAIS ---
extern int Global_Continues_Count; // Antigo DAT_80193250
extern int _DAT_801eb574;

// --- FUNCOES EXTERNAS ---
extern void Trigger_GameOver(void); // Antigo FUN_8018539c
extern void FUN_8018536c(void);
extern void FUN_80177f40(int param);
extern void FUN_80177f70(int param);
extern char FUN_80177f90(void);
extern void FUN_8017d2e0(int param1, int param2);
extern void FUN_80177fa0(void *param1, void *param2);
extern void FUN_8017d610(void *param);

// Função Gerenciadora de Estados de Combate e Vida
void Handle_Combat_State(void) {
  char cVar1;
  int in_t0; // Ponteiro base da engine
  int combat_state_flag; // Antigo in_stack_000006b0
  int unaff_s4;
  int unaff_s8;
  undefined6 uVar2;
  GlobalCombatState *rion_stats; // O nosso ponteiro tipado
  
  // --- Variaveis de Stack que o Ghidra encontrou ---
  int stack0x000005d0;
  int stack0x00000690;
  int stack0x000000d0;
  
  // 1. O GATILHO DA MORTE: Checa se a vida recem calculada ficou negativa
  if (*(int *)(in_t0 + 0x1858) < 0) {
    Trigger_GameOver(); // O jogo acaba aqui!
  }
  
  FUN_80177f40(_DAT_801eb574);
  FUN_80177f70(_DAT_801eb574);
  cVar1 = FUN_80177f90();
  
  // 2. MAQUINA DE ESTADOS DO RION
  if (combat_state_flag == 1) {
    FUN_8017d2e0(unaff_s4 + *(int *)(unaff_s8 + 0x258c) * 0x28, 1);
  }
  
  if (combat_state_flag < 2) {
    if (combat_state_flag == 0) {
      FUN_8017d2e0(unaff_s4 + *(int *)(unaff_s8 + 0x258c) * 0x28, 0);
    }
  } 
  else {
    if (combat_state_flag == 2) {
      // Checa o limite da barra de AP (Short/Addiction)
      if (rion_stats->Max_AP < 0x11) {
        FUN_80177fa0(&stack0x000005d0, &stack0x00000690);
      }
      FUN_8017d610(&stack0x000000d0); // A nossa funcao "quente"
    }
    
    // 3. LOGICA DE CONTINUE / ITEM DE REVIVER
    if (combat_state_flag == 9) {
      Global_Continues_Count = Global_Continues_Count - 1; // Subtrai 1 Continue
      FUN_8018536c();
      Trigger_GameOver(); // Aciona a morte final
    }
  }
  
  // Rotinas de limpeza e atualizacao final da engine
  FUN_8016cbe0();
  FUN_8016d4d0();
  
  if (cVar1 != '\0') {
    if (cVar1 == '\x04') {
      FUN_8016d4b0();
    }
    return;
  }
  
  uVar2 = FUN_8016d490();
  *(short *)((int)uVar2 + 0x3c) = (short)((uint6)uVar2 >> 0x20);
  return;
}