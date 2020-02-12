#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "Helper.h"

class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	void accept();

	void clientHandler(SOCKET clientSocket);
	void saveMsg();
	std::string logginHandler(SOCKET clientSocket);
	void clientUpdate(SOCKET clientSocket, std::string userName);
	std::string readFromFile(std::string fromUser, std::string toUser);
	std::string getUserNameList();

	std::mutex _clientListLock;
	std::mutex _fileLock;
	std::mutex _msgLock;
	std::condition_variable _cond;


	SOCKET _serverSocket;
	std::map<std::string, SOCKET> _clients_List;
	std::queue<std::pair<std::string, std::string>> _clients_Msgs;
};
