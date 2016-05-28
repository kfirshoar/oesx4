//
// Created by kelsier on 5/27/16.
//

#ifndef FS_BLOCKDATA_H
#define FS_BLOCKDATA_H

#include <string>

struct BlockData {
    BlockData(unsigned int blockIndex, size_t blockSize, std::string name, char *d)
            : bIndex(blockIndex), numUsed(1), fileName(name), bSize(blockSize), data(d)
    {
    }

    void rename(char *name) { fileName = std::string(name); }

    std::string fileName;
    unsigned int bIndex;
    unsigned int numUsed;
    size_t bSize;
    char *data;
};

inline bool operator<(const BlockData &lhs, const BlockData &rhs)
{ return lhs.numUsed < rhs.numUsed; }

#endif //FS_BLOCKDATA_H
