//
// Created by kelsier on 5/27/16.
//

#include <string.h>
#include "BufferManager.h"

static void renameFileInList(std::list<BlockData*> *l, char *name, char *newName);
static void removeMaxFromRight(std::list<BlockData*> *l);

BufferManager::BufferManager(unsigned int numberOfBlocks, double fOld,
                             double fNew)
{
    newMaxSize = numberOfBlocks / fNew;
    oldMaxSize = numberOfBlocks / fOld;
    midMaxSize = numberOfBlocks - newMaxSize - oldMaxSize;
}

BlockData *BufferManager::getBlock(const char *name, unsigned int block)
{
    for (auto it = newData.begin(); it != newData.end(); it++)
    {
        if ((*it)->fileName == std::string(name) && (*it)->bIndex == block)
        {
            BlockData *b = *it;
            newData.erase(it);
            newData.push_front(b);
            return b;
        }
    }
    for (auto it = midData.begin(); it != midData.end(); it++)
    {
        if ((*it)->fileName == std::string(name) && (*it)->bIndex == block)
        {
            BlockData *b = *it;
            midData.erase(it);
            midData.push_front(newData.back());
            newData.pop_back();
            newData.push_front(b);
            b->numUsed++;
            return b;
        }
    }
    for (auto it = oldData.begin(); it != oldData.end(); it++)
    {
        if ((*it)->fileName == std::string(name) && (*it)->bIndex == block)
        {
            BlockData *b = *it;
            oldData.erase(it);
            oldData.push_front(midData.back());
            midData.pop_back();
            midData.push_front(newData.back());
            newData.pop_back();
            newData.push_front(b);
            b->numUsed++;
            return b;
        }
    }
    return nullptr;
}

void BufferManager::insertBlock(BlockData *b)
{
    newData.push_front(b);
    if(newData.size() > newMaxSize)
    {
        midData.push_front(newData.back());
        newData.pop_back();
        if(midData.size() > midMaxSize)
        {
            if(oldData.size() > oldMaxSize - 1)
            {
                removeMaxFromRight(&oldData);
            }
            oldData.push_front(midData.back());
            midData.pop_back();
        }
    }
}

void BufferManager::renameFile(char *name, char *newName)
{
    renameFileInList(&newData, name, newName);
    renameFileInList(&midData, name, newName);
    renameFileInList(&oldData, name, newName);
}

static void renameFileInList(std::list<BlockData*> *l, char *name, char *newName)
{
    std::string oldName = std::string(name);
    for(auto b : *l)
    {
        if(b->fileName == oldName)
        {
            b->rename(newName);
        }
    }
}

static void removeMaxFromRight(std::list<BlockData*> *l)
{
    auto maxIt = l->begin();
    auto it = l->begin();
    while(++it != l->end())
    {
        if(*it >= *maxIt)
        {
            maxIt = it;
        }
    }
    l->erase(maxIt);
}