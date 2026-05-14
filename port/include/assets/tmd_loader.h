#ifndef ASSETS_TMD_LOADER_H
#define ASSETS_TMD_LOADER_H

#include <glad/glad.h>
#include <cstddef>
#include <cstdint>
#include <vector>

struct TMDVertex   { float x, y, z; };
struct TMDNormal   { float x, y, z; };
struct TMDPrimitive {
    uint8_t  r, g, b;
    uint16_t v0, v1, v2;
    bool     is_quad;
    uint16_t v3;
};

struct TMDObject {
    std::vector<TMDVertex>    vertices;
    std::vector<TMDNormal>    normals;
    std::vector<TMDPrimitive> primitives;
    GLuint vao, vbo, ebo;
    int    index_count;
};

struct TMDModel {
    std::vector<TMDObject> objects;
    bool valid;
};

TMDModel TMD_Load(const uint8_t* data, size_t size);
void     TMD_Upload(TMDModel& model);
void     TMD_Draw(const TMDModel& model);
void     TMD_Free(TMDModel& model);

#endif /* ASSETS_TMD_LOADER_H */
