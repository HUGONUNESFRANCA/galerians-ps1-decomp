#ifndef ASSETS_CDB_LOADER_H
#define ASSETS_CDB_LOADER_H

#include <cstdint>
#include <string>
#include <vector>

struct CDBAsset {
    std::vector<uint8_t> data;
    std::string type;
};

std::vector<CDBAsset> CDB_Load(const std::string& path);

#endif /* ASSETS_CDB_LOADER_H */
