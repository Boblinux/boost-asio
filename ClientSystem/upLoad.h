#ifndef MD_UPLOAD_H
#define MD_UPLOAD_H

#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include "File.h"

#define BUFFERSIZE              1024*1024 
using namespace Md;

// Get the size of the file by its file descriptor
ll getFileSize(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}

void upLoadFile(int conndfd, std::string argv)
{
    static char buffer[BUFFERSIZE];
    bzero(buffer, sizeof(buffer)); 
    int n;
    int fd = open(argv.data(), O_RDONLY);
    ll file_size = getFileSize(fd);
    Md::File File(fd, conndfd, file_size); 
    int blockNum = File.getFileBlockNum();
    std::cout << "File num: " << blockNum << " File MD5: "<< File.getFileMD5()  << std::endl;
    File.sendFileinfo(argv); //1、seng Fileinfo
    if((n = recv(conndfd, buffer, BUFFERSIZE, 0)) > 0)
    {
        std::string idstr = buffer;
        bzero(buffer, sizeof(buffer)); 
        //std::cout << idstr << std::endl;
        if(idstr == "success")
        {
            return ;  //5、success
        }
        
        std::string::size_type position = idstr.find(':');
        if(position != idstr.npos)
        {
            File.updateuploadId(idstr); //2、if file existence, updateuploadId
        }
        else
        {
            File.sendFileBlockMD5info(); //3、else sendFileBlockMD5info 
        }
    }
    File.sendFileBlock();   //4、sendFileBlock
    while((n = recv(conndfd, buffer, BUFFERSIZE, 0)) > 0)
    {
        std::cout << buffer << std::endl;
        std::string idstr = buffer;
        bzero(buffer, sizeof(buffer));
        if(idstr == "success")
        {
            break;  //5、success
        }
        else
        {
            File.updateuploadId(idstr); // goto 2、updateuploadId
            File.sendFileBlock(); 
        }
    }
}

#endif