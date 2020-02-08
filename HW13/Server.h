#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include "Helper.h"

class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	void accept();
	
	friend void clientHandler(SOCKET clientSocket);
	friend void saveMsg();
	friend std::string logginHandler(SOCKET clientSocket);
	friend void clientUpdate(SOCKET clientSocket, std::string userName);

	SOCKET _serverSocket;
};