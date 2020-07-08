#include <sys/stat.h>    
#include <fcntl.h>
#include <dirent.h>
#include "md5.h"
#include <string.h>
#include <memory.h>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class downLoad
{
public:
  downLoad(tcp::socket& socket)
  : socket_(socket),
    fileName_("./File/"),
    data_(std::vector<char>(max_length)),
    head_(std::vector<char>(head_length))
  {
  }

  void download()
  {
    boost::asio::read(socket_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_)
    {
      std::cout << "downLoad error" << '\n';
      return ;
    }
    size_t fileLen = network_to_host_long(head_);
    // std::cout << "fileLen: " << fileLen << '\n';
    
    boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), fileLen), ec_);
    if(ec_)
    {
      std::cout << "downLoad error" << '\n';
      return ;
    } 
    fileName_ += std::string(&*data_.begin(), fileLen) + '/';
    // std::cout << fileName_ << '\n';
    std::cout << "start send file block" << '\n';
    readFileList();
    std::cout << "send file block finish" << '\n';
  }

private:
  
  inline uint32_t network_to_host_long(std::vector<char>& head)
  {
    const void *tmp = &*head.begin();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    return ntohl(be32);
  }

  void readFileList()
  {
    DIR *dir;
    if ((dir=opendir(fileName_.data())) == NULL)
    {
      perror("Open dir error...");
      //exit(1);
    }
    struct dirent *ptr;
    while ((ptr=readdir(dir)) != NULL)
    {
      if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    //current dir OR parrent dir
        continue;
      else
      {
        std::string Name = ptr->d_name;
        std::string fileName = fileName_ + Name;
        int fd = open(fileName.data(),O_RDONLY);
        if(-1 == fd)
        {
          perror("ferror");
          //exit(-1);
        }
        struct stat fs;
        fstat(fd, &fs);
        uint32_t bsbe = htonl(fs.st_size);
        std::string sendbuf = "";
        sendbuf.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), head_length);
        sendbuf.append(Name.data(), Name.size());
        int n, location = 0, filesize = fs.st_size;
        char *buff = (char*)malloc(max_length*sizeof(char));
        while((n = read(fd, buff+location, filesize)) < filesize)
        {
          location += n;
          filesize -= n;
        }
        // std::cout << std::string(buff, filesize).size() << '\n';
        sendbuf.append(std::string(buff, filesize).data(), filesize);
        boost::asio::write(socket_, boost::asio::buffer(sendbuf.data(), sendbuf.size()), ec_);
        if(ec_)
        {
          return ;
        }
        // std::cout << sendbuf.size() << '\n';
        // printf("d_name:%s%s\n",fileName_.data(), ptr->d_name);
        close(fd);
      }
    }
    closedir(dir);
  }

  tcp::socket&    socket_;
  std::string     fileName_;
  enum { max_length = 1024*1024, head_length = 4 };
  std::vector<char> data_;
  std::vector<char> head_;
  boost::system::error_code ec_;
};
