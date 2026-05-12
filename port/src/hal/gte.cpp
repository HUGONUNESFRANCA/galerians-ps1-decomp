/*
 * HAL — GTE replacement (GLM)
 *
 * Replaces the PS1 Geometry Transformation Engine (COP2). Internal
 * state mirrors what the GTE control registers hold: a 3x3 rotation
 * matrix + translation (loaded from a packed PS1 MATRIX), a screen
 * center (OFX/OFY equivalent), and a projection plane distance (H).
 */

#include "hal/gte.h"
#include "port_config.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {

glm::mat4 g_view = glm::mat4(1.0f);
glm::mat4 g_proj = glm::mat4(1.0f);

float g_center_x = PORT_TARGET_WIDTH  * 0.5f;
float g_center_y = PORT_TARGET_HEIGHT * 0.5f;
int   g_h         = PORT_NATIVE_WIDTH;       /* matches PS1 stock 320 */

void RebuildProjection(void) {
    /* PS1 GTE projection is essentially:
     *   sx = (vx * h) / vz + ofx
     *   sy = (vy * h) / vz + ofy
     * which is a standard pinhole camera with focal length = h.
     * For a true 16:9 widescreen FOV we scale h by 1/PORT_SCALE_X so
     * the horizontal field of view widens instead of stretching. */
    float aspect = float(PORT_TARGET_WIDTH) / float(PORT_TARGET_HEIGHT);
    float fovy = 2.0f * glm::atan(float(PORT_NATIVE_HEIGHT) * 0.5f,
                                  float(g_h));
    g_proj = glm::perspective(fovy, aspect, 1.0f, 100000.0f);
    (void)g_center_x;
    (void)g_center_y;
}

} /* namespace */

extern "C" void HAL_GTE_SetScreenCenter(int x, int y) {
    g_center_x = float(x);
    g_center_y = float(y);
    RebuildProjection();
}

extern "C" void HAL_GTE_SetProjection(int h) {
    g_h = h;
    RebuildProjection();
}

extern "C" void HAL_GTE_LoadMatrix(const int16_t matrix[9]) {
    /* PS1 stores the rotation matrix row-major as int16 with 4096 = 1.0.
     * GLM is column-major so we transpose during conversion. */
    glm::mat3 r;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            r[col][row] = float(matrix[row * 3 + col]) / PS1_FIXED_POINT;
        }
    }
    g_view = glm::mat4(r);
}

extern "C" glm::vec4 HAL_GTE_Project(const glm::vec3 &v) {
    return g_proj * g_view * glm::vec4(v, 1.0f);
}

extern "C" const glm::mat4 &HAL_GTE_GetView(void)       { return g_view; }
extern "C" const glm::mat4 &HAL_GTE_GetProjection(void) { return g_proj; }
