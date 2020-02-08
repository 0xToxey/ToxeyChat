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
	
	friend void clientHandler(SOCKET clientSocket);
	friend void saveMsg();
	friend void logginHandler(SOCKET clientSocket);

	SOCKET _serverSocket;
};