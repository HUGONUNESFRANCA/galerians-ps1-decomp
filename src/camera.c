#include "include/camera.h"
#include "include/ghidra_types.h"

/*
 * Camera_LoadToGTE — 0x80187338  ✅ Confirmado
 *
 * Carrega a GTEMatrix da câmera nos registradores COP2 (GTE):
 *   R11-R33  ← cam->gte.m[3][3]   (matriz de rotação)
 *   TRX/Y/Z  ← cam->gte.t[3]      (vetor de translação)
 *
 * Após esta chamada, instruções GTE como RTPS/RTPT transformam
 * vértices do espaço mundo para o espaço câmera.
 *
 * Equivalente PsyQ:
 *   SetRotMatrix((MATRIX *)&cam->gte);
 *   SetTransMatrix((MATRIX *)&cam->gte);
 *
 * Equivalente PC (Phase 2):
 *   Construir view matrix 4×4 a partir de gte.m + gte.t
 *   e enviar ao shader via glUniformMatrix4fv.
 */
void Camera_LoadToGTE(const CameraEntry *cam)
{
    /*
     * No PS1 original, esta função escreve diretamente nos
     * registradores COP2 via instruções MTC2 (Move To Coprocessor 2).
     *
     * Reconstrução (pseudocódigo GTE):
     *
     *   GTE_R11 = cam->gte.m[0][0];  GTE_R12 = cam->gte.m[0][1];
     *   GTE_R13 = cam->gte.m[0][2];
     *   GTE_R21 = cam->gte.m[1][0];  GTE_R22 = cam->gte.m[1][1];
     *   GTE_R23 = cam->gte.m[1][2];
     *   GTE_R31 = cam->gte.m[2][0];  GTE_R32 = cam->gte.m[2][1];
     *   GTE_R33 = cam->gte.m[2][2];
     *   GTE_TRX = cam->gte.t[0];
     *   GTE_TRY = cam->gte.t[1];
     *   GTE_TRZ = cam->gte.t[2];
     *
     * Stub — implementação real requer intrínsecos GTE ou inline asm MIPS.
     */
    (void)cam;
}

/*
 * Camera_SetActive — endereço a confirmar (🔴 reconstruído)
 *
 * Copia a entrada 'index' da g_CameraTable para g_ActiveCamera
 * e carrega nos registradores GTE.
 *
 * Hipótese baseada no padrão comum de câmeras fixas PS1:
 *   1. Cada sala define um ou mais ângulos de câmera na tabela.
 *   2. Ao entrar num trigger de câmera, o jogo chama esta função.
 *   3. Camera_LoadToGTE é chamada a cada frame para manter GTE atualizado.
 */
void Camera_SetActive(int index)
{
    *g_ActiveCamera = g_CameraTable[index];
    Camera_LoadToGTE(g_ActiveCamera);
}

/*
 * Camera_BuildMatrix — endereço a confirmar (🔴 reconstruído)
 *
 * Reconstrói cam->gte.m[][] a partir dos ângulos Euler rot_x/y/z.
 *
 * PS1 usa ângulos em [0, 4095] onde 4096 == 360° (= 2π rad).
 * A GTE opera com senos/cossenos pré-calculados via tabela (rcos/rsin)
 * ou usando a instrução GTE ROTY/ROTZ/ROTX.
 *
 * Nota: câmeras fixas pré-calculam a matriz offline (no editor de nível)
 * e a armazenam na ROM — esta função pode nunca ser chamada em runtime.
 * Manter como referência de como a matriz seria gerada.
 *
 * Equivalência de ângulo:
 *   float rad = (float)rot * (2.0f * M_PI / 4096.0f);
 */
void Camera_BuildMatrix(CameraEntry *cam)
{
    /*
     * Stub — reconstrução requer tabela de seno/cosseno do PsyQ
     * (rcos[]/rsin[] em libmath) ou conversão float para PC port.
     */
    (void)cam;
}
