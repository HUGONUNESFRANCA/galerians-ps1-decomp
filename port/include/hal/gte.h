#ifndef HAL_GTE_H
#define HAL_GTE_H

#include <stdint.h>
#include <glm/glm.hpp>

/*
 * HAL — GTE replacement
 *
 * The PS1 Geometry Transformation Engine (COP2) is replaced by GLM on
 * the PC. The HAL keeps the same call surface used in the original
 * code so engine modules can be ported with minimal changes.
 *
 * PS1 GTE fixed-point: 12-bit fraction (4096 = 1.0).
 * Conversion: float = int16_value / 4096.0f  (see PS1_FIXED_POINT).
 *
 * The GTE H register controls the projection plane distance and acts
 * as the FOV control — adjusting it is the path to true widescreen.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* PORT_NOTE: PS1 writes OFX (0xC000) and OFY (0xC800) GTE control regs.
 * On PC we cache (x, y) and rebuild the projection matrix in
 * HAL_GTE_SetProjection. */
void HAL_GTE_SetScreenCenter(int x, int y);

/* PORT_NOTE: GTE H register = projection plane distance.
 * Smaller h => wider FOV. PS1 stock value ≈ 320 (matches native width).
 * For a 16:9 widescreen FOV we adjust h with PORT_SCALE_X. */
void HAL_GTE_SetProjection(int h);

/* PORT_NOTE: PsyQ Camera_LoadToGTE (0x80187320) writes a packed PS1
 * MATRIX (9 × int16, row-major, 4096-fixed) into the GTE scratchpad
 * at 0x1F800168. On PC we convert it to a glm::mat4 view matrix and
 * stash it for later HAL_GTE_Project calls. */
void HAL_GTE_LoadMatrix(const int16_t matrix[9]);

/* Returns clip-space {x, y, z, w}. Caller does perspective divide
 * (x/w, y/w) and applies HAL_SetScreenCenter offset. */
glm::vec4 HAL_GTE_Project(const glm::vec3 &v);

/* Accessors for engine code that wants the GLM forms directly. */
const glm::mat4 &HAL_GTE_GetView(void);
const glm::mat4 &HAL_GTE_GetProjection(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_GTE_H */
