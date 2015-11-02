#pragma once

#ifdef WIN32  //if compiling on windows
  //library
  #pragma comment(lib, "Ws2_32.lib")

  //includes
  #include <Ws2tcpip.h>
  #include <WinSock2.h>
  #include <stdlib.h>

  //defines for other stuff
  #define GETERROR() WSAGetLastError()

  //shutdown codes
  #define SHUTDOWN_NO_SEND      SD_SEND
  #define SHUTDOWN_NO_RECIEVE   SD_RECEIVE
  #define SHUTDOWN_NO_BOTH      SD_BOTH
#else         //else, assume posix
  //includes
  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <netinet/ip.h>
  #include <arpa/inet.h>
  #include <errno.h>
  #include <unistd.h>
  #include <stdlib.h>

  //typedef to make code all uniform
  typedef int SOCKET;

  //defines for other stuff
  #define GETERROR() errno
  #define NO_ERROR            0
  #define SOCKET_ERROR        -1
  #define INVALID_SOCKET      -1

  //shutdown codes
  #define SHUTDOWN_NO_SEND      SHUT_WR
  #define SHUTDOWN_NO_RECIEVE   SHUT_RD
  #define SHUTDOWN_NO_BOTH      SHUT_RDWR
#endif

//function declarations
//initialize
int Initialize();

//make a socket
SOCKET CreateSocket(int protocol, bool blocking);

//make remote address
sockaddr_in *CreateAddress(const char *ip, int port);

//connect to the socket to the address
int ConnectSocket(SOCKET socket, sockaddr_in *address);

//send data
int SendTCP(SOCKET socket, char* buffer, int bytes, sockaddr_in* dest);

//recieve data
int ReceiveTCP(SOCKET socket, char *dataBuffer, int bufferSize);

//close socket, sends shutdown event if not closed immediately
void CloseSocket(SOCKET socket, bool now);

//uninitialize
void Uninitialize();
