#pragma once

#include "TCPCore.h"
#include <string>
#include <vector>
#include <stdlib.h>
#include <pthread.h>
#include <cstdio>
#include <map>


enum IRC_RESPONSE
{
  RSP_IGNORE = 0,
  RSP_CONTINUE,
  RSP_RET_TRUE,
  RSP_RET_FALSE,
  RSP_BREAK
};

class IRC_Client
{
public:
  //Initialize all sockets and stuff given IP, port, etc
  bool Initialize(const char *address, const int port, bool verbose = false);

  //login to server, give login info
  bool Login(const char *password, const char *nickname, const char *user);

  //disconnect
  bool Disconnect();

  //main update loop
  void Run();

  //join/leave channel
  void Join(const std::string channel, bool hasPound = false);
  void Part(const std::string channel);

  //main update

private:
  // MEMBERS ////////////////////////////////////////////////////////
  SOCKET ClientSocket_;         //socket of the client
  sockaddr_in *ServerAddress_;  //address of the server
  std::string Nickname_;        //nickname of user
  bool Close_;                  //should we close irc?
  bool Verbose_;                //verbose output
  std::map<std::string, bool> ChannelMap_;

  // METHODS ////////////////////////////////////////////////////////
  //parsing user input to decide what to do
  bool HandleUserInput(std::string message);

  //handling any message from server
  IRC_RESPONSE HandleServerMessage(std::string message);

  //send message
  void SendMessage(std::string message);
  void SendMessage(std::string prefix, std::string command, std::string parameters, std::string trail);

  // UTILITIES //////////////////////////////////////////////////////
  //pull nickname from prefix
  std::string GetNickname(std::string prefix);

  //parse
  void ParseServerMessage(std::string message, std::string &prefix, std::string &command, std::string &end, std::vector<std::string> &parameters);

  // COMMANDS ///////////////////////////////////////////////////////
  void CMD_Ping(std::string input);

  // USER INPUT /////////////////////////////////////////////////////
  static void *_inputThread(void*);
};
