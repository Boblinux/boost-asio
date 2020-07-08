#ifndef MD_BLOCKFILE_H
#define MD_BLOCKFILE_H

#include <stdio.h>
#include <string>
#include <arpa/inet.h>
#include "MD5f.h"

namespace Md
{

class BlockFile
{

public:

BlockFile(int fd, int conn, ll blockSize, int id, off_t offset) 
    : fd_(fd),
      conn_(conn),
      blockSize_(blockSize),
      id_(id),
      offset_(offset),
      md5_(fd_, blockSize_, offset_)
{
    
}
~BlockFile() {}
    
ll getBlockSize()
{
    return blockSize_;
}

std::string getBlockFileMD5()
{
    return md5_.getMD5();
}

int getBlockFileid()
{
    return id_;
}

off_t getBlockFileOffset()
{
    return offset_;
}

void sendBlockFile()
{
    char *bockBuffer = (char*)mmap(0, blockSize_, PROT_READ, MAP_SHARED, fd_, offset_);
    uint32_t bsbe = htonl(blockSize_);
    uint32_t idbe = htonl(id_); //send id
    std::string sendstr = "";
    sendstr.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), 4);
    sendstr.append(static_cast<const char*>(static_cast<const void*>(&idbe)), 4);
    sendstr.append(bockBuffer, blockSize_);
    ::send(conn_, sendstr.data(), sendstr.size(), 0);
    munmap(bockBuffer, blockSize_); 
}

private:
    int             fd_;
    int             conn_; 
    ll              blockSize_;
    uint32_t        id_;
    off_t           offset_;
    Md5             md5_;
};

}; // namespace MD5

#endif