/*
 * Assets — TIM Loader
 *
 * Decodes PS1 TIM textures (4/8/16/24 bpp, optional CLUT) into RGBA8
 * and uploads them as GL_TEXTURE_2D. See docs/asset_format.md.
 */

#include "assets/tim_loader.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr uint32_t kTimMagic = 0x10;

constexpr uint32_t kPmodeMask = 0x07;
constexpr uint32_t kClutFlag  = 0x08;

constexpr uint32_t kPmode4bpp  = 0;
constexpr uint32_t kPmode8bpp  = 1;
constexpr uint32_t kPmode16bpp = 2;
constexpr uint32_t kPmode24bpp = 3;

inline uint32_t Read32LE(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline uint16_t Read16LE(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

inline void Decode5551(uint16_t pixel, uint8_t* out_rgba) {
    uint8_t r = uint8_t((pixel & 0x001F) << 3);
    uint8_t g = uint8_t(((pixel >> 5) & 0x001F) << 3);
    uint8_t b = uint8_t(((pixel >> 10) & 0x001F) << 3);
    bool stp = (pixel & 0x8000) != 0;

    /* PS1 convention: pixel value 0x0000 is fully transparent.
     * Otherwise, STP=1 indicates semi-transparent (blended), but for
     * a simple textured quad we treat all non-zero pixels as opaque. */
    uint8_t a = (pixel == 0) ? 0 : 255;
    (void)stp;

    out_rgba[0] = r;
    out_rgba[1] = g;
    out_rgba[2] = b;
    out_rgba[3] = a;
}

GLuint UploadRGBA(const uint8_t* rgba, int w, int h) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

} /* namespace */

TIMImage TIM_Load(const uint8_t* data, size_t size) {
    TIMImage out{0, 0, 0};

    if (!data || size < 8) {
        std::fprintf(stderr, "TIM_Load: buffer too small\n");
        return out;
    }

    uint32_t magic = Read32LE(data);
    if (magic != kTimMagic) {
        std::fprintf(stderr, "TIM_Load: bad magic 0x%08X\n", magic);
        return out;
    }

    uint32_t flags = Read32LE(data + 4);
    uint32_t pmode = flags & kPmodeMask;
    bool has_clut = (flags & kClutFlag) != 0;

    size_t cursor = 8;

    /* CLUT block. */
    std::vector<uint16_t> clut;
    if (has_clut) {
        if (cursor + 12 > size) {
            std::fprintf(stderr, "TIM_Load: truncated CLUT header\n");
            return out;
        }
        uint32_t clut_size = Read32LE(data + cursor);
        uint16_t clut_w = Read16LE(data + cursor + 8);
        uint16_t clut_h = Read16LE(data + cursor + 10);
        size_t entries = size_t(clut_w) * size_t(clut_h);
        if (cursor + clut_size > size || clut_size < 12 ||
            entries * 2 + 12 > clut_size) {
            std::fprintf(stderr, "TIM_Load: CLUT bounds error\n");
            return out;
        }
        clut.resize(entries);
        std::memcpy(clut.data(), data + cursor + 12, entries * 2);
        cursor += clut_size;
    }

    /* Image block. */
    if (cursor + 12 > size) {
        std::fprintf(stderr, "TIM_Load: truncated image header\n");
        return out;
    }
    uint32_t image_size = Read32LE(data + cursor);
    uint16_t img_w_units = Read16LE(data + cursor + 8);
    uint16_t img_h = Read16LE(data + cursor + 10);
    const uint8_t* pixels = data + cursor + 12;
    /* Some extracted TIM slices are truncated — instead of bailing when
     * the declared image block runs past the file, work with whatever
     * pixel bytes are actually present and clamp the height per pmode. */
    size_t pixels_bytes = (cursor + 12 < size) ? (size - cursor - 12) : 0;
    if (image_size >= 12) {
        size_t declared = image_size - 12;
        if (declared < pixels_bytes) pixels_bytes = declared;
    }

    int width = 0, height = int(img_h);
    std::vector<uint8_t> rgba;

    /* Debug: surface the header values so truncated/odd TIMs can be
     * diagnosed without a hex editor. */
    std::printf("  TIM: pmode=%ubpp clut=%d w_units=%u h=%u "
                "file_size=%zu pixel_bytes=%zu\n",
                pmode == 0 ? 4 : pmode == 1 ? 8 : pmode == 2 ? 16 : 24,
                int(has_clut), unsigned(img_w_units), unsigned(img_h),
                size, pixels_bytes);

    switch (pmode) {
    case kPmode4bpp: {
        if (clut.empty()) {
            /* Fall back to a 16-entry grayscale ramp so slices stripped
             * of their palette still render as a visible image. */
            std::fprintf(stderr,
                         "TIM_Load: 4bpp without CLUT, using grayscale\n");
            clut.resize(16);
            for (int i = 0; i < 16; ++i) {
                uint8_t v5 = uint8_t((i * 17) >> 3); /* fit into 5 bits */
                uint16_t gray = uint16_t(v5) |
                                (uint16_t(v5) << 5) |
                                (uint16_t(v5) << 10);
                clut[i] = gray ? gray : 0x0421; /* avoid all-zero = transparent */
            }
        }
        width = img_w_units * 4;
        if (width <= 0) return out;
        size_t row_bytes = size_t(width) / 2;
        int max_rows = row_bytes ? int(pixels_bytes / row_bytes) : 0;
        if (max_rows < height) height = max_rows;
        if (height <= 0) {
            std::fprintf(stderr, "TIM_Load: 4bpp no rows available\n");
            return out;
        }
        rgba.resize(size_t(width) * size_t(height) * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; x += 2) {
                uint8_t byte = pixels[(y * width + x) / 2];
                uint8_t idx0 = byte & 0x0F;
                uint8_t idx1 = (byte >> 4) & 0x0F;
                uint16_t c0 = idx0 < clut.size() ? clut[idx0] : 0;
                uint16_t c1 = idx1 < clut.size() ? clut[idx1] : 0;
                Decode5551(c0, &rgba[(y * width + x) * 4]);
                Decode5551(c1, &rgba[(y * width + x + 1) * 4]);
            }
        }
        break;
    }
    case kPmode8bpp: {
        if (clut.empty()) {
            std::fprintf(stderr,
                         "TIM_Load: 8bpp without CLUT, using grayscale\n");
            clut.resize(256);
            for (int i = 0; i < 256; ++i) {
                uint8_t v5 = uint8_t(i >> 3);
                uint16_t gray = uint16_t(v5) |
                                (uint16_t(v5) << 5) |
                                (uint16_t(v5) << 10);
                clut[i] = gray ? gray : 0x0421;
            }
        }
        width = img_w_units * 2;
        if (width <= 0) return out;
        size_t row_bytes = size_t(width);
        int max_rows = row_bytes ? int(pixels_bytes / row_bytes) : 0;
        if (max_rows < height) height = max_rows;
        if (height <= 0) {
            std::fprintf(stderr, "TIM_Load: 8bpp no rows available\n");
            return out;
        }
        rgba.resize(size_t(width) * size_t(height) * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint8_t idx = pixels[y * width + x];
                uint16_t c = idx < clut.size() ? clut[idx] : 0;
                Decode5551(c, &rgba[(y * width + x) * 4]);
            }
        }
        break;
    }
    case kPmode16bpp: {
        width = img_w_units;
        if (width <= 0) return out;
        size_t row_bytes = size_t(width) * 2;
        int max_rows = row_bytes ? int(pixels_bytes / row_bytes) : 0;
        if (max_rows < height) height = max_rows;
        if (height <= 0) {
            std::fprintf(stderr, "TIM_Load: 16bpp no rows available\n");
            return out;
        }
        rgba.resize(size_t(width) * size_t(height) * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint16_t c = Read16LE(pixels + (y * width + x) * 2);
                Decode5551(c, &rgba[(y * width + x) * 4]);
            }
        }
        break;
    }
    case kPmode24bpp: {
        width = (img_w_units * 2) / 3;
        if (width <= 0) return out;
        size_t row_bytes = size_t(width) * 3;
        int max_rows = row_bytes ? int(pixels_bytes / row_bytes) : 0;
        if (max_rows < height) height = max_rows;
        if (height <= 0) {
            std::fprintf(stderr, "TIM_Load: 24bpp no rows available\n");
            return out;
        }
        rgba.resize(size_t(width) * size_t(height) * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const uint8_t* p = pixels + (y * width + x) * 3;
                size_t o = (y * width + x) * 4;
                rgba[o + 0] = p[0];
                rgba[o + 1] = p[1];
                rgba[o + 2] = p[2];
                rgba[o + 3] = 255;
            }
        }
        break;
    }
    default:
        std::fprintf(stderr, "TIM_Load: unknown pmode %u\n", pmode);
        return out;
    }

    out.texture_id = UploadRGBA(rgba.data(), width, height);
    out.width = width;
    out.height = height;
    return out;
}

void TIM_Free(TIMImage& img) {
    if (img.texture_id) {
        glDeleteTextures(1, &img.texture_id);
    }
    img.texture_id = 0;
    img.width = 0;
    img.height = 0;
}
