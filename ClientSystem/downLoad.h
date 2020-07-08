#ifndef MD_DOWNLOAD_H
#define MD_DOWNLOAD_H
#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include <map>
#include <thread>
#include <mutex>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "File.h"
#include "Buffer.h"
#include "md5.h"
#include <sys/wait.h>   

std::vector<std::string> MD5str;
std::vector<std::string> blockMD5Num;
std::vector<std::string> content;
std::mutex mux;

void remove_dir(const char *dir)
{
	char cur_dir[] = ".";
	char up_dir[] = "..";
	char dir_name[128];
	DIR *dirp;
	struct dirent *dp;
	struct stat dir_stat;
	if ( 0 > stat(dir, &dir_stat) ) {
		perror("get directory stat error");
	}

	if ( S_ISREG(dir_stat.st_mode) ) {	
		remove(dir);
	} else if ( S_ISDIR(dir_stat.st_mode) ) {	
		dirp = opendir(dir);
		while ( (dp=readdir(dirp)) != NULL ) {
			if ( (0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) ) {
				continue;
			}
			
			sprintf(dir_name, "%s/%s", dir, dp->d_name);
			remove_dir(dir_name);  
		}
		closedir(dirp);

		rmdir(dir);	
	} else {
		perror("unknow file type!");	
	}
}

inline void conbFileblock(const char *basePath, int fd, std::string md5)
{
  int fdb = open(basePath, O_RDONLY);
  if(-1 == fdb)
  {
    std::cout << "get file errror\n";
    perror("ferror");
  }

  struct stat fs;
  fstat(fdb, &fs);
  int n, location = 0, filesize = fs.st_size;
  char *buff = (char*)malloc(1024*1024*sizeof(char));
  while((n = read(fdb, buff+location, filesize)) < filesize)
  {
    location += n;
    filesize -= n;
  }

  if(md5 == getstringMD5(std::string(buff, fs.st_size).data()))
  {
    //std::cout << "protect block success\n";
    ::write(fd, std::string(buff, fs.st_size).data(), std::string(buff, fs.st_size).size());
  }
  else
  {
    std::cout << "protect block error\n";
    std::cout << md5 << '\n';
    std::cout << getstringMD5(std::string(buff, fs.st_size).data()) << '\n';
  }
}

void combinateFile(std::vector<std::string> MD5str, const char *basePath)
{
  
  std::string fileName = std::string("./") + basePath + std::string("/");
  // std::cout << fileName << '\n';
  int fd = ::open(fileName.substr(0, fileName.size()-2).data(), O_RDWR | O_CREAT);
  if(-1 == fd)
  {
    std::cout << "create file errror\n";
    perror("ferror");
  }

  for(int i=0; i<MD5str.size(); i++)
  {
    std::string files = fileName + MD5str[i] + ".txt";
    conbFileblock(files.data(), fd, MD5str[i].substr(0, 32));
    printf("\rcombined block[%.2lf%%]:", (i*100.0) / (MD5str.size()));
  }
  printf("\rcombined block[%.2lf%%]:", 100.0);
  remove_dir(basePath);
  rename(fileName.substr(0, fileName.size()-2).data(), basePath);
}

bool lsMD5InFile(std::string md5)
{
    for(int i=0; i<blockMD5Num.size(); i++)
    {
        if(blockMD5Num[i] == md5)
        {
            return true;
        }
    }
    return false;
}

void downLoadMD5(int conndfd, std::vector<std::string>& MD5str, std::string argv)
{
  const int BUFFSIZE = 1024*1024;
  char BUFFER[BUFFSIZE];
  ::read(conndfd, BUFFER, 8);
  // std::cout <<  BUFFER << '\n';
  
  std::string Buff = "";
  uint32_t bsbe = htonl(argv.size());
  Buff.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), 4);
  Buff.append(argv.data(), argv.size());
  ::send(conndfd, Buff.data(), Buff.size(), 0); 

  bzero(BUFFER, BUFFSIZE);
  int n, location=0, resvsize = 4;
  while((n = ::read(conndfd, BUFFER+location, resvsize)) < resvsize)
  {
     location += n;
     resvsize -= n;
  }
  int32_t be32 = *static_cast<const int32_t*>(
                  static_cast<const void*>(std::string(BUFFER, 4).data()));
  resvsize = ntohl(be32);
  //std::cout<<"resvsize: " << resvsize << 'n';
  location=0;
  bzero(BUFFER, BUFFSIZE);
  while((n = ::read(conndfd, BUFFER+location, resvsize)) < resvsize)
  {
     location += n;
     resvsize -= n;
  }
  std::string str = std::string(BUFFER, ntohl(be32));
  for(int i=0; i+32 <= str.size(); i += 32)
  {
    MD5str.push_back(str.substr(i,32));
  }    
}

void downLoadFileBlock(int conndfd, std::string argv)
{
  std::string sendBuff = "";
  uint32_t bsbe = htonl(argv.size());
  sendBuff.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), 4);
  sendBuff.append(argv.data(), argv.size());
  ::send(conndfd, sendBuff.data(), sendBuff.size(), 0); 
  
  Reactor::Buffer Buff;
  while(true)
  {
    Buff.readFd(conndfd);
    
    while(Buff.readableBytes() >= 4)
    {
      const void *tmp = Buff.peek();
      int32_t be32 = *static_cast<const int32_t*>(tmp);
      size_t BlockFileSize = boost::asio::detail::socket_ops::host_to_network_long(be32);
      //std::cout << BlockFileSize << '\n';
      if(Buff.readableBytes() >= 4+36+BlockFileSize)
      {
        // std::cout << BlockFileSize << '\n';
        Buff.retrieve(4);
        std::string FileContent = Buff.readAsBlock(36+BlockFileSize);
        if(getstringMD5(FileContent.substr(36)) == FileContent.substr(0,32))
        {
          // std::cout <<  FileContent.substr(0,32) << '\n';
          // std::cout <<  getstringMD5(FileContent.substr(36)) << '\n';
          if(lsMD5InFile(FileContent.substr(0,32)))
          {
            continue;
          }
          else
          {
            std::lock_guard<std::mutex> guard(mux);
            blockMD5Num.push_back(FileContent.substr(0,32));
            //std::cout << blockMD5Num.size() << ':' << MD5str.size() << '\n';
            std::string file = "./" + argv + '/' + FileContent.substr(0,36);
            // std::cout << file << '\n';
            int fd = open(file.data(), O_RDWR|O_CREAT);
            chmod(file.data(), 0777);
            ::write(fd, FileContent.substr(36).data(), FileContent.substr(36).size());
            close(fd);
            // std::cout << "resv block success" << '\n';
            printf("\rresv block[%.2lf%%]:", (blockMD5Num.size()*100.0) / (MD5str.size()));
          }
        }
        else
        {
          std::cout << "file resv error" << '\n';
        }
      }
      else
      {
        break;
      }
    }
  }
}

#endif