#include "Server.h"
#include <exception>
#include <string>
#include <fstream>

std::mutex clientListLock;
std::mutex msgLock;

std::vector<std::thread*> clients_Thread;
std::map<std::string, SOCKET>clients_List;
std::queue<std::string> client_Msgs;

void clientHandler(SOCKET clientSocket);
void saveMsg();
void logginHandler(SOCKET clientSocket);

Server::Server()
{
	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET,  SOCK_STREAM,  IPPROTO_TCP); 

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - socket");
}

Server::~Server()
{
	try
	{
		closesocket(_serverSocket);
	}
	catch (...) {}
}

void Server::serve(int port)
{
	struct sockaddr_in sa = { 0 };
	
	sa.sin_port = htons(port);			// port that server will listen for
	sa.sin_family = AF_INET;			// must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	std::cout << "binded" << std::endl;

	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "listening on port " << port << std::endl;

	// Create thread that handle with client messages.
	std::thread msgHandle(saveMsg);

	// Accepting clients.
	while (true)
	{
		std::cout << "accepting clients..." << std::endl;
		accept();
	}

	// Delete clients threads.
	for (auto& clientThread : clients_Thread)
	{
		clientThread->join();
		delete clientThread;
	}
	msgHandle.join();
}

void Server::accept()
{
	// this accepts the client and create a specific socket from server to this client
	SOCKET client_socket = ::accept(_serverSocket, NULL, NULL);

	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted ! " << std::endl;

	// Make a thread for the client and add it to the clients Threads.
	std::thread* client = new std::thread(clientHandler, client_socket);
	clients_Thread.push_back(client);
}

/*
	The function handle with new client
*/
void clientHandler(SOCKET clientSocket)
{
	int typeCode = 0;
	try
	{
		while (true)
		{
			typeCode = Helper::getMessageTypeCode(clientSocket);

			switch (typeCode)
			{
			case 200:	// Login msg.
				logginHandler(clientSocket);
				break;

			case 204:	// Client update msg.

				break;
		
			default:
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		closesocket(clientSocket);
	}
}

/*
	The function handle with the loggin request.
*/
void logginHandler(SOCKET clientSocket)
{
	int nameBytes = 0;
	std::string userName;

	std::string userNames = "";

	// Get user name from socket.
	nameBytes = Helper::getIntPartFromSocket(clientSocket, 2);
	userName = Helper::getStringPartFromSocket(clientSocket, nameBytes);

	// lock the Clients list and add the new client.
	std::unique_lock<std::mutex> lock(clientListLock);
	clients_List.insert({ userName, clientSocket });
	lock.unlock();
	
	std::cout << "ADDED new client! - " << userName << std::endl;

	// Make the All_usernames
	if (clients_List.size() > 1)
	{
		for (auto& user : clients_List)
		{
			userNames += user.first;
			userNames += "&";
		}
	}
	else
	{
		userNames += userName;
	}

	// Send update msg to client.
	Helper::send_update_message_to_client(clientSocket, "", "", userNames);
}

/*
	The funtion save the messages to files.
*/
void saveMsg()
{
	std::fstream file;
}