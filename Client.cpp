/*
Author: Intekhab Naser
Client queries Bank server for the following tasks:
Create a new account with specific amount, Update any existing account with specific amount and Check balance of specific account.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>

using namespace std;

void error(const char *msg)									// Function to print error messages
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	long sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	if(argc < 3) {
		fprintf(stderr,"usage %s port hostname\n", argv[0]);
		exit(0);
	}
	
	portno = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd < 0) 
		error("ERROR opening socket");
	
	server = gethostbyname(argv[2]);
	
	if(server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR connecting");

	bzero(buffer,256);
	n = recv(sockfd,buffer,sizeof(buffer),0);				// Read connection acknowledgement message from front-end server
	if(n < 0) 
		error("ERROR reading from socket");
	
	cout << buffer << endl;
	
	int choice;												// Variable for storing User's choice of action
	
	do{
		cout << "Select your choice of action:" << endl;
		cout << "\t1) Create account\n\t2) Update account\n\t3) Check account balance\n\t4) Quit" << endl;
		cin >> choice;
	
		switch(choice){
			case 1:
			{
				float amnt;
					
				cout << "Enter initial deposit amount: ";
				cin >> amnt;
				
				stringstream stream;
				stream << fixed << setprecision(2) << amnt;
				string s = stream.str();
				string create = "CREATE ";
				create += s;
					
				bzero(buffer,256);
				strcpy(buffer,create.c_str());
				
				n = send(sockfd,buffer,strlen(buffer),0);	// Send CREATE command to front-end server
				if (n < 0) 
					error("ERROR writing to socket");
					
				cout << "Sent msg : " << create << endl;
					
				bzero(buffer,256);
				n = recv(sockfd,buffer,sizeof(buffer),0);	// Receive the reply for CREATE from front-end server
				if(n < 0) 
					error("ERROR reading from socket");
				
				cout << buffer << endl;
				break;
			}
			case 2:
			{
				int accnt;
				float amnt;
				
				cout << "Enter the account number to update: ";
				cin >> accnt;
				
				stringstream stream1;
				stream1 << accnt;
				string s1 = stream1.str();
				
				cout << "Enter the amount to update: ";
				cin >> amnt;
				
				stringstream stream2;
				stream2 << fixed << setprecision(2) << amnt;
				string s2 = stream2.str();
				string update = "UPDATE ";
				update += s1;
				update += " ";
				update += s2;
				
				bzero(buffer,256);
				strcpy(buffer,update.c_str());
				
				n = send(sockfd,buffer,strlen(buffer),0);	// Send UPDATE command to front-end server
				if (n < 0) 
					error("ERROR writing to socket");
				
				cout << "Sent msg : " << update << endl;
				
				bzero(buffer,256);
				n = recv(sockfd,buffer,255,0);				// Receive the reply for UPDATE from front-end server
				if(n < 0) 
					error("ERROR reading from socket");
				
				cout << buffer << endl;
				break;
			}
			case 3:
			{
				int accnt;
			
				cout << "Enter the account number to check: ";
				cin >> accnt;
		
				stringstream stream1;
				stream1 << accnt;
				string s1 = stream1.str();
				string query = "QUERY ";
				query += s1;
		
				bzero(buffer,256);
				strcpy(buffer,query.c_str());
		
				n = send(sockfd,buffer,strlen(buffer),0);	// Send QUERY command to front-end server
				if (n < 0) 
					error("ERROR writing to socket");
		
				cout << "Sent msg : " << query << endl;
		
				bzero(buffer,256);
				n = recv(sockfd,buffer,255,0);				// Receive the reply for QUERY from front-end server
				if(n < 0) 
					error("ERROR reading from socket");
		
				cout << buffer << endl;
				break;
			}
			case 4:
			{
				strcpy(buffer,"QUIT");
		
				n = send(sockfd,buffer,strlen(buffer),0);	// Send QUIT command to front-end server
				if (n < 0) 
					error("ERROR writing to socket");
		
				bzero(buffer,256);
				n = recv(sockfd,buffer,255,0);				// Receive the reply for QUIT from front-end server
				if(n < 0) 
					error("ERROR reading from socket");
		
				cout << buffer << endl;
				break;
			}
		}
		
		if(choice < 1 || choice > 4)
			cout << "Invalid Choice. Please try again!" << endl;
	}while(choice != 4);

	return 0;
}

