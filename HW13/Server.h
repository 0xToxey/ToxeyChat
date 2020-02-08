#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <mutex>

class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	void accept();
	friend void clientHandler(SOCKET clientSocket);

	SOCKET _serverSocket;
	std::vector<std::thread*> clients_thread;
	std::map<std::string, SOCKET>clients_list;

	std::mutex clientListLock;
};

