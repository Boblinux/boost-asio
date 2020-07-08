#ifndef MINI_UPLOAD_H
#define MINI_UPLOAD_H

#include <cstdlib>
#include <arpa/inet.h>
#include <iostream>
#include <iomanip>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <deque>
#include <thread>
#include <openssl/md5.h>
#include "fileproxy.h" 

using boost::asio::ip::tcp;

class upload
{

public:
  upload(tcp::socket& socket0, tcp::socket& socket1, tcp::socket& socket2, tcp::socket& socket3)
  : socket0_(socket0),
    socket1_(socket1),
    socket2_(socket2),
    socket3_(socket3),
    upStatu_(upstatu1),
    FileName_(),
    data_(std::vector<char>(max_length)),
    head_(std::vector<char>(head_length))
  {
    start();
  }
  ~upload() {}

  void start()
  {
    while(true)
    {
      switch(upStatu_)
      {
        case upstatu1:  switchOpr();
                        break;
        case upstatu2:  initFileNumber(); 
                        break;
        case upstatu3:  initFileMD5(); 
                        break;
        case upstatu4:  sendBlock(); 
                        break;
        case upstatu5:  downLoad(); 
                        break;
        case upstatu6:  break;
      }
      if(upStatu_ == upstatu6)
      {
        break;
      }
    }
  }

private:

  inline void abnormal()
  {
    upStatu_ = upstatu6;
    std::string sendstr = "";
    uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(0);
    sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
    sendstr.append("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM", 32);
    boost::asio::write(socket1_, boost::asio::buffer(sendstr.data(), sendstr.size()));
    boost::asio::write(socket2_, boost::asio::buffer(sendstr.data(), sendstr.size()));
    boost::asio::write(socket3_, boost::asio::buffer(sendstr.data(), sendstr.size()));
  }

  inline int convertMD5toInt(std::string md5)
  {
    int res = 0;
    for(int i=0; i<32; i++)
    {  
      res += static_cast<int>(md5[i]);
    }
    return res;
  }

  inline std::string getFileName(std::string FileName)
  {
    int i=FileName.size()-1;
    for( ; i>=0; i--)
    {
      if(FileName[i] == '/')
      {
        break;
      }
    }
    return FileName.substr(i+1, FileName.size()-i-1);
  }

  inline std::string getstringMD5(std::string str)
  {
    unsigned char tmp1[MD5_DIGEST_LENGTH];
    ::MD5((unsigned char*) str.data(), str.size(), tmp1);
    char buf[33] = {0};  
    char tmp2[3] = {0};  
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++ )  
    {  
        sprintf(tmp2,"%02X", tmp1[i]);  
        strcat(buf, tmp2);  
    }
    std::string md5 = buf;
    return md5;
  }

  void switchOpr()
  {
    boost::asio::read(socket0_, boost::asio::buffer(data_, 8), ec_); 
    if(ec_)
    {
      std::cout << "switchOpr error" << '\n';
      upStatu_ = upstatu6;
    }
    if(std::string(&*data_.begin(), 8) == "upppload")
    {
      upStatu_ = upstatu2;
    }
    else if(std::string(&*data_.begin(), 8) == "download")
    {
      upStatu_ = upstatu5;
    }
  }

  inline uint32_t network_to_host_long(std::vector<char>& head)
  {
    const void *tmp = &*head.begin();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    return ntohl(be32);
  }

  void initFileNumber()
  { 
    //get filename+md5 len
    boost::asio::read(socket0_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_)
    {
      std::cout << "initFileNumber error" << '\n';
      upStatu_ = upstatu6;
    }
    size_t fileNameMD5Len = network_to_host_long(head_);
    // std::cout << "fileNameMD5Len: " << fileNameMD5Len << '\n';
    
    //get blockNum len
    boost::asio::read(socket0_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_)
    {
      std::cout << "initFileNumber error" << '\n';
      upStatu_ = upstatu6;
    }
    size_t blockNum = network_to_host_long(head_);
    // std::cout << "blockNum: " << blockNum << '\n';
    
    boost::asio::read(socket0_, boost::asio::buffer(&*data_.begin(), fileNameMD5Len), ec_); 
    if(ec_)
    {
      std::cout << "initFileNumber error" << '\n';
      upStatu_ = upstatu6;
    }
    std::string Data = std::string(&*data_.begin(), fileNameMD5Len);
    FileName_ = getFileName(Data.substr(0,fileNameMD5Len-32));
    // std::cout << FileName_ << '\n';

    if(!g_File.lsFile(FileName_))
    {
      g_File.addFile(FileName_, blockNum, Data.substr(fileNameMD5Len-32));
      int fileNameLen = FileName_.size();
      uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(fileNameLen);
    
      std::string sendstr = "";
      sendstr.append("upppload", 8);
      sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
      sendstr += FileName_;
      boost::asio::write(socket1_, boost::asio::buffer(sendstr, 8+head_length+fileNameLen));
      boost::asio::write(socket2_, boost::asio::buffer(sendstr, 8+head_length+fileNameLen));
      boost::asio::write(socket3_, boost::asio::buffer(sendstr, 8+head_length+fileNameLen));
      //std::cout << FileName_ << '\n';
      //add
      sendstr.clear();
      //nbe = boost::asio::detail::socket_ops::host_to_network_long(3);
      //sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
      sendstr.append("ACK", 3);
      boost::asio::write(socket0_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      upStatu_ = upstatu3;
    }
    else
    {
      if(g_File.lsFileFinish(FileName_))
      {
        upStatu_ = upstatu6;

        //add
        std::string sendstr = "";
        //uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(7);
        //sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
        sendstr.append("success", 7);
        boost::asio::write(socket0_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      }
      else
      {
        upStatu_ = upstatu4;
        //add
        std::string sendstr = "";
        std::string MD5string = g_File.getFileidString(FileName_);
        //uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(MD5string.size());
        //sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
        sendstr.append(MD5string.data(), MD5string.size());
        boost::asio::write(socket0_, boost::asio::buffer(sendstr.data(), sendstr.size()));
        // std::cout << sendstr.data() << '\n';
      }
    }
  }

  void initFileMD5()
  {
    boost::asio::read(socket0_, boost::asio::buffer(&*data_.begin(), 32*g_File.getFileNum(FileName_)), ec_);
    if(ec_)
    {
      std::cout << "initFileMD5 error" << '\n';
      upStatu_ = upstatu6;
    }
    g_File.setBlockFileMD5(FileName_, std::string(&*data_.begin(), 32*g_File.getFileNum(FileName_)).data());
    upStatu_ = upstatu4;
  }

  void sendBlock()
  {
    boost::asio::read(socket0_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_) { 
      std::cout << "sendBlock error" << '\n';
      abnormal();
    }
    uint32_t blocksize = network_to_host_long(head_);
    // std::cout << "blocksize: " << blocksize << '\n';
    
    boost::asio::read(socket0_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_) { 
      std::cout << "sendBlock error" << '\n';
      abnormal(); 
    }
    uint32_t blockid = network_to_host_long(head_);
    // std::cout << "blockid: " << blockid << '\n';
    
    boost::asio::read(socket0_, boost::asio::buffer(&*data_.begin(), blocksize), ec_);
    if(ec_) { 
      std::cout << "sendBlock error" << '\n';
      abnormal();
    }
    std::string md5 = getstringMD5(std::string(&*data_.begin(), blocksize));
        
    if(md5 == g_File.getBlockFileMD5(blockid, FileName_)) 
    {
      int filelocation = convertMD5toInt(md5)%3;
      md5 += std::string(&*data_.begin(), blocksize);
      uint32_t bsbe32 = htonl(blocksize);
      std::string sendstr = "";
      sendstr.append(static_cast<const char*>(static_cast<const void*>(&bsbe32)), head_length);
      sendstr.append(md5.data(), md5.size());
      if(filelocation == 0)
      {
        boost::asio::write(socket1_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      }
      else if(filelocation == 1)
      {
        boost::asio::write(socket2_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      }
      else
      {
        boost::asio::write(socket3_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      }
      g_File.setFileuploadId(FileName_, blockid);
      // std::cout << "seccess" <<std::endl;
    }
    else
    {
      std::cout << "error" <<std::endl;
    }
      
    if(g_File.lsFileFinish(FileName_))
    {
      upStatu_ = upstatu6;
      std::string sendstr = "";
      //uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(7);
      //sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
      sendstr.append("success", 7);
      boost::asio::write(socket0_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      sendstr.clear();
      uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(0);
      sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
      sendstr.append("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE", 32);
      boost::asio::write(socket1_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      boost::asio::write(socket2_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      boost::asio::write(socket3_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      std::cout << "save " << FileName_ << " success!" << '\n';
    }
  }

  void downLoad()
  {
    boost::asio::write(socket0_, boost::asio::buffer("down ack", 8));
    boost::asio::read(socket0_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_)
    {
      std::cout << "downLoad error" << '\n';
      upStatu_ = upstatu6;
    }
    uint32_t FileNameLen = network_to_host_long(head_);
    // std::cout << "FileNameLen: " << FileNameLen << '\n';

    boost::asio::read(socket0_, boost::asio::buffer(&*data_.begin(), FileNameLen), ec_);
    if(ec_)
    {
      std::cout << "downLoad error" << '\n';
      upStatu_ = upstatu6;
    }
    FileName_ = std::string(&*data_.begin(), FileNameLen);  
    if(g_File.lsFile(FileName_))
    {
      //std::cout << FileName_ << " is in map" << '\n';
      //add
      std::string sendstr = "";
      uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(g_File.getNumIdMD5(FileName_).size());
      sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
      sendstr.append(g_File.getNumIdMD5(FileName_).data(), g_File.getNumIdMD5(FileName_).size());
      boost::asio::write(socket0_, boost::asio::buffer(sendstr.data(), sendstr.size()));
      // std::cout << g_File.getNumIdMD5(FileName_).data();
    }
    else
    {
      std::string sendstr = "";
      uint32_t nbe = boost::asio::detail::socket_ops::host_to_network_long(6);
      sendstr.append(static_cast<const char*>(static_cast<const void*>(&nbe)), head_length);
      sendstr.append("NOFile", 6);
      boost::asio::write(socket0_, boost::asio::buffer(sendstr.data(), sendstr.size()));
    }
    upStatu_ = upstatu6;
  }


  tcp::socket& socket0_;
  tcp::socket& socket1_;
  tcp::socket& socket2_;
  tcp::socket& socket3_;
  enum upStatuFile { upstatu1, upstatu2, upstatu3, upstatu4, upstatu5, upstatu6 };
  upStatuFile upStatu_;
  std::string FileName_;
  enum { max_length = 1024*1024, head_length = 4 };
  std::vector<char> data_;
  std::vector<char> head_;
  boost::system::error_code ec_;
};

#endif