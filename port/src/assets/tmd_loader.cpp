/*
 * Assets — TMD Loader
 *
 * PS1 TMD (Three-dimensional Model Data) format.
 * Handles both absolute-pointer (flags=0) and relative-pointer (flags=1) TMDs.
 * Supported primitive modes: 0x20 (flat tri), 0x21 (gouraud tri),
 *                            0x28 (flat quad), 0x29 (gouraud quad).
 * Textured primitives (0x24, 0x25, 0x2C, 0x2D) are skipped.
 */

#include "assets/tmd_loader.h"

#include <cstdio>
#include <cstring>

namespace {

inline uint32_t R32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

inline int16_t R16s(const uint8_t* p) {
    return int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
}

inline uint16_t R16u(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

/* Sizes of each primitive packet in bytes (header + payload).
 * Code Rle: mode, flag, ilen, olen then payload.
 * We read mode from offset +3 of the packet.
 *
 * 0x20: 1 packet word header + RGB(4) + 3×v(2) + pad(2) = 4+4+8 = 16 bytes total
 * 0x21: 1 header + 3×RGB(4) + 3×v(2)+pad(2) = 4+12+8 = 24 bytes total
 * 0x28: 1 header + RGB(4) + 4×v(2) = 4+4+10 = 18 but must be word-aligned -> 20
 * 0x29: 1 header + 4×RGB(4) + 4×v(2) = 4+16+10 = 30 -> 32
 *
 * Actual packet layout per PsyQ TMD spec (all values in bytes):
 *   Each primitive starts with a 4-byte header: r,g,b,mode | flag,ilen,olen,0
 *   Then the payload.
 */
struct PrimSize { int total; bool is_quad; };

PrimSize prim_packet_size(uint8_t mode) {
    switch (mode) {
        case 0x20: return {16, false}; /* flat  tri: 4hdr + 4rgb + 6vi + 2pad */
        case 0x21: return {28, false}; /* gour  tri: 4hdr + 12rgb + 6vi + 2pad => 24? */
        case 0x28: return {20, true};  /* flat  quad: 4hdr + 4rgb + 8vi + 4pad */
        case 0x29: return {36, true};  /* gour  quad: 4hdr + 16rgb + 8vi + 4pad => 32? */
        default:   return {0,  false};
    }
}

/*
 * Parse primitives from a raw primitive block.
 * Each packet:
 *   byte 0: r (or r0 for gouraud)
 *   byte 1: g
 *   byte 2: b
 *   byte 3: mode
 *   byte 4: flag
 *   byte 5: ilen  (input packet length in 32-bit words, includes header word)
 *   byte 6: olen  (output packet length in 32-bit words)
 *   byte 7: pad
 *   ... payload follows
 *
 * We use ilen to advance through the block (ilen * 4 bytes per packet).
 */
std::vector<TMDPrimitive> ParsePrimitives(const uint8_t* base,
                                          uint32_t       prim_top,
                                          uint32_t       prim_n,
                                          bool           relative,
                                          const uint8_t* data_start,
                                          size_t         data_size) {
    std::vector<TMDPrimitive> out;
    out.reserve(prim_n);

    const uint8_t* pptr;
    if (relative) {
        /* prim_top is relative to the address of the prim_top field itself.
         * In the object descriptor at offset od, prim_top is at +16 from od. */
        pptr = base + prim_top;
    } else {
        if (prim_top >= data_size) return out;
        pptr = data_start + prim_top;
    }

    for (uint32_t i = 0; i < prim_n; ++i) {
        if (pptr + 8 > data_start + data_size) break;

        uint8_t mode = pptr[3];
        uint8_t ilen = pptr[5]; /* packet length in 32-bit words */
        int pkt_bytes = int(ilen) * 4;

        if (pkt_bytes < 8 || pptr + pkt_bytes > data_start + data_size) break;

        switch (mode) {
            case 0x20: { /* flat triangle: RGB + v0,v1,v2 */
                if (pkt_bytes < 16) { pptr += pkt_bytes; continue; }
                TMDPrimitive p{};
                p.r = pptr[0]; p.g = pptr[1]; p.b = pptr[2];
                p.v0 = R16u(pptr + 8);
                p.v1 = R16u(pptr + 10);
                p.v2 = R16u(pptr + 12);
                p.is_quad = false;
                out.push_back(p);
                break;
            }
            case 0x21: { /* gouraud triangle: RGB0,RGB1,RGB2 + v0,v1,v2 */
                if (pkt_bytes < 24) { pptr += pkt_bytes; continue; }
                TMDPrimitive p{};
                /* Average the three vertex colours */
                p.r = uint8_t((int(pptr[0]) + pptr[4] + pptr[8])  / 3);
                p.g = uint8_t((int(pptr[1]) + pptr[5] + pptr[9])  / 3);
                p.b = uint8_t((int(pptr[2]) + pptr[6] + pptr[10]) / 3);
                p.v0 = R16u(pptr + 12);
                p.v1 = R16u(pptr + 14);
                p.v2 = R16u(pptr + 16);
                p.is_quad = false;
                out.push_back(p);
                break;
            }
            case 0x28: { /* flat quad: RGB + v0,v1,v2,v3 */
                if (pkt_bytes < 20) { pptr += pkt_bytes; continue; }
                TMDPrimitive p{};
                p.r = pptr[0]; p.g = pptr[1]; p.b = pptr[2];
                p.v0 = R16u(pptr + 8);
                p.v1 = R16u(pptr + 10);
                p.v2 = R16u(pptr + 12);
                p.v3 = R16u(pptr + 14);
                p.is_quad = true;
                out.push_back(p);
                break;
            }
            case 0x29: { /* gouraud quad: RGB0..3 + v0,v1,v2,v3 */
                if (pkt_bytes < 32) { pptr += pkt_bytes; continue; }
                TMDPrimitive p{};
                p.r = uint8_t((int(pptr[0]) + pptr[4] + pptr[8]  + pptr[12]) / 4);
                p.g = uint8_t((int(pptr[1]) + pptr[5] + pptr[9]  + pptr[13]) / 4);
                p.b = uint8_t((int(pptr[2]) + pptr[6] + pptr[10] + pptr[14]) / 4);
                p.v0 = R16u(pptr + 16);
                p.v1 = R16u(pptr + 18);
                p.v2 = R16u(pptr + 20);
                p.v3 = R16u(pptr + 22);
                p.is_quad = true;
                out.push_back(p);
                break;
            }
            default:
                break; /* skip textured or unknown primitives */
        }
        pptr += pkt_bytes;
    }
    return out;
}

} /* namespace */

TMDModel TMD_Load(const uint8_t* data, size_t size) {
    TMDModel model{};
    model.valid = false;

    if (size < 12) return model;

    uint32_t magic = R32(data);
    uint32_t flags = R32(data + 4);
    uint32_t nobj  = R32(data + 8);

    if (magic != 0x41000000) return model;
    if (flags > 1 || nobj < 1 || nobj > 256) return model;

    bool relative = (flags == 1);
    const uint8_t* obj_table = data + 12;

    model.objects.reserve(nobj);

    for (uint32_t i = 0; i < nobj; ++i) {
        size_t od = size_t(i) * 28;
        if (od + 28 > size - 12) break;
        const uint8_t* op = obj_table + od;

        uint32_t vert_top = R32(op);
        uint32_t vert_n   = R32(op + 4);
        uint32_t norm_top = R32(op + 8);
        uint32_t norm_n   = R32(op + 12);
        uint32_t prim_top = R32(op + 16);
        uint32_t prim_n   = R32(op + 20);

        if (vert_n < 1 || vert_n > 65535) continue;
        if (prim_n < 1 || prim_n > 65535) continue;

        /* Resolve vertex base pointer */
        const uint8_t* vbase;
        const uint8_t* nbase;
        if (relative) {
            /* Pointers are relative to the address of the field itself */
            vbase = (op)      + vert_top; /* vert_top is at op+0 */
            nbase = (op + 8)  + norm_top; /* norm_top is at op+8 */
        } else {
            if (vert_top >= size) continue;
            vbase = data + vert_top;
            nbase = (norm_top < size) ? data + norm_top : nullptr;
        }

        /* Bounds check vertices */
        size_t vert_bytes = size_t(vert_n) * 8; /* int16[4] per vertex */
        if (vbase + vert_bytes > data + size) continue;

        TMDObject obj{};
        obj.vao = obj.vbo = obj.ebo = 0;
        obj.index_count = 0;

        /* Parse vertices: int16 x,y,z,pad → float / 1000 */
        obj.vertices.reserve(vert_n);
        for (uint32_t v = 0; v < vert_n; ++v) {
            const uint8_t* vp = vbase + v * 8;
            float x = float(R16s(vp))     / 1000.0f;
            float y = float(R16s(vp + 2)) / 1000.0f;
            float z = float(R16s(vp + 4)) / 1000.0f;
            obj.vertices.push_back({x, y, z});
        }

        /* Parse normals: int16 nx,ny,nz,pad → float / 4096 */
        if (nbase && norm_n > 0) {
            size_t norm_bytes = size_t(norm_n) * 8;
            if (nbase + norm_bytes <= data + size) {
                obj.normals.reserve(norm_n);
                for (uint32_t n = 0; n < norm_n; ++n) {
                    const uint8_t* np = nbase + n * 8;
                    float nx = float(R16s(np))     / 4096.0f;
                    float ny = float(R16s(np + 2)) / 4096.0f;
                    float nz = float(R16s(np + 4)) / 4096.0f;
                    obj.normals.push_back({nx, ny, nz});
                }
            }
        }

        /* Parse primitives */
        const uint8_t* pbase = relative ? (op + 16) : data; /* op+16 = address of prim_top field */
        obj.primitives = ParsePrimitives(pbase, prim_top, prim_n,
                                         relative, data, size);

        model.objects.push_back(std::move(obj));
    }

    model.valid = !model.objects.empty();
    return model;
}

void TMD_Upload(TMDModel& model) {
    /* Interleaved VBO layout: vec3 position + vec3 color (6 floats, 24 bytes) */
    for (auto& obj : model.objects) {
        if (obj.primitives.empty() || obj.vertices.empty()) continue;

        /* Build an expanded triangle list (no index sharing — colour is per-prim) */
        struct Vtx { float x, y, z, r, g, b; };
        std::vector<Vtx> verts;
        std::vector<uint32_t> indices;
        verts.reserve(obj.primitives.size() * 6);
        indices.reserve(obj.primitives.size() * 6);

        auto push_tri = [&](const TMDPrimitive& p,
                            uint16_t i0, uint16_t i1, uint16_t i2) {
            float fr = p.r / 255.0f;
            float fg = p.g / 255.0f;
            float fb = p.b / 255.0f;
            auto safe = [&](uint16_t idx) -> const TMDVertex& {
                static const TMDVertex zero{0,0,0};
                return idx < obj.vertices.size() ? obj.vertices[idx] : zero;
            };
            uint32_t base = uint32_t(verts.size());
            const TMDVertex& va = safe(i0);
            const TMDVertex& vb = safe(i1);
            const TMDVertex& vc = safe(i2);
            verts.push_back({va.x, va.y, va.z, fr, fg, fb});
            verts.push_back({vb.x, vb.y, vb.z, fr, fg, fb});
            verts.push_back({vc.x, vc.y, vc.z, fr, fg, fb});
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
        };

        for (const auto& p : obj.primitives) {
            push_tri(p, p.v0, p.v1, p.v2);
            if (p.is_quad) push_tri(p, p.v1, p.v3, p.v2);
        }

        if (verts.empty()) continue;

        glGenVertexArrays(1, &obj.vao);
        glGenBuffers(1, &obj.vbo);
        glGenBuffers(1, &obj.ebo);

        glBindVertexArray(obj.vao);

        glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     GLsizeiptr(verts.size() * sizeof(Vtx)),
                     verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     GLsizeiptr(indices.size() * sizeof(uint32_t)),
                     indices.data(), GL_STATIC_DRAW);

        /* location 0: position */
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx),
                              reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);
        /* location 1: color */
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx),
                              reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        obj.index_count = int(indices.size());
    }
}

void TMD_Draw(const TMDModel& model) {
    for (const auto& obj : model.objects) {
        if (obj.vao == 0 || obj.index_count == 0) continue;
        glBindVertexArray(obj.vao);
        glDrawElements(GL_TRIANGLES, obj.index_count, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void TMD_Free(TMDModel& model) {
    for (auto& obj : model.objects) {
        if (obj.vao) { glDeleteVertexArrays(1, &obj.vao); obj.vao = 0; }
        if (obj.vbo) { glDeleteBuffers(1, &obj.vbo);      obj.vbo = 0; }
        if (obj.ebo) { glDeleteBuffers(1, &obj.ebo);      obj.ebo = 0; }
        obj.index_count = 0;
    }
    model.objects.clear();
    model.valid = false;
}
