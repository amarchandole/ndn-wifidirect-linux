#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main()
{
  char MAC[4];
  FILE *fptr;
  // struct ifreq s;
  // int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

  // strcpy(s.ifr_name, "eth0");
  // if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
  //   int i;
  //   for (i = 0; i < 6; ++i)
  //     printf(" %02x", (unsigned char) s.ifr_addr.sa_data[i]);
  //   puts("\n");
  // }

  printf("Enter the MAC address of the WiFi-Direct device you want to connect to:\n");
  scanf("%s", MAC);
  MAC[3]='\0';

  fptr = fopen("/tmp/ndntemp", "w");
  if(fptr == NULL)
  {
    printf("Error!");
  }
  fprintf(fptr, "%s", MAC);
  fclose(fptr);

  return 1;
}