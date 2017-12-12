/*
Author: Intekhab Naser
Front-end server is an interface between clients and bank servers.
It multicasts the client requests to all bank servers and performs as Coordinator in 2 Phase Commit Protocol with back-end servers.
It also performs locking on each back-end server thread to prevent race conditions in case of concurrent client requests.
*/
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <list>

using namespace std;

#define READY 1
#define OK 2
#define COMMIT 3
#define ABORT 4
#define FAIL 5

struct newserver{								// Structure for maintaining connection information of all back-end servers
	long socket_fd;
	long port;
	struct sockaddr_in new_serv_addr;
	struct hostent *new_server;
};

void error(const char *msg) 					// Function to print error messages
{
	perror(msg);
	exit(1);
}

struct newserver p[10];
pthread_mutex_t lock;

void *NewClient(void *sockfd) 					// Thread function interface for communication between each client and back-end servers
{
	char buffer[256], tmp_buffer[256];
	int req;
	
	long *ptr = (long *)sockfd;
	long newsockfd = *ptr;
	
	if(newsockfd < 0) 
		error("ERROR on accept");
	
	string reply = "OK";						// Sending 'OK' confirmation message to the client for successful connection
	
	bzero(buffer,256);
	strcpy(buffer,reply.c_str());
	
	int rc = send(newsockfd,buffer,strlen(buffer),0);
	if (rc < 0) 
		error("ERROR writing to socket");

	while(1)
	{
		bzero(buffer,256);
		int rc = recv(newsockfd,buffer,sizeof(buffer),0);	//Read the request message from client
		if(rc < 0) 
			error("ERROR reading from socket");

		cout << "Received request from client: " << buffer << endl;

		if(strcmp(buffer,"QUIT") == 0)						// If the client sends "QUIT", thread stops looping  	
		{
			break;
		}

		struct timeval tv;
		tv.tv_sec = 0;										// 0 Secs Timeout
		tv.tv_usec = 10;									// 10 microsec Timeout

		for(int i = 0; i < 3; i++)							// Setting the send and recv timeout values for back-end server sockets
		{
			if(setsockopt(p[i].socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0)
			{
				error("Setsockopt failed on Setting Recv Timeout");
			}
		}

		pthread_mutex_lock(&lock);							// Locking the Thread between front-end and back-end servers

		for(int i = 0; i < 3; i++)							// Send the client request to all back-end servers
		{
			int rc = send(p[i].socket_fd,buffer,strlen(buffer),MSG_NOSIGNAL);
			if (rc < 0)
			{
				cout << "Send failed with rc: <" << rc << "> on server: " << i+1 << endl;
				continue;
			}
			else
				cout << "Sent msg: <" << buffer << "> to server: " << i+1 << endl;
		}

		// Two-Phase Commit Protocol Begins

		req = READY;										// Message to initiate commit READY voting
		
		for(int i = 0; i < 3; i++)							// Ask all back-end servers if they are READY for committing
		{
			int rc = send(p[i].socket_fd,&req,sizeof(int),MSG_NOSIGNAL);
			if (rc < 0)
			{
				cout << "Send failed with rc: <" << rc << "> on server: " << i+1 << endl;
				continue;
			}
			else
				cout << "Sent 'READY' msg: <" << req << "> to server: " << i+1 << endl;
		}

		int flag = 1, cnt = 0;

		for(int i = 0; i < 3; i++)							// Receive 'OK' or 'ABORT' voting responses from back-end servers
		{
			int rc = recv(p[i].socket_fd,&req,sizeof(int),0);
			if (rc <= 0)
			{
				cout << "Receive failed with rc: <" << rc << "> on server: " << i+1 << endl;
				continue;
			}
			else
			{
				cout << "Received 'OK' msg: <" << req << "> from server: " << i+1 << endl;
				
				cnt++;
				
				cout << "cnt = " << cnt << endl;
				
				if(req != OK)
					flag = 0;
			}
		}

		cout << "Out cnt = " << cnt << endl;

		if(flag == 1 && cnt >= 2)
		{
			req = COMMIT;									// Message to send COMMIT command to all back-end servers
		}
		else
		{
			req = ABORT;									// Message to send ABORT command to all back-end servers
			if(cnt < 2)
				req = FAIL;									// More than 2 servers crashed. Aborting further requests
		}

		for(int i = 0; i < 3; i++)							// Ask all back-end servers to COMMIT or ABORT the changes
		{		
			int rc = send(p[i].socket_fd,&req,sizeof(int),MSG_NOSIGNAL);
			if (rc < 0)
			{
				cout << "Send failed with rc: <" << rc << "> on server: " << i+1 << endl;
				continue;
			}
			else
				cout << "Sent msg: <" << req << "> to server: " << i+1 << endl;
		}

		for(int i = 0; i < 3; i++)							// Receive the response to client query from all back-end servers
		{
			bzero(tmp_buffer,256);
			int rc = recv(p[i].socket_fd,tmp_buffer,sizeof(tmp_buffer),0);
			if (rc <= 0)
			{
				cout << "Receive failed with rc: <" << rc << "> on server: " << i+1 << endl;
				continue;
			}
			else
			{
				strncpy(buffer, tmp_buffer, sizeof(tmp_buffer));	// Store the response only for running the back-end servers
				cout << "Received msg: <" << buffer << "> from server: " << i+1 << endl;
			}
		}

		// Two-Phase Commit Protocol Terminates

		pthread_mutex_unlock(&lock);							// Unlocking the Thread between front-end and back-end servers

		rc = send(newsockfd,buffer,strlen(buffer),0);			// Send the response received from beck-end servers to client
		if (rc < 0) 
			error("ERROR writing to socket");
		cout << "Sent msg: <" << buffer << "> to client: " << newsockfd << endl;

	}

	string quit = "OK\nConnection closed by foreign host.";
	strcpy(buffer,quit.c_str());
			
	rc = send(newsockfd,buffer,strlen(buffer),0);			// Send Connection Closing Message to Client
	if (rc < 0) 
		error("ERROR writing to socket");
	cout << "Sent msg: <" << buffer << "> to client: " << newsockfd << endl;

	close(newsockfd);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

	int n;
	long port;

	char buffer[256];
	long sockfd, portno;
	socklen_t clilen;
	
	struct sockaddr_in serv_addr, cli_addr;

	pthread_t threads;								// Threads for handling client requests

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	if(pthread_mutex_init(&lock, NULL) != 0)		// Initializing mutex object
	{
		cout << "mutex init has failed" << endl;
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd < 0) 
		error("ERROR opening socket");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	// To reuse socket address in case of crashes and failures
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)	// To reuse socket port in case of crashes and failures
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd,100);

	for(int i = 0; i < 3; i++)							// Connect to all Back-end Servers and store the connection information
	{
		cout << "Enter the port number of Machine to connect: ";
		cin >> p[i].port;

		p[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);

		if(p[i].socket_fd < 0) 
			error("ERROR opening socket");

		p[i].new_server = gethostbyname("localhost");
	
		bzero((char *) &p[i].new_serv_addr, sizeof(p[i].new_serv_addr));
		p[i].new_serv_addr.sin_family = AF_INET;
		bcopy((char *)p[i].new_server->h_addr, (char *)&p[i].new_serv_addr.sin_addr.s_addr, p[i].new_server->h_length);
		p[i].new_serv_addr.sin_port = htons(p[i].port);

		if(connect(p[i].socket_fd,(struct sockaddr *) &p[i].new_serv_addr,sizeof(p[i].new_serv_addr)) < 0) 
			error("ERROR connecting");
	}

	long newsockfd[100];
	clilen = sizeof(cli_addr);

	int i = 0;

	while(1)											// Accepting Client connections as and when they try to connect
	{
		newsockfd[i] = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
		pthread_create(&threads, NULL, NewClient, (void *)&newsockfd[i]);
		i++;
	}

	pthread_mutex_destroy(&lock);						// Code never reacher here, but still following good practice to destroy mutexes
}