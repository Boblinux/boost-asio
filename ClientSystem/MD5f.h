#ifndef MD_Md5_H
#define MD_Md5_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <openssl/md5.h>

typedef unsigned long long ll;
namespace Md
{

class Md5
{

public:

Md5(int fd, ll blockSize, off_t offset) 
    : fd_(fd),
      blockSize_(blockSize),
      offset_(offset)
{
}

~Md5() 
{
}
    
std::string getMD5() 
{ 
    unsigned char tmp1[MD5_DIGEST_LENGTH];

    char *file_buffer = (char*)mmap(0, blockSize_, PROT_READ, MAP_SHARED, fd_, offset_);

    MD5((const unsigned char*) file_buffer, blockSize_, tmp1);

    munmap(file_buffer, blockSize_); 

    char buf[33] = {0};
    char tmp[3] = {0};
    for(int i=0; i<MD5_DIGEST_LENGTH; i++) {
        sprintf(tmp,"%02X", tmp1[i]); 
		strcat(buf, tmp); 
    }
    md5_ = buf;
    return md5_; 
}


private:
    std::string     md5_;
    int             fd_; 
    ll              blockSize_;
    off_t           offset_;
      
};

}; // namespace MD5

#endif