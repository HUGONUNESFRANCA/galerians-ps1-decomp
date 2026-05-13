/*
 * Assets — CDB Loader
 *
 * CDB header (8 bytes):
 *   u32 sector_count
 *   u32 compression_flag (non-zero = LZSS)
 * Payload starts at offset 0x08. If compressed, decompress using the
 * PS1 LZSS variant (4 KB sliding window, 18-byte lookahead, MSB-first
 * flag bits where 1=literal, 0=window copy). After decompression,
 * scan for sub-file magic words:
 *   0x10000000 -> TIM texture
 *   0x41000000 -> TMD model
 */

#include "assets/cdb_loader.h"

#include <cstdio>
#include <cstring>
#include <fstream>

namespace {

constexpr size_t kLzssWindow    = 0x1000;
constexpr size_t kLzssLookahead = 0x12;

inline uint32_t Read32LE(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

std::vector<uint8_t> LzssDecompress(const uint8_t* src, size_t src_size) {
    std::vector<uint8_t> out;
    out.reserve(src_size * 4);

    uint8_t window[kLzssWindow];
    std::memset(window, 0, sizeof(window));
    /* Standard PS1 LZSS initializes the write cursor so that the first
     * back-reference reads from freshly written bytes. */
    size_t w_pos = kLzssWindow - kLzssLookahead;

    size_t s_pos = 0;
    while (s_pos < src_size) {
        uint8_t flags = src[s_pos++];
        for (int bit = 0; bit < 8 && s_pos < src_size; ++bit) {
            bool is_literal = (flags & 0x80) != 0;
            flags = uint8_t(flags << 1);

            if (is_literal) {
                uint8_t b = src[s_pos++];
                out.push_back(b);
                window[w_pos] = b;
                w_pos = (w_pos + 1) & (kLzssWindow - 1);
            } else {
                if (s_pos + 2 > src_size) return out;
                uint8_t b0 = src[s_pos++];
                uint8_t b1 = src[s_pos++];
                /* Offset = low 12 bits of (b0 | (b1<<8)) where the
                 * upper 4 bits of b1 hold (length - 3). */
                size_t offset = size_t(b0) | (size_t(b1 & 0x0F) << 8);
                size_t length = size_t((b1 >> 4) & 0x0F) + 3;
                for (size_t i = 0; i < length; ++i) {
                    uint8_t b = window[(offset + i) & (kLzssWindow - 1)];
                    out.push_back(b);
                    window[w_pos] = b;
                    w_pos = (w_pos + 1) & (kLzssWindow - 1);
                }
            }
        }
    }
    return out;
}

void ScanSubfiles(const std::vector<uint8_t>& blob,
                  std::vector<CDBAsset>& out) {
    if (blob.size() < 4) return;

    struct Hit { size_t off; const char* type; };
    std::vector<Hit> hits;

    for (size_t i = 0; i + 4 <= blob.size(); ++i) {
        uint32_t m = Read32LE(&blob[i]);
        if (m == 0x10000000) hits.push_back({i, "TIM"});
        else if (m == 0x41000000) hits.push_back({i, "TMD"});
    }

    for (size_t k = 0; k < hits.size(); ++k) {
        size_t start = hits[k].off;
        size_t end = (k + 1 < hits.size()) ? hits[k + 1].off : blob.size();
        CDBAsset a;
        a.type = hits[k].type;
        a.data.assign(blob.begin() + start, blob.begin() + end);
        out.push_back(std::move(a));
    }
}

} /* namespace */

std::vector<CDBAsset> CDB_Load(const std::string& path) {
    std::vector<CDBAsset> result;

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        std::fprintf(stderr, "CDB_Load: cannot open %s\n", path.c_str());
        return result;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    if (size < 8) {
        std::fprintf(stderr, "CDB_Load: file too small (%lld bytes)\n",
                     static_cast<long long>(size));
        return result;
    }

    std::vector<uint8_t> raw(static_cast<size_t>(size));
    if (!f.read(reinterpret_cast<char*>(raw.data()), size)) {
        std::fprintf(stderr, "CDB_Load: read failed for %s\n", path.c_str());
        return result;
    }

    uint32_t compression_flag = Read32LE(&raw[4]);
    const uint8_t* payload = raw.data() + 8;
    size_t payload_size = raw.size() - 8;

    std::vector<uint8_t> blob;
    if (compression_flag != 0) {
        blob = LzssDecompress(payload, payload_size);
    } else {
        blob.assign(payload, payload + payload_size);
    }

    ScanSubfiles(blob, result);
    return result;
}
