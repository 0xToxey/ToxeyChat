#define NOMINMAX

#include "Server.h"
#include <exception>
#include <string>
#include <fstream>
#include <filesystem>

Server::Server()
{
	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
	if (bind(_serverSocket, (struct sockaddr*) & sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	std::cout << "binded" << std::endl;

	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "listening on port " << port << std::endl;

	// Create thread that handle with client messages.
	std::thread(&Server::saveMsg, this).detach();

	// Accepting clients.
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

	std::cout << "Client accepted ! " << std::endl;

	// Make a thread for the client and add it to the clients Threads.
	std::thread(&Server::clientHandler, this, client_socket).detach();
}


/*
	The function handle with new client
*/
void Server::clientHandler(SOCKET clientSocket)
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
			case MT_CLIENT_LOG_IN:	// Login msg.
				userName = logginHandler(clientSocket);
				break;

			case MT_CLIENT_UPDATE:	// Client update msg.
				clientUpdate(clientSocket, userName);
				break;

			default:
				break;
			}
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		closesocket(clientSocket);
		this->_clients_List.erase(userName);
		std::cout << userName << " disconnected!" << std::endl;
	}
}


/*
	The function handle with the loggin request.
	return:
		userName - the new username that was loggin.
*/
std::string Server::logginHandler(SOCKET clientSocket)
{
	int nameBytes = 0;
	std::string userName, userNames;

	// Get user name from socket.
	nameBytes = Helper::getIntPartFromSocket(clientSocket, 2);
	userName = Helper::getStringPartFromSocket(clientSocket, nameBytes);

<<<<<<< HEAD
=======
	passBytes = Helper::getIntPartFromSocket(clientSocket, 2);
	password = Helper::getStringPartFromSocket(clientSocket, passBytes);

	bool loggin = tryLoggin(userName, password);
	if (!loggin)
	{
		Helper::send_update_message_to_client(clientSocket, "", "", "Wrong|Pass");
		return "";
	}

>>>>>>> loggin
	// lock the Clients list and add the new client.
	std::unique_lock<std::mutex> locker(this->_clientListLock);
	this->_clients_List.insert({ userName, clientSocket });
	locker.unlock();

	std::cout << "ADDED new client! - " << userName << std::endl;

	// Make the All_usernames
	// Dont add the last name. (wont write '&' after no name)
	userNames = getUserNameList();

	// Send update msg to client.
	Helper::send_update_message_to_client(clientSocket, "", "", userNames);

	return userName;
}


/*
	The function handle with the users communication. (send msg between users)
*/
void Server::clientUpdate(SOCKET clientSocket, std::string userName)
{
	// Get username to send to from client.
	int userBytes = Helper::getIntPartFromSocket(clientSocket, 2);
	std::string sendToUser = Helper::getStringPartFromSocket(clientSocket, userBytes);

	// Get msg to send.
	int msgBytes = Helper::getIntPartFromSocket(clientSocket, 5);
	std::string msgToSend = Helper::getStringPartFromSocket(clientSocket, msgBytes);

	// if user wasnt found.
	if (this->_clients_List.find(sendToUser) == this->_clients_List.end())
	{
		sendToUser = "";
		msgToSend = "";
	}

	// Make the All_usernames
	std::string userNames = getUserNameList();

	// If the message to send to is valid, add it to the queue.
	if (!msgToSend.empty())
	{
		// Get file name.
		std::string fileName = std::min(userName, sendToUser) + "&" + std::max(userName, sendToUser) + ".txt";

		// Create the message to save in the file.
		std::string msg = "&MAGSH_MESSAGE&&Author&" + userName + "&DATA&" + msgToSend;

		// Lock and push to the queue of masseges.
		std::unique_lock<std::mutex> locker(this->_msgLock);
		this->_clients_Msgs.push(std::pair<std::string, std::string>({ fileName, msg }));
		locker.unlock();
		this->_cond.notify_one();
	}

	if (!sendToUser.empty())
	{
		msgToSend = readFromFile(userName, sendToUser);
	}

	// Send update msg to client.
	Helper::send_update_message_to_client(clientSocket, msgToSend, sendToUser, userNames);
}


/*
	The funtion save the messages to files.
*/
void Server::saveMsg()
{
	// file things
	std::ofstream file;
	std::string fileName;

	// msg things.
	std::pair<std::string, std::string> name_msg;
	std::string msg;

	while (true)
	{
		std::unique_lock<std::mutex> locker(this->_msgLock);
		this->_cond.wait(locker); // if the queue is empry return to sleep.

		name_msg = this->_clients_Msgs.front(); // Get the last message
		fileName = name_msg.first;
		msg = name_msg.second;

		// Write to the file the msg.
		std::unique_lock<std::mutex> file_Locker(this->_fileLock);
		file.open(fileName, std::ios::app);
		if (file.is_open())
		{
			file << msg;
			file.close();
		}
		file_Locker.unlock();

		// Delete the last message.
		this->_clients_Msgs.pop();
	}
}


/*
	The function read from the file the chat "history".
*/
std::string Server::readFromFile(std::string fromUser, std::string toUser)
{
	std::string fileName = std::min(fromUser, toUser) + "&" + std::max(fromUser, toUser) + ".txt";
	std::string msgs;

	// Open the file and get all the messages between the users.
	std::unique_lock<std::mutex> file_Locker(this->_fileLock);

	std::ifstream file(fileName);

	if (file.is_open())
	{
		msgs = std::string((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
	}
	file_Locker.unlock();

	return msgs;
}


/*
	The function return the list of userNames.
*/
std::string Server::getUserNameList()
{
	std::string userNames = "";

	for (const auto& client : this->_clients_List)
	{
		userNames += client.first + "&";
	}
	userNames = userNames.substr(0, userNames.size() - 1);

	return userNames;
}
<<<<<<< HEAD
=======


/*
	The function check if the user can loggin.
*/
bool Server::tryLoggin(std::string userName, std::string password)
{
	bool successLoggin = false;
	std::string user, pass;

	// checking if the file exists.
	std::unique_lock<std::mutex> dataLocker(this->_usersDataFile);
	if (!std::ifstream("userData.txt").good())
	{
		dataLocker.unlock();
		this->addNewUser(userName, password);
		return true;
	}

	std::ifstream usersFile("usersData.txt");

	if (!usersFile.is_open()) // If the file was not open, quit.
	{
		return false; 
	}

	// get data from file into map of username & pass.
	while (std::getline(usersFile, pass))
	{
		// Get user and pass (format: username|password )
		user = pass.substr(0, pass.find("|"));
		pass = pass.substr(pass.find("|") + 1);

		if (userName == user) // If user was found.
		{
			if (pass == password) // If pass match.
			{
				return true;
			}

			return false;
		}
	}

	usersFile.close();
	dataLocker.unlock();
	return addNewUser(userName, password); // Add new user.
}


/*
	The function add new user to the app.
*/
bool Server::addNewUser(std::string userName, std::string password)
{
	std::ofstream usersFile;
	std::string newUser = userName + "|" + password + "\n";

	std::unique_lock<std::mutex> usersDataLocker(this->_usersDataFile);
	usersFile.open("usersData.txt", std::ios::app);
	if (!usersFile.is_open())
	{
		return false;
	}

	usersFile << newUser;
	usersFile.close();
	usersDataLocker.unlock();
	return true;
}
>>>>>>> loggin
