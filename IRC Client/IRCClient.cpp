#include "IRCClient.h"
#include <iostream>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#define START_COMMANDS(cmd)\
  std::string _cmd = cmd;\
  if(false)\
    {\

#define COMMAND(input)\
    }\
    else if(_cmd == std::string(#input))\
    {\

#define END_COMMANDS() }

#define V_MESSAGE(strg)\
      if(Verbose_)\
        printf("\r\e[31m     VERB:\e[34m %s \e[m", (strg));

#define V_MESSAGE_NL(strg)\
      if(Verbose_)\
        printf("\r\e[31m     VERB:\e[34m %s\n \e[m", (strg));



//Initialize all sockets and stuff given IP, port, etc
bool IRC_Client::Initialize(const char *address, const int port, bool verbose)
{
  int result;

  //set these guys
  Close_ = false;
  Verbose_ = verbose;

  //verbose stuff
  V_MESSAGE_NL("---------------INITIALIZE---------------");
  V_MESSAGE_NL("ADDRESS:");
  V_MESSAGE_NL(address);

  //initialize networking
  result = ::Initialize();

  //create blocking socket
  ClientSocket_ = CreateSocket(IPPROTO_TCP, true);

  //create server address
  ServerAddress_ = CreateAddress(address, port);

  //connect socket
  result = ConnectSocket(ClientSocket_, ServerAddress_);
  if(result != 0)
  {
    return false;
  }

  //good! everything worked. Ready to log in 
  return true;
}

//login to server, give login info
bool IRC_Client::Login(const char *password, const char *nickname, const char *user)
{
  //vb stuff
  V_MESSAGE_NL("PASSWORD:");
  V_MESSAGE_NL(password);
  V_MESSAGE_NL("USER:");
  V_MESSAGE_NL(user);

  //construct messages for password, nickname, and user
  std::string pwd = "PASS " + std::string(password) + "\r\n";
  std::string nick = "NICK " + std::string(nickname) + "\r\n";
  std::string usr = "USER " + std::string(user) + " 0 * :MattYan\r\n";

  //send messages
  SendMessage(pwd);
  SendMessage(nick);
  SendMessage(usr);

  //save for later
  Nickname_ = nickname;

  //prepare for incoming messages
  char returnMessage[1024];
  int length = 1024 - 1;
  int result;
  bool breakOut = false;

  std::string currentData;
  std::string nextData;

  //attempt to get the messages from the return
  do
  {
    result = ReceiveTCP(ClientSocket_, returnMessage, length);

    if(result < 0)
    {
      std::cout << "Failed to receive packet during login!" << std::endl;
      break;
    }

    //if we recieved data, add it to the queue
    if (result > 0)
    {
      returnMessage[result] = 0;
      nextData = nextData + returnMessage;
    }

    //loop through current data while we can
    while (nextData.find("\r\n") != std::string::npos)
    {
      //grab the current line
      currentData = nextData.substr(0, nextData.find("\r\n") + 2);

      //remove from queue
      nextData.erase(0, nextData.find("\r\n") + 2);

      //verb message output
      V_MESSAGE(currentData.c_str());

      if(currentData.find("Password mismatch") != std::string::npos)
      {
        std::cout << "Server did not accept password: '" << password << "'." << std::endl;
        breakOut = true;
        break;
      }

      //handle message, get response
      IRC_RESPONSE response = HandleServerMessage(currentData);

      //handle response
      switch (response)
      {
      case RSP_CONTINUE:
        continue;
        break;
      case RSP_RET_TRUE:
        return true;
        break;
      case RSP_RET_FALSE:
        return false;
        break;
      case RSP_BREAK:
        result = -1;
        breakOut = true;
        break;
      case RSP_IGNORE:
      default:
        break;
      }

      //break out
      if (breakOut)
        break;
    }

    //break out
    if (breakOut)
      break;

  } while (result > 0 || nextData.size() > 0);

  //if we broke out, it means result was -1 and something bad happened
  return false;
}

//disconnect
bool IRC_Client::Disconnect()
{
  SendTCP(ClientSocket_, "QUIT \r\n", strlen("QUIT \r\n"), ServerAddress_);
  return false;
}

//main update loop
void IRC_Client::Run()
{
  //create input loop
  pthread_t threadID;
  pthread_create(&threadID, 0, IRC_Client::_inputThread, this);

  //prepare for incoming messages
  char returnMessage[1024];
  int length = 1024 - 1;
  int result;
  bool breakOut = false;

  std::string currentData;
  std::string nextData;

  //attempt to get the messages from the return
  do
  {
    result = ReceiveTCP(ClientSocket_, returnMessage, length);

    if(Close_)
      break;

    //if we recieved data, add it to the queue
    if (result > 0)
    {
      returnMessage[result] = 0;
      nextData = nextData + returnMessage;
    }
    else
    {
      Close_ = true;
      std::cout << "Failed to receive packet during main loop!" << std::endl;
      break;
    }

    //leep through current data while we can
    while (nextData.find("\r\n") != std::string::npos)
    {
      if(Close_)
      {
        breakOut = true;
        break;
      }

      //grab the current line
      currentData = nextData.substr(0, nextData.find("\r\n") + 2);

      //remove from queue
      nextData.erase(0, nextData.find("\r\n") + 2);

      V_MESSAGE(currentData.c_str());

      //handle message, get response
      IRC_RESPONSE response = HandleServerMessage(currentData);

      //handle response
      switch (response)
      {
      case RSP_CONTINUE:
        continue;
        break;
      case RSP_BREAK:
        result = -1;
        breakOut = true;
        break;
      case RSP_IGNORE:
      default:
        break;
      }

      //break out
      if (breakOut)
        break;
    }

    //break out
    if (breakOut)
      break;

  } while (true);

  //wait for input thread to close
  Close_ = true;
  pthread_join(threadID, NULL);

  Disconnect();
}

void IRC_Client::Join(const std::string channel, bool hasPound)
{
  std::string Channel_;

  //don't try to join it again
  if(ChannelMap_[channel] == true)
  {
    std::cout << "You can't join a channel you are already in.\r\n";
    return;
  }

  //add a pound, if needed
  if(!hasPound)
    Channel_ = std::string("#") + channel;
  else
    Channel_ = channel;

  //set index in map to true
  ChannelMap_[Channel_] = true;

  //send join message to the new channel
  SendMessage(Nickname_, "JOIN", Channel_, std::string("\r\n"));
}

void IRC_Client::Part(const std::string channel)
{
  //don't try to leave if you're not in already
  if(ChannelMap_[channel] == false)
  {
    std::cout << "You can't part from a channel you aren't in.\r\n";
    return;
  }

  //set channel value
  ChannelMap_[channel] = false;

  //send message
  SendMessage(Nickname_, "PART", channel, std::string("\r\n"));
}

/////////////////////////////////////////////////////////////////////
// PRIVATE METHODS //////////////////////////////////////////////////
bool IRC_Client::HandleUserInput(std::string message)
{
  //no slash? just a message
  if(message[0] != '/')
  {
    //if message is empty
    if(message.size() == 0)
    {
      std::cout << "\r";
    }
    else
    {
      //iterate through all channels we have connected to
      std::map<std::string, bool>::iterator it;
      for(it = ChannelMap_.begin(); it != ChannelMap_.end(); ++it)
      {
        //if true (currently in that channel)
        if(it->second)
        {
          //send a message to that specific channel
          SendMessage("", "PRIVMSG", it->first, message);
        }
      }

      //output user tex to screen
      std::cout << "[you] " << message << std::endl;
    }
  }
  else
  {
    V_MESSAGE((std::string("USER INPUT - ") + message).c_str());
    //for spacing
    message += " ";

    //get command
    std::string command = message.substr(1, message.find(" ") - 1);

    //get parameters
    std::vector<std::string> parameters;
    int walker = message.find(" ") + 1;
    int searcher = -1;
    do
    {
      searcher = message.find(" ", walker);
      if (searcher != std::string::npos)
      {
        parameters.push_back(message.substr(walker, searcher - walker));
        walker = searcher + 1;
      }
      else
        break;
    } while (1);

    // now we have all the parameters in a nice vector
    //QUIT MESSAGE
    if(command == "quit")
    {
      //have everything else close
      Close_ = true;

      //send message to server
      SendMessage("", "QUIT", "", "");

      //return false to chat thread kills itself
      return false;
    }
    //JOIN MESSAGE
    else if(command == "join")
    {
      if(parameters[0][0] == '#')
        SendMessage("", "JOIN", parameters[0], "");
      else
        std::cout << "Improper channel name! (" << parameters[0] << ")" << std::endl;
    }
    //PART MESSAGE
    else if(command == "part")
    {
      if(parameters[0][0] == '#')
        SendMessage("", "PART", parameters[0], "");
      else
        std::cout << "Improper channel name! (" << parameters[0] << ")" << std::endl;
    }
    //TELL MESSAGE
    else if(command == "tell")
    {
      if(parameters.size() < 2)
        std::cout << "2 parameters needed for /tell! (/tell nickname message)" << std::endl;
      else
      {
        std::string msg;
        for(int x = 1; x < parameters.size(); ++x)
          msg += parameters[x] + " ";

        SendMessage("", "PRIVMSG", parameters[0], msg);

        std::cout << "[you] whisper to " << parameters[0] << ": " << msg << std::endl;
      }
    }
    else
      std::cout<<"Invalid Command!" << std::endl;
  }

  return true;
}

//handling any message from server
IRC_RESPONSE IRC_Client::HandleServerMessage(std::string message)
{
  std::string prefix, command, trail;
  std::vector<std::string> parameters;
  static bool LoadingNames = false;
  static std::vector<std::string> NameList;

  //get all the info
  ParseServerMessage(message, prefix, command, trail, parameters);

  // COMMANDS ///////////////////////////////////////////////////////
  START_COMMANDS(command)

  //general commands ///////////////////////
    //all done
  COMMAND(PING)             //ping from server
    CMD_Ping(trail);
    return RSP_CONTINUE;
  COMMAND(PRIVMSG)          //message
    //is it address to us, or the room?
    //if a #, its a channel message
    if(parameters[0][0] == '#')
    {
      std::cout << "[" << prefix << "] " << trail;
    }
    //else it's a private message
    else
    {
      std::cout << prefix << " whispers to you: " << trail;
    }
    return RSP_CONTINUE;
  COMMAND(JOIN)             //user joins the room
    if(prefix == "[you]")
      std::cout << prefix << " have joined the channel.\r\n";
    else
      std::cout << prefix << " has joined the channel.\r\n";
    return RSP_CONTINUE;
  COMMAND(PART)             //user leaves the room
    if(prefix == "[you]")
      std::cout << prefix << " have left the channel.\r\n";
    else
      std::cout << prefix << " has left the channel.\r\n";
    return RSP_CONTINUE;
  COMMAND(QUIT)
    if(prefix == "[you]")
      std::cout << prefix << " have left the channel.\r\n";
    else
      std::cout << prefix << " has left the channel.\r\n";
    return RSP_CONTINUE;

  //////////////////////////////////////////
  //numerical commands

  // msg of the day ////////////////////////
    //all done
  COMMAND(375)  //start, don't output this
    //std::cout << trail;
    return RSP_CONTINUE;
  COMMAND(372)  //content
    std::cout << trail;
  return RSP_CONTINUE;
  COMMAND(376)  //end
    return RSP_RET_TRUE;
  // users /////////////////////////////////
    //not done
  COMMAND(353)              //listing a user, start storing names
    //clear if first message
    if(!LoadingNames)
    {
      LoadingNames = true;
      NameList.clear();
    }
    //remove /r/n
    trail.erase(trail.size() - 2, 2);
    trail += " ";
    //iterate through trail, get each name by whitespace. @ means the last one
    int walker = 0;
    int searcher = -1;

    do
    {
      searcher = trail.find(" ", walker);
      if (searcher != std::string::npos || searcher == trail.size())
      {
        std::string user = trail.substr(walker, searcher - walker);
        NameList.push_back(user);
        walker = searcher + 1;
      }
      else
        break;
    } while (1);

    return RSP_CONTINUE;
  COMMAND(366)              //end of user list, print them out
    LoadingNames = false;
    std::cout << "Users in " << parameters[1] << ": ";

    //print off each name
    for(int x = 0; x < NameList.size(); ++x)
    {
      //if own name
      if(NameList[x] == Nickname_ || NameList[x] == ("@" + Nickname_))
        NameList[x] = "[you]";

      //if last name
      if(x == NameList.size() - 1)
      {
        NameList[x].erase(0, 1);
        std::cout << NameList[x];
      }
      //else, regular name
      else
      {
        if(x != 0)
          std::cout << ", " << NameList[x];
        else
          std::cout << NameList[x];
      }
    }

    //end it
    std::cout << "." << std::endl << "\r";
    return RSP_CONTINUE;
  END_COMMANDS()

  return RSP_IGNORE;
}

void IRC_Client::SendMessage(std::string message)
{
  SendTCP(ClientSocket_, const_cast<char*>(message.c_str()), message.size(), ServerAddress_);
}

void IRC_Client::SendMessage(std::string prefix, std::string command, std::string parameters, std::string trail)
{
  std::string message;

  if (prefix.size() > 0)
    prefix = ":" + prefix + " ";

  if (command.size() > 0)
    command = command + " ";

  if (parameters.size() > 0)
    parameters = parameters + " ";

  if (trail.size() > 0)
    trail = ":" + trail;

  message = prefix + command + parameters + trail + "\r\n";

  //this is not recursive
  SendMessage(message);
}

///////////////////////////////////////////////////////////////////
// UTILITIES //////////////////////////////////////////////////////
std::string IRC_Client::GetNickname(std::string prefix)
{
  int at = prefix.find("!");

  if (at == std::string::npos)
    return prefix;

  std::string nickname = prefix.substr(0, at);

  if(nickname == Nickname_)
    nickname = "[you]";

  return nickname;
}

void IRC_Client::ParseServerMessage(std::string message, std::string &prefix, std::string &command, std::string &end, std::vector<std::string> &parameters)
{
  int prefixEnd = -1;
  int trailingBegin = -1;
  int paramBegin = -1;

  // PARSING INPUT //////////////////////////////////////////////////
  //we need to chop up the message into 3 parts:
  //1. prefix, if it exists
  if (message[0] == ':')
  {
    prefixEnd = message.find(" ");
    prefix = message.substr(1, prefixEnd - 1);

    prefix = GetNickname(prefix);
  }
  else
    prefixEnd = -1;

  //2. get the ending, if it exists
  trailingBegin = message.find(" :");
  if (trailingBegin != std::string::npos)
  {
    end = message.substr(trailingBegin + 2, message.size() - trailingBegin);
  }
  else
    trailingBegin = message.size();

  //3. get the command
  paramBegin = message.find(" ", prefixEnd + 1);
  command = message.substr(prefixEnd + 1, paramBegin - prefixEnd - 1);

  //4. get the parameters
  int walker = paramBegin + 1;
  int searcher = -1;
  do
  {
    searcher = message.find(" ", walker);
    if (searcher != std::string::npos && searcher <= trailingBegin)
    {
      parameters.push_back(message.substr(walker, searcher - walker));
      walker = searcher + 1;
    }
    else
      break;
  } while (1);
}

///////////////////////////////////////////////////////////////////
// COMMANDS ///////////////////////////////////////////////////////
void IRC_Client::CMD_Ping(std::string input)
{
  SendMessage("", "PONG", "", input);
}

// USER INPUT /////////////////////////////////////////////////////
void *IRC_Client::_inputThread(void *input)
{
  IRC_Client *client = reinterpret_cast<IRC_Client*>(input);
  std::string line;

  termios tp, save;
  tcgetattr(STDIN_FILENO, &tp);
  save = tp;

  tp.c_lflag &= ~ECHONL;
  tp.c_lflag &=
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp);

  while(1)
  {
    if(client->Close_)
      return 0;
    char c = fgetc(stdin);

    if(c != '\n' && c != '\r')
    {
      line += c;
    }
    else
    {
      //reset line
      std::cout << "\r";
      std::cout << "\033[1A";

      if(!client->HandleUserInput(line))
      {
        //reset console settings
        tcsetattr(STDIN_FILENO, TCSANOW, &save);

        //disconnect
        client->Disconnect();

        break;
      }
      line.clear();
    }
  }

  return 0;
}
