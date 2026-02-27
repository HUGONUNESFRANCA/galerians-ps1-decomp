
//Incluindo biblioteca dos tipos do Ghidra
#include "ghidra_types.h"

//Incluindo funções externas temporariamente até a descoberta de suas funções

// --- DECLARAÇÃO DE VARIÁVEIS EXTERNAS E DE PILHA (STACK) ---
extern int DAT_80193228;
extern int stack0x000005d2;
extern int stack0x000005d0;
extern int stack0x00000670;
extern int stack0x00000690;

// --- DECLARAÇÃO DE FUNÇÕES DESCONHECIDAS ---
extern void FUN_80177fa0();
extern void FUN_8017d610();
extern void FUN_80130cfc();
extern void FUN_80156908();

// Endereço base na RAM (Status Globais): 0x801C2F9C
void Calculate_Entity_Damage(void)
{
  short attack_val1;
  int iVar1;
  int iVar2;
  short attack_val2;
  short *hp_pointer;
  int unaff_s4;
  int unaff_s6;
  int unaff_s8;
  short modifier;
  int in_stack_000006ac;
  int in_stack_00000704;
  
  // Fórmula principal de dano do jogo
  *hp_pointer = *hp_pointer + attack_val1 + (modifier - attack_val2) * -3;
  
  if (unaff_s6 < 0x10) {
    iVar1 = unaff_s6 * 8;
    
    // Checagem do "Índice 1" do Array de Status (Provavelmente AP ou Infecção)
    if (hp_pointer[1] < 0x11) 
    {
      iVar2 = (uint)(ushort)hp_pointer[1] << 0x10;
                    /* WARNING: Subroutine does not return */
      *(short *)(&stack0x000005d2 + iVar1) =
           *(short *)(&stack0x000005d2 + iVar1) + (short)((iVar2 >> 0x10) - (iVar2 >> 0x1f) >> 1);
      FUN_80177fa0(&stack0x000005d0 + iVar1,&stack0x00000690);
    }
                    /* WARNING: Subroutine does not return */
    FUN_8017d610();
  }
  
  // Checagem de Flags Globais de Estado (Bitwise Mask: 0x400)
  if (((*(uint *)(DAT_80193228 + 0xae8) & 0x400) == 0) && (0 < in_stack_000006ac)) 
  {
    if ((in_stack_000006ac == 0x40) || (in_stack_000006ac == 0x32)) 
    {
      FUN_80130cfc(*(undefined4 *)(in_stack_00000704 + 0x183c),0,7,0x3c);
    }
    FUN_80156908();
    return;
  }
                    /* WARNING: Subroutine does not return */
  FUN_80177fa0(&stack0x00000670,unaff_s4 + *(int *)(unaff_s8 + 0x258c) * 0x28 + 8);
}