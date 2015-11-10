#include<iostream>    //cout
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<string>  //string
#include<cstdlib>
//#include<sys/socket.h>    //socket
//#include<arpa/inet.h> //inet_addr
//#include<netdb.h> //hostent

int main(int argc , char *argv[])
{
  int port;
  if (argc ==  2){
    int port = atoi(argv[1]);
  }
  else {
    std::cout << "bad arguments" << std::endl;
    return 1;
  }

return 0;

}
