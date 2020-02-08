#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <mutex>
#include <queue>
#include "Helper.h"

class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	void accept();
	
	friend void clientHandler(SOCKET clientSocket, Server selfClass);
	friend void saveMsg(Server selfClass);
	friend void logginHandler(SOCKET clientSocket, Server selfClass);

	SOCKET _serverSocket;
	std::vector<std::thread*> clients_Thread;
	std::map<std::string, SOCKET>clients_List;
	std::queue<std::string> client_Msgs;
};

