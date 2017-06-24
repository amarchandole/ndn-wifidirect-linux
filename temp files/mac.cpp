#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <string.h>
#include <stdlib.h>
#include <cstdio>
#include <fstream>

std::string
getMAC(std::string name)
{
  const char * ifname = name.c_str();
  struct ifreq s;
  //char c[2];
  char c[2];
  std::string mac = "";
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

  strcpy(s.ifr_name, ifname);
  if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
    int i;
    for (i = 0; i < 6; ++i) {
      std::sprintf(c, "%02x", (unsigned char) s.ifr_addr.sa_data[i]);
      mac = mac + c;
      mac = mac + ":";
    }
    mac[17]='\0';
    return mac;
  }
}

static char transform(char c) {
  if(c == '0')
    return '2';
  if(c == '1')
    return '3';
  if(c == '2')
    return '0';
  if(c == '3')
    return '1';
  if(c == '4')
    return '6';
  if(c == '5')
    return '7';
  if(c == '6')
    return '4';
  if(c == '7')
    return '5';
  if(c == '8')
    return 'a';
  if(c == '9')
    return 'b';
  if(c == 'a')
    return '8';
  if(c == 'b')
    return '9';
  if(c == 'c')
    return 'e';
  if(c == 'd')
    return 'f';
  if(c == 'e')
    return 'c';
  if(c == 'f')
    return 'd';

  return '-';
}

static std::string mac_to_ipv6(const char *mac, std::string ipv6) 
{
  //ipv6 = (char*)malloc(26);

  ipv6 += "fe80::";

  ipv6 += mac[0];
  ipv6 += transform(mac[1]);
  ipv6 += mac[3];
  ipv6 += mac[4];
  
  ipv6 += ":";
  
  ipv6 += mac[6];
  ipv6 += mac[7];
  ipv6 += "ff:fe";
  
  ipv6 += mac[9];
  ipv6 += mac[10];

  ipv6 += ":";
  
  ipv6 += mac[12];
  ipv6 += mac[13];
  ipv6 += mac[15];
  ipv6 += mac[16];

  std::cout<<"IPv6 Address: \n"<<ipv6<<std::endl;
  return ipv6;
}

int main()
{
  std::string mac = getMAC("ens33");
  const char * MAC = mac.c_str();
  std::cout<<"MAC is: "<<mac<<std::endl;
  //char *ipv6 = NULL;
  std::string ipv6 = "";
  ipv6 = mac_to_ipv6(MAC, ipv6);

  std::cout<<"Trying to read"<<std::endl;

  std::ifstream myfile ("/tmp/ndntemp");
  std::string line;
  if (myfile.is_open())
  {
    getline(myfile,line); 
    std::cout << "Line: "<<line<<std::endl; 
    myfile.close();
  }
  else 
    std::cout << "Unable to open file"; 
}