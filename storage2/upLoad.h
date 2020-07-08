#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>    
#include <fstream>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class upLoad
{
public:
  upLoad(tcp::socket& socket)
  : socket_(socket),
    fileName_("./File/"),
    conter_(0),
    statu_(statu1),
    data_(std::vector<char>(max_length)),
    head_(std::vector<char>(head_length))
  {
    start();
  }

  void start()
  {
    while(true)
    {
      switch (statu_)
      {
      case statu1:  mkfile();
                    break;
      case statu2:  saveBlockFile();
                    break;
      case statu3:  finishresv();
                    break;
      default:      break;
      }
    }
  }

private:
  inline uint32_t network_to_host_long(std::vector<char>& head)
  {
    const void *tmp = &*head.begin();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    return ntohl(be32);
  }

  void finishresv()
  {
    std::cout << "resv finish!" << '\n';
    fileName_.clear();
    fileName_ = "./File/";
    boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), 8), ec_);
    while(ec_)
    {
      boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), 8), ec_);
    } 
    statu_ = statu1;
  }

  void mkfile()
  {
    boost::asio::read(socket_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_)
    {
      std::cout << "mkfile error" << '\n';
      statu_ = statu3;
    }
    size_t fileLen = network_to_host_long(head_);
    //std::cout << "fileLen: " << fileLen << '\n';
    
    boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), fileLen), ec_);
    if(ec_)
    {
      std::cout << "mkfile error" << '\n';
      statu_ = statu3;
    } 
    fileName_ += std::string(&*data_.begin(), fileLen) + '/';
    mkdir(fileName_.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    std::cout << "start save file: " << fileName_ << '\n';
    statu_ = statu2;
  }

  void saveBlockFile()
  {
    while(true)
    { 
      boost::asio::read(socket_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
      if(ec_)
      {
        std::cout << "saveBlockFile error" << '\n';
        statu_ = statu3;
      } 
      size_t blocksize = network_to_host_long(head_);
      //std::cout << "blocksize: " << blocksize << '\n';
      
      boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), 32), ec_);
      if(ec_)
      {
        std::cout << "saveBlockFile error" << '\n';
        statu_ = statu3;
      } 
      std::string blockmd5 = std::string(&*data_.begin(), 32);
      //std::cout << blockmd5 << '\n';
      if(blocksize == 0 && blockmd5 == "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE")
      {
        std::cout << "\nsave file finish: " << fileName_ << '\n';
        break;
      }
      if(blocksize == 0 && blockmd5 == "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM")
      {
        continue;
      }

      boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), blocksize), ec_);
      if(ec_)
      {
        std::cout << "saveBlockFile error" << '\n';
        statu_ = statu3;
      } 
      std::string inputstr = std::string(&*data_.begin(), blocksize);
      std::string md5 = getstringMD5(inputstr);
      if(blockmd5 == getstringMD5(inputstr))
      {
        ++conter_;
        printf("\rreceive block nums: %d", conter_);
        std::ofstream f_out;
        std::string filename;
        filename = fileName_ + md5 + ".txt";
        f_out.open(filename, std::ios::out | std::ios::app); 
        f_out << inputstr;
        f_out.close();
      }
    }
  }

  tcp::socket&    socket_;
  std::string     fileName_;
  int             conter_;
  enum StatuFile { statu1, statu2, statu3, statu4 };
  StatuFile       statu_;
  enum { max_length = 2*1024*1024, head_length = 4 };
  std::vector<char> data_;
  std::vector<char> head_;
  boost::system::error_code ec_;
};