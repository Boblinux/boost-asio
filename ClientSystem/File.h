#ifndef MD_fILE_H
#define MD_fILE_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include "MD5f.h"
#include "BlockFile.h"

const int PAGE_SIZE = 4096*256;

namespace Md
{

class BlockFile;
class Md5;
class File
{

public:

File(int fd, int conn, ll fileSize) 
    : fd_(fd),
      conn_(conn),
      fileSize_(fileSize),
      md5_(fd_, fileSize_, 0),
      blockNum_((ll)(fileSize_ % PAGE_SIZE == 0 ? fileSize_/PAGE_SIZE:fileSize_/PAGE_SIZE+1)),
      uploadId_(std::vector<bool>(blockNum_, false)),
      downloadId_(std::vector<bool>(blockNum_, false))
{
    initFile();
}
~File() 
{
    BlockFileList_.clear();
}
    
std::string getFileMD5()
{
    return md5_.getMD5();
}

unsigned long getFileBlockNum() {return blockNum_; }

std::vector<BlockFile*> getFileBlockFile() {return BlockFileList_; }

void initFile()
{
    for(int i=0; i<blockNum_; i++)
    {
        if(i != blockNum_-1)
        {
            BlockFile *block = new BlockFile(fd_, conn_, PAGE_SIZE, i, i*PAGE_SIZE);
            BlockFileList_.push_back(block);
        }
        else 
        {
            BlockFile *block = new BlockFile(fd_, conn_, fileSize_%PAGE_SIZE, i, i*PAGE_SIZE);
            BlockFileList_.push_back(block);
        }
    }
}

void sendFileinfo(std::string fileName)
{
    fileName += md5_.getMD5();
    int fileNameMd5Len = fileName.size();
    uint32_t be = htonl(fileNameMd5Len);
    uint32_t nbe = htonl(blockNum_);
    std::string sendstr = "";
    sendstr.append(static_cast<const char*>(static_cast<const void*>(&be)), 4);
    sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), 4);
    sendstr.append(fileName.data(), fileNameMd5Len);
    ::send(conn_, sendstr.data(), sendstr.size(), 0);
}

void sendFileBlockMD5info()
{
    std::string sendmd5 = "";
    for(int i=0; i<blockNum_; i++)
    {
        sendmd5 += BlockFileList_[i]->getBlockFileMD5();
    }
    ::send(conn_, sendmd5.data(), sendmd5.size(), 0);
}

void sendFileBlock()
{
    for(int i=0; i<blockNum_; i++)
    {
        if(uploadId_[i] == false)
        {
            BlockFileList_[i]->sendBlockFile();
        }
        printf("\rsend block[%.2lf%%]:", (i*100.0) / blockNum_);
    }
    printf("\rsend block[%.2lf%%]:", 100.0);
}

void updateuploadId(std::string idstr)
{
    std::string tmp;
    int start = 0, end = 0;
    for(int i=0; i<(int)idstr.size(); i++)
    {
        if(idstr[i] == ':')
        {
            tmp = idstr.substr(start, i-start);
            uploadId_[atoi(tmp.data())] = true;
            start = i+1;
        }
    }
}

void updatedownloadId(std::string idstr)
{
    std::string tmp;
    int start = 0, end = 0;
    for(int i=0; i<(int)idstr.size(); i++)
    {
        if(idstr[i] == ':')
        {
            tmp = idstr.substr(start, i-start);
            downloadId_[atoi(tmp.data())] = true;
            start = i+1;
        }
    }
}


private:
    int                         fd_; 
    int                         conn_; 
    ll                          fileSize_;
    Md5                         md5_; 
    unsigned long               blockNum_;
    std::vector<BlockFile*>     BlockFileList_;
    std::vector<bool>           uploadId_; 
    std::vector<bool>           downloadId_;  
};

}; // namespace MD5

#endif