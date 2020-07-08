#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>  
#include <dirent.h>
#include <iostream>
#include <openssl/md5.h>
#include <fcntl.h>
#include <sys/wait.h>   
#include <boost/asio.hpp>
#include <ctime>
#include <chrono>
#include "File.h"
#include "upLoad.h"
#include "downLoad.h"
#include "mysocket.h"

#define PORT                    9981 
#define BUFFERSIZE              1024*1024 

using namespace Md;

struct time_t_clock
{
  // The duration type.
  typedef std::chrono::steady_clock::duration duration;

  // The duration's underlying arithmetic representation.
  typedef duration::rep rep;

  // The ratio representing the duration's tick period.
  typedef duration::period period;

  // An absolute time point represented using the clock.
  typedef std::chrono::time_point<time_t_clock> time_point;

  // The clock is not monotonically increasing.
  static constexpr bool is_steady = false;

  // Get the current time.
  static time_point now() noexcept
  {
    return time_point() + std::chrono::seconds(std::time(0));
  }
};

// The boost::asio::basic_waitable_timer template accepts an optional WaitTraits
// template parameter. The underlying time_t clock has one-second granularity,
// so these traits may be customised to reduce the latency between the clock
// ticking over and a wait operation's completion. When the timeout is near
// (less than one second away) we poll the clock more frequently to detect the
// time change closer to when it occurs. The user can select the appropriate
// trade off between accuracy and the increased CPU cost of polling. In extreme
// cases, a zero duration may be returned to make the timers as accurate as
// possible, albeit with 100% CPU usage.
struct time_t_wait_traits
{
  // Determine how long until the clock should be next polled to determine
  // whether the duration has elapsed.
  static time_t_clock::duration to_wait_duration(
      const time_t_clock::duration& d)
  {
    if (d > std::chrono::seconds(1))
      return d - std::chrono::seconds(1);
    else if (d > std::chrono::seconds(0))
      return std::chrono::milliseconds(10);
    else
      return std::chrono::seconds(0);
  }

  // Determine how long until the clock should be next polled to determine
  // whether the absoluate time has been reached.
  static time_t_clock::duration to_wait_duration(
      const time_t_clock::time_point& t)
  {
    return to_wait_duration(t - time_t_clock::now());
  }
};

typedef boost::asio::basic_waitable_timer<
  time_t_clock, time_t_wait_traits> time_t_timer;


void printfMD5vec(std::vector<std::string> MD5str)
{
    for(int i=0; i<MD5str.size(); i++)
    {
        std::cout << MD5str[i] << '\n';
    }
}

void getMD5formFile(const char* path, std::vector<std::string>& blockMD5Num)
{
    DIR *dir;
    if ((dir=opendir(path)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }
    struct dirent *ptr;
    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0) //current dir OR parrent dir
            continue;
        else
        {
            std::string Name = ptr->d_name;
            blockMD5Num.push_back(Name.substr(0,32));
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if(argc != 3) { 
        printf("Please input: ./test filename upload/download\n");
        exit(-1);
    }
    int conndfd;
    int conn[3];
    std::string fileName = argv[1];
    std::string opr = argv[2];
    if(opr == "upload")
    {
        conndfd = mySocket1(9981);
        ::send(conndfd, "upppload", 8, 0);
        upLoadFile(conndfd, fileName);
    }
    else if(opr == "download")
    {
        if(access(argv[1], 0) == 0)
        {
            getMD5formFile(argv[1], blockMD5Num);
            //std::cout << "frome file: " << blockMD5Num.size() << '\n';
        }
        else
        {
            mkdir(argv[1], S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        }
        
        conndfd = mySocket2(9981);
        ::send(conndfd, "download", 8, 0);
        std::cout << "start frome proxy receive file‘s md5 " << '\n';
        downLoadMD5(conndfd, MD5str, argv[1]);
        std::cout << "The number of files md5 received frome proxy: " << MD5str.size() << '\n';

        if(blockMD5Num.size() == MD5str.size())
        {
            combinateFile(MD5str, argv[1]);
            exit(0);
        }
        
        conn[0] = mySocket2(9982);
        conn[1] = mySocket2(9983);
        conn[2] = mySocket2(9980);
        ::send(conn[0], "download", 8, 0);
        ::send(conn[1], "download", 8, 0);
        ::send(conn[2], "download", 8, 0);
        std::cout << "start frome storages download file" << '\n';
        std::thread thr1(std::bind(downLoadFileBlock, conn[0], argv[1]));
        std::thread thr2(std::bind(downLoadFileBlock, conn[1], argv[1]));
        std::thread thr3(std::bind(downLoadFileBlock, conn[2], argv[1]));
        thr1.detach();
        thr2.detach();
        thr3.detach();
        
        boost::asio::io_context io_context;
        time_t_timer timer(io_context);
        while(true)
        {
            if(blockMD5Num.size() == MD5str.size())
            {
                std::cout << "\ndownload finish!" << '\n';
                std::cout << "start combine files from local" << '\n';
                combinateFile(MD5str, argv[1]);
                std::cout << "\ncombine files finish" << '\n';
                break;
            }
            timer.expires_after(std::chrono::seconds(1));
            // std::cout << "Starting synchronous wait\n";
            timer.wait();
            // std::cout << "Finished synchronous wait\n";
        }
        
        close(conn[0]); 
        close(conn[1]); 
        close(conn[2]); 
    }
    else
    {
        printf("Please input: ./test filename upload/download\n");
        exit(-1);
    }
  
    close(conndfd); 
    return 0;
}