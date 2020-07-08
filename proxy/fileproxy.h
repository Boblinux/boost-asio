#ifndef MD_Proxy_fILE_H
#define MD_Proxy_fILE_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>

namespace MINI
{

class FileValue
{

public:

FileValue(unsigned long blockNum, std::string md5)
: blockNum_(blockNum),
  fileMD5_(md5)
{
    BlockFileMD5_ = std::vector<std::string>(blockNum);
    uploadId_ = std::vector<bool>(blockNum, false);
    downloadId_ = std::vector<bool>(blockNum, false);
    BlockFile_ = std::vector<std::string>(blockNum);
}

void setFileMD5(std::string md5)
{
    fileMD5_ = md5;
}

void setBlockFileMD5(std::string md5)
{
    for(int i=0; i<(int)blockNum_; i++)
    {
        BlockFileMD5_[i] = md5.substr(i*32,32);
    }
}

void setuploadId(int id)
{
    uploadId_[id] = true;
}

void setdownloadId(int id)
{
    downloadId_[id] = true;
}

std::string getuploadId()
{
    std::string idstr;
    for(int i=0; i<(int)blockNum_; i++)
    {
      if(uploadId_[i] == true) //false->true
      {
        idstr += std::to_string(i) + ':';
      }
    }
    return idstr;
}

std::string getBlockFileMD5(int id)
{
    return BlockFileMD5_[id];
}

std::string getFileMD5()
{
    return fileMD5_;
}

unsigned long getFileNum()
{
    return blockNum_;
}

bool lsFileFinish()
{
    for(int i=0; i<static_cast<int>(blockNum_); i++)
    {
        if(uploadId_[i] == false)
        {
            return false;
        }
    }
    return true;
}

void saveDownloadBlock(int id, std::string BlockContent)
{
    BlockFile_[id] = BlockContent;
    setdownloadId(id);

}

int getidFromMD5(std::string MD5)
{
    int i=0;
    for( ; i<blockNum_; i++)
    {
        if(BlockFileMD5_[i] == MD5)
        {
            return i;
        }
    }
    return -1;
}

int getBlockFileSize()
{
    return BlockFile_.size();
}

std::string getBlockFile(int id)
{
    return BlockFile_[id];
}

~FileValue()
{
    BlockFileMD5_.clear();
    uploadId_.clear();
    downloadId_.clear();
}

private:
    unsigned long               blockNum_;
    std::string                 fileMD5_;
    std::vector<std::string>    BlockFileMD5_;
    std::vector<bool>           uploadId_; 
    std::vector<bool>           downloadId_;  
    std::vector<std::string>    BlockFile_;
};


class Fileproxy
{

public:

Fileproxy() 
{  
}

~Fileproxy() 
{
}

void addFile(std::string FileName, unsigned long blockNum, std::string FileMD5)
{
    FileMap_[FileName] = new FileValue(blockNum, FileMD5);
}
   
std::string getFileidString(std::string FileName)
{
    return FileMap_[FileName]->getuploadId();
}

void setFileMD5(std::string FileName, std::string MD5)
{
    FileMap_[FileName]->setFileMD5(MD5);
}

void setBlockFileMD5(std::string FileName, std::string MD5string)
{
    FileMap_[FileName]->setBlockFileMD5(MD5string);
}

void setFileuploadId(std::string FileName, int id)
{
    FileMap_[FileName]->setuploadId(id);
}

void setdownloadId(std::string FileName, int id)
{
    FileMap_[FileName]->setdownloadId(id);
}

std::string getBlockFileMD5(int id, std::string FileName)
{
    return FileMap_[FileName]->getBlockFileMD5(id);
}

std::string getFileMD5(std::string FileName)
{
    return FileMap_[FileName]->getFileMD5();
}

bool lsFile(std::string FileName)
{
    std::map<std::string, FileValue *>::iterator it;
    it = FileMap_.find(FileName);
    if(it != FileMap_.end())
    {
        return true;
    }
    return false;
}

bool lsFileFinish(std::string FileName)
{
    return FileMap_[FileName]->lsFileFinish();
}

unsigned long getFileNum(std::string FileName)
{
    return FileMap_[FileName]->getFileNum();
}

void saveDownloadBlock(std::string FileName, int id, std::string BlockContent)
{
    FileMap_[FileName]->saveDownloadBlock(id, BlockContent);
}

int getidFromMD5(std::string FileName, std::string MD5)
{
    return FileMap_[FileName]->getidFromMD5(MD5);
}

int getBlockFileSize(std::string FileName)
{
    return FileMap_[FileName]->getBlockFileSize();
}

std::string getBlockFile(std::string FileName, int id)
{
    return FileMap_[FileName]->getBlockFile(id);
}

std::string getNumIdMD5(std::string FileName)
{
    std::string str;
    int num = getFileNum(FileName);
    for(int i=0; i<num; i++)
    {
      str += getBlockFileMD5(i, FileName).data();
    }
    return str;
}


private:
    std::map<std::string, FileValue *>        FileMap_;

};

}; // namespace MD5

MINI::Fileproxy g_File;

#endif