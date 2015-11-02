
#include "IRCClient.h"
#include <iostream>
#include <string>
#include <getopt.h>

//input
std::string host;
bool verbose = false;

unsigned port;
std::string ipAddress;
std::string password;
std::string username;
std::string channel = "#CS260";

//making sure we're ready
unsigned bitField = 7;

int main(int argc, char *argv[])
{
  IRC_Client client;


  ///////////////////////////////////////////////////////////////////
  // handling getopt cmd arguments 
  optind = 0;
  int input = 0;

  //no bloody clue why this can't be in the while loop but whatever
  input = getopt(argc, argv, ":h:u:p:c::v::");

  /*loop until getopt == -1*/
  while((input) != -1)
  {
    /*case statement, handle input*/
    switch(input)
    {
      case 'h': //host
        host = optarg;
        bitField -= 1;
        break;
      case 'u': //username
        username = optarg;
        bitField -= 2;
        break;
      case 'p': //password
        password = optarg;
        bitField -= 4;
        break;
      case 'c': //optional channel
        //in-case # is forgotten
        if(optarg[0] != '#')
          channel = std::string("#") + optarg;
        else
          channel = optarg;
        break;
      case 'v':
        verbose = true;
        break;
      case ':': //missing option                  
        std::cout << "Missing option for '" << static_cast<char>(optopt) << "'." << std::endl;
        std::cout << "Usage: simplechat -h host:port -u username -p password -c channel -v" << std::endl;
        return -1;
        break;
      case '?': //unknown option                  
        std::cout << "Unknown option '" << static_cast<char>(optopt) << "'." << std::endl;
        return -1;
        break;
      default:
        break;
    }
    input = getopt(argc, argv, ":h:u:p:c::v::");
  }

  //making sure all parameters were passed
  if(bitField != 0)
  {
    std::cout << "Usage: simplechat -h host:port -u username -p password -c channel -v" << std::endl;
    return -1;
  }

  //seperate port and ipAddress
  size_t colonPos = host.find(":");
  if(colonPos != std::string::npos)
  {
    port = atoi(host.substr(colonPos + 1, host.size() - colonPos - 1).c_str());
    ipAddress = host.substr(0, colonPos);
  }
  else
  {
    port = 6667;
    ipAddress = host;
  }

  ///////////////////////////////////////////////////////////////////
  //initialize irc
  //now that all input is done, it's time to launch irc
  if(client.Initialize(ipAddress.c_str(), port, verbose))
  {
    //logic w/ default user info
    if(client.Login(password.c_str(), username.c_str(), username.c_str()))
    {
      //managed to log in, join the channel
      client.Join(channel, true);
      //run the main loop
      client.Run();

      return 0;
    }
    else
    {
      //failed to login, ret -1
      std::cout << "Failed to login!" << std::endl;
      std::cout << "Server: '" << ipAddress << ":" << port << "'." << std::endl;
      std::cout << "  Username: '" << username << "'." << std::endl;
      std::cout << "  Password: '" << password << "'." << std::endl;
      std::cout << std::endl;
      return -1;
    }
  }
  else
  {
    //failed to login, ret -1
    std::cout << "Failed to connect socket!" << std::endl;
    return -1;
  }
  
  //failed to make connection
  return -1;
}