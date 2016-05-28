//
// Created by kelsier on 5/27/16.
//

#ifndef FS_BUFFERMANAGER_H
#define FS_BUFFERMANAGER_H

#include <list>
#include "BlockData.h"

struct BufferManager {
    BufferManager(unsigned int numberOfBlocks, double fOld, double fNew);
    BlockData *getBlock(const char *name, unsigned int block);
    void insertBlock(BlockData *b);
    void renameFile(char *name, char *newName);
    std::list<BlockData*> newData;
    std::list<BlockData*> midData;
    std::list<BlockData*> oldData;
    unsigned int newMaxSize, midMaxSize, oldMaxSize;
    unsigned int blockSize;
};


#endif //FS_BUFFERMANAGER_H
