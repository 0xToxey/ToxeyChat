#include "Server.h"
#include <exception>
#include <string>

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
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);
	}
	catch (...) {}
}

void Server::serve(int port)
{
	struct sockaddr_in sa = { 0 };
	
	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// again stepping out to the global namespace
	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	std::cout << "binded" << std::endl;

	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "listening on port " << port << std::endl;

	while (true)
	{
		std::cout << "accepting clients..." << std::endl;
		accept();
	}
}


void Server::accept()
{
	// this accepts the client and create a specific socket from server to this client
	SOCKET client_socket = ::accept(_serverSocket, NULL, NULL);

	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted! " << std::endl;

	// Make a thread for the client and add it to the clients.
	std::thread* client = new std::thread(clientHandler, client_socket);

	this->clients_thread.push_back(client);
}


void clientHandler(SOCKET clientSocket)
{
	try
	{


		//std::string s = "Welcome! What is your name (4 bytes)? ";
		//send(clientSocket, s.c_str(), s.size(), 0);  // last parameter: flag. for us will be 0.

		//char m[5];
		//recv(clientSocket, m, 4, 0);
		//m[4] = 0;
		//std::cout << "Client name is: " << m << std::endl;

		//s = "Bye";
		//send(clientSocket, s.c_str(), s.size(), 0);

		//// Closing the socket (in the level of the TCP protocol)
		//closesocket(clientSocket);
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
	}
}