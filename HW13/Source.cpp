//#pragma comment (lib, "ws2_32.lib")
//
//#include "WSAInitializer.h"
//#include "Server.h"
//#include <iostream>
//#include <exception>
//#include <fstream>
//#include <string>
//
//int main()
//{
//	try
//	{
//		WSAInitializer wsaInit;
//		Server myServer;
//
//		// Get configorations
//		std::ifstream configFile("..//config.txt");
//		
//		if (configFile.is_open())
//		{
//			std::string ip;
//			std::string port;
//			getline(configFile, ip);
//			getline(configFile, port);
//
//			std::cout << "Starting... " << std::endl;
//
//			// Start server.
//			myServer.serve(std::stoi(port));
//		}
//		else // if config file dosent exist.
//		{
//			throw std::exception("config file wasnt found!");
//		}
//	}
//	catch (std::exception& e)
//	{
//		std::cerr << "Error occured: " << e.what() << std::endl;
//	}
//
//	system("PAUSE");
//	return 0;
//}