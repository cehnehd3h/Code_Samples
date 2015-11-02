# Macros ========================================

# Change compiler path to match your system
CC=g++
CFLAGS= -Wno-write-strings 
OBJECTS=main.o TCPCore.o IRCClient.o
EXEFILE=simplechat.sh
ERASE=rm

# Targets ========================================

$(EXEFILE) : $(OBJECTS)
	$(CC) -o $(EXEFILE) $(OBJECTS) -pthread

main.o : main.cpp IRCClient.h
	$(CC) -c -o main.o $(CFLAGS) main.cpp 

TCPCore.o : TCPCore.cpp TCPCore.h
	$(CC) -c -o TCPCore.o $(CFLAGS) TCPCore.cpp

IRCClient.o : IRCClient.cpp IRCClient.h TCPCore.h
	$(CC) -c -o IRCClient.o $(CFLAGS) IRCClient.cpp

all : 
	$(MAKE) $(EXEFILE)

rebuild : 
	$(MAKE) clean
	$(MAKE) all

clean :
	rm $(EXEFILE) $(OBJECTS)