#include "Server.h"
#include <exception>
#include <string>
#include <fstream>
#include <queue>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>


std::mutex clientListLock;
std::mutex fileLock;
std::mutex msgLock;
std::condition_variable cond;

std::vector<std::thread*> clients_Thread;
std::map<std::string, SOCKET>clients_List;
std::queue<std::string> clients_Msgs;

void clientHandler(SOCKET clientSocket);
void saveMsg();
std::string logginHandler(SOCKET clientSocket);
void clientUpdate(SOCKET clientSocket, std::string userName);
std::string readFromFile(std::string fromUser, std::string toUser);

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
	std::string userName = "";
	try
	{
		while (true)
		{
			typeCode = Helper::getMessageTypeCode(clientSocket);

			switch (typeCode)
			{
			case 200:	// Login msg.
				userName = logginHandler(clientSocket);
				break;

			case 204:	// Client update msg.
				clientUpdate(clientSocket, userName);
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
		clients_List.erase(userName);
		std::cout << userName << " disconnected!" << std::endl;
	}
}

/*
	The function handle with the loggin request.
	return:
		userName - the new username that was loggin.
*/
std::string logginHandler(SOCKET clientSocket)
{
	int nameBytes = 0;
	std::string userName;

	std::string userNames = "";

	// Get user name from socket.
	nameBytes = Helper::getIntPartFromSocket(clientSocket, 2);
	userName = Helper::getStringPartFromSocket(clientSocket, nameBytes);

	// lock the Clients list and add the new client.
	std::unique_lock<std::mutex> locker(clientListLock);
	clients_List.insert({ userName, clientSocket });
	locker.unlock();
	
	std::cout << "ADDED new client! - " << userName << std::endl;

	// Make the All_usernames
	// Dont add the last name. (wont write '&' after no name)
	std::map<std::string, SOCKET>::iterator it = clients_List.begin();
	for (int i = 0; i < clients_List.size() - 1; i++)
	{
		userNames += it->first;
		userNames += "&";
		it++;
	}
	userNames += it->first; // Add the last name.

	// Send update msg to client.
	Helper::send_update_message_to_client(clientSocket, "", "", userNames);
	
	return userName;
}

/*
	The function handle with the users communication. (send msg between users)
*/
void clientUpdate(SOCKET clientSocket, std::string userName)
{
	// Get length of username to send
	int userBytes = Helper::getIntPartFromSocket(clientSocket, 2);

	// Get username to send
	std::string sendToUser = Helper::getStringPartFromSocket(clientSocket, userBytes);

	// Get Len of msg to send.
	int msgBytes = Helper::getIntPartFromSocket(clientSocket, 5);

	// Get msg to send
	std::string msgToSend = Helper::getStringPartFromSocket(clientSocket, msgBytes);
	
	// if user wasnt found.
	if (clients_List.find(sendToUser) == clients_List.end()) 
	{
		sendToUser = "";
		msgToSend = "";
	}

	// Make the All_usernames
	std::string userNames = "";
	std::map<std::string, SOCKET>::iterator it = clients_List.begin();

	// Dont add the last name. (wont write '&' after no name)
	for (int i = 0; i < clients_List.size() - 1; i++)
	{
		userNames += it->first;
		userNames += "&";
		it++;
	}
	userNames += it->first; // Add the last name.

	// If the message to send to is valid, add it to the queue.
	if (msgToSend != "")
	{
		std::string msg = userName + "&" + msgToSend + "&" + sendToUser; // Create the message that will be in the file.
		std::unique_lock<std::mutex> locker(msgLock);
		clients_Msgs.push(msg);
		locker.unlock();
		cond.notify_one();

		// If there is no problem with userName or msg, get the history from file. 
		msgToSend = readFromFile(userName, sendToUser);
	}

	// Send update msg to client.
	Helper::send_update_message_to_client(clientSocket, msgToSend, sendToUser, userNames);

}

/*
	The funtion save the messages to files.
*/
void saveMsg()
{
	// file things
	std::ofstream file;
	std::string fileName, finishMsg;

	// msg things
	std::string totalMsg, fromUser, msg, toUser;

	while (true)
	{
		std::unique_lock<std::mutex> locker(msgLock);
		cond.wait(locker, []() { return !clients_Msgs.empty(); }); // if the queue is empry return to sleep.

		totalMsg = clients_Msgs.front(); // Get the last message
		// Get user to send to
		toUser = totalMsg.substr(totalMsg.find_last_of("&") + 1);	
		// Get user that send the msg
		fromUser = totalMsg.substr(0, totalMsg.find_first_of("&"));	
		// Get message length
		int msg_length = totalMsg.length() - toUser.length() - fromUser.length() - 2;
		// Get the message
		msg = totalMsg.substr(totalMsg.find_first_of("&") + 1, msg_length);
		
		// check who is bigger for the file name
		if (fromUser < toUser)
		{
			fileName = fromUser + "&" + toUser + ".txt";
		}
		else
		{
			fileName = toUser + "&" + fromUser + ".txt";
		}

		// Create the msg to save.
		finishMsg += "&MAGSH_MESSAGE&&Author&" + fromUser + "&DATA&" + msg;

		// Write to the file the msg.
		std::unique_lock<std::mutex> file_Locker(fileLock);
		file.open(fileName, std::ios::app);
		if (file.is_open())
		{
			file << finishMsg;
			file.close();
		}
		file_Locker.unlock();

		// Delete the last message.
		clients_Msgs.pop();
	}
}

/*
	The function read from the file the chat "history".
*/
std::string readFromFile(std::string fromUser, std::string toUser)
{
	std::ifstream file;
	std::string fileName, msgs, msg;

	// Get the file name.
	if (fromUser < toUser)
	{
		fileName = fromUser + "&" + toUser + ".txt";
	}
	else
	{
		fileName = toUser + "&" + fromUser + ".txt";
	}

	// Open the file and get all the messages between the users.
	std::unique_lock<std::mutex> file_Locker(fileLock);
	file.open(fileName, std::ios::in);
	if (file.is_open())
	{
		while (std::getline(file, msg))
		{
			msgs += msg;
		}
	}
	file_Locker.unlock();

	return msgs;
}