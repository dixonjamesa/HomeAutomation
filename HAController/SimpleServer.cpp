#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <mosquittopp.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <list>
#include <map>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../Libraries/HomeAutomation/HACommon.h"
#include "MessageMap.h"
#include "HomeController.h"
#include <boost/algorithm/string.hpp>

using namespace std;

extern string GenerateContents(string _url);
extern bool QuittinTime;
extern int SocketId;

// get a socket in_address
void * get_in_addr(struct sockaddr * addr)
{
    if(addr->sa_family == AF_INET)
    {
	return &(((struct sockaddr_in *)addr)->sin_addr);
    }
    return &(((sockaddr_in6 *)addr)->sin6_addr);
}


/// Listen for network traffic, and serve up some data as required
/// We are listening on _portno
void jlisten(int _portno)
{
	int connfd = 0;
    	struct sockaddr_in serv_addr; 
    	struct sockaddr_storage client_addr;

	char sendBuff[256];
	char requestBuff[4096];
	string content, req;
	time_t ticks;
	struct tm tm;
	int errcount = 0;

	// create a socket:
	SocketId = socket(AF_INET, SOCK_STREAM, 0);
	if( SocketId == -1 )
	{
		WriteLog( "Server socket could not be created, error %d\n", errno);
		return;
	}
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuff, '0', sizeof(sendBuff)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(_portno); 

	// bind to the socket
	if( bind(SocketId, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1 )
	{
		WriteLog( "Server failed to bind to socket with error %d\n", errno);
		return;
	}
	if( listen(SocketId, 5) == -1 )
	{
		WriteLog( "Server listen failed with error %d\n", errno);
		return;
	}

	char s[INET6_ADDRSTRLEN];
	socklen_t addr_size = sizeof(client_addr);

	WriteLog( "Server listening on port %d\n", _portno);
	while(!QuittinTime)
	{
		// blocking call to wait for a connection on the socket:
		connfd = accept(SocketId, (struct sockaddr*)&client_addr, &addr_size); 

		if( connfd < 0 ) // -1 is error state 
		{
			WriteLog("Error encountered on accept() call, %d\n", errno);
			if( ++errcount > 10 )
			{
				// stop trying
				return;
			}
		}
		else
		{
			string header, url;

			errcount = 0;
			// find out who connected:
			inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*)&client_addr), s, sizeof(s));
			WriteLog( "Request for info made from %s ...\n", s);
			// read in the http request data:
			if( read(connfd, requestBuff, 4096) > 0 )
			{
				// parse it:
				map<string, string> m;

				std::istringstream req(requestBuff);

				string::size_type index;

				// first line should be something like GET /info HTTP/1.1
				getline(req, url);
				index = url.find(' ', 0)+1;
				url = url.substr(index, url.find(' ', index)-index);

				// followed by : separated list of headers:
				while(getline(req, header) && header != "\r")
				{
					index = header.find(':', 0 );
					if( index != string::npos)
					{
						m.insert(make_pair(
							boost::algorithm::trim_copy(header.substr(0, index)), 
							boost::algorithm::trim_copy(header.substr(index + 1))
						));
					}
				}
			}

			ticks = time(0);
			tm = *gmtime(&ticks);

			content = GenerateContents(url);

			snprintf(sendBuff, sizeof(sendBuff), "HTTP/1.1 200 OK\r\n");
			write(connfd, sendBuff, strlen(sendBuff)); 
			snprintf(sendBuff, sizeof(sendBuff), "Server: pibrain\r\n");
			write(connfd, sendBuff, strlen(sendBuff));
			snprintf(sendBuff, sizeof(sendBuff), "Host: pibrain\r\n");
			write(connfd, sendBuff, strlen(sendBuff));
			strftime(sendBuff, sizeof(sendBuff), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
			write(connfd, sendBuff, strlen(sendBuff));
			snprintf(sendBuff, sizeof(sendBuff), "Connection: Closed\r\n");
			write(connfd, sendBuff, strlen(sendBuff));
			snprintf(sendBuff, sizeof(sendBuff), "Content-Length: %d\r\n", content.length());
			write(connfd, sendBuff, strlen(sendBuff));
			snprintf(sendBuff, sizeof(sendBuff), "Content-Type: text/html; charset=UTF-8\r\n\r\n");
			write(connfd, sendBuff, strlen(sendBuff));

			write(connfd, content.c_str(), content.length());

			//cerr << "Content sent...";
			shutdown(connfd, SHUT_WR);
			close(connfd);
			//cerr << "Closed\r\n";
		}
		sleep(1);
	}
}
