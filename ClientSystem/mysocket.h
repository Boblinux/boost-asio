#ifndef MD_MYSOCKET_H
#define MD_MYSOCKET_H

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <algorithm>
#include <arpa/inet.h>


void setTcpNoDelay(int sockfd_, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, sizeof optval);
}

void setKeepAlive(int sockfd_, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
}

inline int mySocket1(int PORT)
{
    int conndfd;
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serverAddr.sin_port=htons(PORT);
    
    if((conndfd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
      printf("Create Socket Error\n");
    }

    setTcpNoDelay(conndfd, true);
    setKeepAlive(conndfd, true);
    if(-1 == connect(conndfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)))
    {  
      printf("Connetc Error\n");
    }
    return conndfd;
}

inline int mySocket2(int PORT)
{
    int conndfd;
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serverAddr.sin_port=htons(PORT);
    
    if((conndfd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
      printf("Create Socket Error\n");
    }

    setTcpNoDelay(conndfd, true);
    setKeepAlive(conndfd, true);
    while(0 != connect(conndfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)))
    {  
      
    }
    //printf("Connetc success\n");
    return conndfd;
}
#endif