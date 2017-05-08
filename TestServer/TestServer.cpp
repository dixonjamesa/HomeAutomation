#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <iostream>
#include <thread>

using namespace std;

void jlisten();

void * get_in_addr(struct sockaddr * addr)
{
    if(addr->sa_family == AF_INET)
    {
	return &(((struct sockaddr_in *)addr)->sin_addr);
    }
    return &(((sockaddr_in6 *)addr)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int i;
	thread t1(jlisten);

	for(i=0;i<100;i++)
	{
		printf("Doing stuff %d\n", i);
		sleep(1);
	}
	t1.join();
}


void jlisten()
{
	int listenfd = 0, connfd = 0;
    	struct sockaddr_in serv_addr; 
   	struct sockaddr_storage client_addr;

	char sendBuff[1024];
	char content[2048];
	char parsebuf[1024];
	char readbuf[4096];
	time_t ticks;
	struct tm tm;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 

    char s[INET6_ADDRSTRLEN];
    socklen_t addr_size = sizeof(client_addr);

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)&client_addr, &addr_size); 

	inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*)&client_addr), s, sizeof(s));
	cout << "Connection made to " << s << "...";

	read(connfd, readbuf, 4096);

        ticks = time(0);
	tm = *gmtime(&ticks);

	sprintf(content, "<!DOCTYPE html\">");
	strcat(content, "<html><head><title>Test Page</title></head><body>");
	strcat(content, "Hello World");
	strcat(content, readbuf);
        strftime(timebuf, sizeof(timebuf), "Date:%a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
	strcat(content, timebuf);
	strcat(content, "</body></html>");


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
        snprintf(sendBuff, sizeof(sendBuff), "Content-Length: %d\r\n", strlen(content));
        write(connfd, sendBuff, strlen(sendBuff));
        snprintf(sendBuff, sizeof(sendBuff), "Content-Type: text/html; charset=UTF-8\r\n\r\n");
        write(connfd, sendBuff, strlen(sendBuff));

        write(connfd, content, strlen(content));


	cerr << "Content sent...";
	shutdown(connfd, SHUT_WR);
        close(connfd);
	cerr << "Closed\r\n";
        sleep(1);
     }
}
