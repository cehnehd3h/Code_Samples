#include "TCPCore.h"

#ifdef WIN32
  static void _internalCloseSocket(SOCKET socket)
  {
    closesocket(socket);
  }

  #define IOCTL(socket, cmd, argp) ioctlsocket(result, FIONBIO, &mode);
#else
  static void _internalCloseSocket(SOCKET socket)
  {
    close(socket);
  }

  #define IOCTL(socket, cmd, argp) ioctl(result, FIONBIO, &mode);
#endif

//initialize
int Initialize()
{
#ifdef WIN32
  WSADATA wsa;
  int error;

  error = WSAStartup(MAKEWORD(2, 2), &wsa);
  return error;
#else
  return 0;
#endif
}

//make a socket
SOCKET CreateSocket(int protocol, bool blocking)
{
  SOCKET result = INVALID_SOCKET;

  if (blocking)
  {
    int type = SOCK_DGRAM;
    if (protocol == IPPROTO_TCP)
      type = SOCK_STREAM;

    result = socket(AF_INET, type, protocol);

    return result;
  }
  else
  {
    SOCKET result = INVALID_SOCKET;
    int type = SOCK_DGRAM;
    if (protocol == IPPROTO_TCP)
      type = SOCK_STREAM;

    result = socket(AF_INET, type, protocol);
    if (result != INVALID_SOCKET && protocol == IPPROTO_TCP)
    {
      unsigned long mode = 1;
      int err = IOCTL(result, FIONBIO, &mode);
      if (err != NO_ERROR)
      {
        _internalCloseSocket(result);
        result = INVALID_SOCKET;
      }
    }

    return result;
  }
}

//make remote address
sockaddr_in *CreateAddress(const char *ip, int port)
{
  sockaddr_in* result = (sockaddr_in*)calloc(sizeof(*result), 1);

  result->sin_family = AF_INET;
  result->sin_port = htons(port);

  if (ip == NULL)
#ifdef WIN32
    result->sin_addr.S_un.S_addr = INADDR_ANY;
#else
    result->sin_addr.s_addr = INADDR_ANY;
#endif
  else
    inet_pton(result->sin_family, ip, &(result->sin_addr));

  return result;
}

//connect to the socket to the address
int ConnectSocket(SOCKET socket, sockaddr_in *address)
{
  if (connect(socket, (sockaddr*)address, sizeof(sockaddr_in)) == SOCKET_ERROR)
  {
    int error = GETERROR();

    //in the progress of connecting, we need to wait
    if (error == 10035)
    {
      return 0;
    }
    else
      return error;
  }
  else
    return 0;
}

//send data
int SendTCP(SOCKET socket, char* buffer, int bytes, sockaddr_in* dest)
{
  int result = sendto(socket, buffer, bytes, 0, (sockaddr*)dest, sizeof(sockaddr_in));
  if (result == SOCKET_ERROR)
    return GETERROR();
  else
    return result;
}

//recieve data
int ReceiveTCP(SOCKET socket, char *dataBuffer, int bufferSize)
{
  int bytes = recv(socket, dataBuffer, bufferSize, 0);
  if (bytes == SOCKET_ERROR)
    return -1;
  return bytes;
}

//close socket
void CloseSocket(SOCKET socket, bool now)
{
  if (now)
  {
    _internalCloseSocket(socket);
  }
  else
    shutdown(socket, SHUTDOWN_NO_SEND);

}

//uninitialize
void Uninitialize()
{
#ifdef WIN32
  WSACleanup();
#endif
}
