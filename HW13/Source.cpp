#pragma comment (lib, "ws2_32.lib")

#include "WSAInitializer.h"
#include "Server.h"
#include <iostream>
#include <exception>
#include <fstream>

int main()
{
	try
	{
		WSAInitializer wsaInit;
		Server myServer;
		std::fstream configFile;
		configFile.open("..//config.txt");
		std::string ip;
		std::string port;

		getline(configFile, ip);
		getline(configFile, port);


		myServer.serve(8876);
	}
	catch (std::exception& e)
	{
		std::cout << "Error occured: " << e.what() << std::endl;
	}
	system("PAUSE");
	return 0;
}