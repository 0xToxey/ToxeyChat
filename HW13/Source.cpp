#pragma comment (lib, "ws2_32.lib")

#include "WSAInitializer.h"
#include "Server.h"
#include <iostream>
#include <exception>
#include <fstream>
#include <string>

int main()
{
	try
	{
		WSAInitializer wsaInit;
		Server myServer;

		// Get configorations
		std::ifstream configFile("..//config.txt");
		std::string ip;
		std::string port;
		getline(configFile, ip);
		getline(configFile, port);

		myServer.serve(std::stoi(port));
	}
	catch (std::exception& e)
	{
		std::cerr << "Error occured: " << e.what() << std::endl;
	}
	system("PAUSE");
	return 0;
}