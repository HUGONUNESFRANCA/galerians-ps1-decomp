#ifndef ASSETS_TIM_LOADER_H
#define ASSETS_TIM_LOADER_H

#include <glad/glad.h>
#include <cstddef>
#include <cstdint>

struct TIMImage {
    GLuint texture_id;
    int width;
    int height;
};

TIMImage TIM_Load(const uint8_t* data, size_t size);
void TIM_Free(TIMImage& img);

#endif /* ASSETS_TIM_LOADER_H */
