#ifndef MD_MD5_H
#define MD_MD5_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <openssl/md5.h>

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

#endif