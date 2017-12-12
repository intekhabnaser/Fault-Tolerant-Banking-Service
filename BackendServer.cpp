/*
Author: Intekhab Naser
Back-end servers perform all the processing of queries requested by the client and forwarded by front-end server.
They participate and respond to Coordinator commands in 2 Phase Commit Protocol. 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace std;

#define READY 1
#define OK 2
#define COMMIT 3
#define ABORT 4
#define FAIL 5

struct bank_record									// Structure to store bank record of all accounts
{
	int acc_number;
	float balance;
}account[1000];

int num_of_accounts = 0;							// Variable for storing total number of accounts so far

void error(const char *msg)							// Function to print error messages
{
	perror(msg);
	exit(1);
}

void *NewConnection(void *sockfd) 					// Thread function for communicating with front-end server
{
	int req, n;
	char buffer[256];
	
	long *ptr = (long *)sockfd;
	long newsockfd = *ptr;
	
	if(newsockfd < 0) 
		error("ERROR on accept");

	int choice;

	while(1)
	{
		bzero(buffer,256);
		int rc = recv(newsockfd,buffer,sizeof(buffer),0);	// Read the request message sent by client via front-end server
		if(rc < 0) 
			error("ERROR reading from socket");

		// Two-Phase Commit Protocol Begins

		rc = recv(newsockfd,&req,sizeof(int),0);			// Read the voting request from front-end server
		if(rc < 0) 
			error("ERROR reading from socket");
	
		if(req == READY)									// Check if the message is READY message from Coordinator
		{
			cout << "Received voting msg 'READY?' from Coordinator!" << endl;
			req = OK;
			int rc = send(newsockfd,&req,sizeof(int),0);	// Send ACK 'OK' message to front-end server denoting READY state
			if (rc < 0) 
				error("ERROR writing to socket");
			cout << "Sent 'OK' back to front-end server" << endl;
		}

		rc = recv(newsockfd,&req,sizeof(int),0);			// Recv the COMMIT or ABORT command from front-end server
		if(rc < 0) 
			error("ERROR reading from socket");

		if(req == COMMIT)
		{
			cout << "'COMMIT' received from front-end" << endl;
		
			stringstream ss;
			string tmpstr;
			ss.str("");
			ss << buffer;
			ss >> tmpstr;										// Check the request type and process accordingly

			if(tmpstr == "CREATE")
			{
				choice = 1;
			}
			else if(tmpstr == "UPDATE")
			{
				choice = 2;
			}
			else if(tmpstr == "QUERY")
			{
				choice = 3;
			}

			switch(choice)
			{
				case 1:
				{
					float amnt;
					ss >> amnt;

					string create;

					if(amnt < 0)
					{
						create = "ERR Invalid Amount\r\n";
					}
					else
					{
						if(num_of_accounts == 0)						// If this is the first account to be created
						{
							account[num_of_accounts].acc_number = 100;	// Assign account number as 100
						}
						else											// If accounts already exist
						{
							account[num_of_accounts].acc_number = account[num_of_accounts - 1].acc_number + 1;	// Assign account number as (last accnt + 1) 
						}
						account[num_of_accounts].balance = amnt;		// Update balance as provided

						stringstream stream1;
						stream1 << account[num_of_accounts].acc_number;
						string s1 = stream1.str();

						create = "OK ";
						create += s1;
						create += "\r\n";

						num_of_accounts++;								// Increment the count of 'Number of Accounts'
					}

					strcpy(buffer,create.c_str());
					cout << buffer << endl;

					int rc = send(newsockfd,buffer,strlen(buffer),0);	// Reply Success confirmation 'OK' with generated account number
					if (rc < 0) 
						error("ERROR writing to socket");

					cout << "\tAccNo\tBal" << endl;
					for(int i = 0; i < num_of_accounts; i++)
						cout << "\t" << account[i].acc_number << "\t" << account[i].balance << endl;

					break;
				}
				case 2:
				{
					int accnt;
					ss >> accnt;
					
					float amnt;
					ss >> amnt;
					
					string update;
					stringstream stream1;
					string s1;
					
					if(num_of_accounts == 0)							// If no accounts exist, construct a reply with Error message
					{
						update = "ERR ";
						update += "Account ";
						
						stringstream tmp_ss;
						tmp_ss << accnt;
						
						string tmp_s = tmp_ss.str();
						update += tmp_s;
						
						update += " does not exist.";
					}
					else
					{
						int flag = 0;
						
						for(int i = 0; i < num_of_accounts; i++)
						{
							if(account[i].acc_number == accnt)			// If given account exists, update the balance by given amount
							{
								if(account[i].balance + amnt < 0)
								{
									update = "ERR Insufficient Balance";

									flag = 2;

									break;
								}
								else
								{
									account[i].balance += amnt;
									
									stream1 << fixed << setprecision(2) << account[i].balance;
									s1 = stream1.str();
									
									flag = 1;
									
									break;
								}
							}
						}
						if(flag == 0)									// If given accounts does not exist, construct a reply with Error message
						{
							update = "ERR ";
							update += "Account ";
							
							stringstream tmp_ss;
							tmp_ss << accnt;
							
							string tmp_s = tmp_ss.str();
							update += tmp_s;
							
							update += " does not exist.";
						}
						else if(flag == 1)
						{
							update = "OK ";								// If given accounts exists, construct a reply with ACK 'OK' message
							update += s1;
						}
					}
					
					update += "\r\n";

					strcpy(buffer,update.c_str());

					cout << buffer << endl;

					int rc = send(newsockfd,buffer,strlen(buffer),0);	// Reply Success/Failure with Updated Balance amount
					if (rc < 0) 
						error("ERROR writing to socket");

					cout << "\tAccNo\tBal" << endl;
					for(int i = 0; i < num_of_accounts; i++)
						cout << "\t" << account[i].acc_number << "\t" << account[i].balance << endl;

					break;
				}
				case 3:
				{
					int accnt;
					ss >> accnt;
			
					string query;
					stringstream stream1;
					string s1;
				
					if(num_of_accounts == 0)							// If no accounts exist, construct a reply with Error message
					{
						query = "ERR ";
						query += "Account ";
				
						stringstream tmp_ss;
						tmp_ss << accnt;
				
						string tmp_s = tmp_ss.str();
						query += tmp_s;
				
						query += " does not exist.";
					}
					else
					{
						int flag = 0;
					
						for(int i = 0; i < num_of_accounts; i++)
						{
							if(account[i].acc_number == accnt)			// If given accounts exist
							{
								stream1 << fixed << setprecision(2) << account[i].balance;
								s1 = stream1.str();

								flag = 1;

								break;
							}
						}
						if(flag == 0)									// If no accounts exist, construct a reply with Error message
						{
							query = "ERR ";
							query += "Account ";
						
							stringstream tmp_ss;
							tmp_ss << accnt;
						
							string tmp_s = tmp_ss.str();
							query += tmp_s;
						
							query += " does not exist.";
						}
						else
						{
							query = "OK ";								// Construct a reply with ACK 'OK' message
							query += s1;
						}
					}
					
					query += "\r\n";

					strcpy(buffer,query.c_str());
				
					cout << buffer << endl;
				
					int rc = send(newsockfd,buffer,strlen(buffer),0);	// Reply Success/Failure with Current Balance amount
					if (rc < 0) 
						error("ERROR writing to socket");

					cout << "\tAccNo\tBal" << endl;
					for(int i = 0; i < num_of_accounts; i++)
						cout << "\t" << account[i].acc_number << "\t" << account[i].balance << endl;

					break;
				}
			}
		}
		else if(req == ABORT)
		{
			cout << "'ABORT' received from front-end" << endl;

			string abort = "Request Aborted! Not all participants are ready.\r\n";
			strcpy(buffer,abort.c_str());

			cout << buffer << endl;
				
			int rc = send(newsockfd,buffer,strlen(buffer),0);			// Reply Abort message
			if (rc < 0) 
				error("ERROR writing to socket");
		}
		else if(req == FAIL)
		{
			cout << "'FAIL' received from front-end" << endl;

			string fail = "Request Aborted! More than one back-end servers failed.\r\n";
			strcpy(buffer,fail.c_str());

			cout << buffer << endl;
				
			int rc = send(newsockfd,buffer,strlen(buffer),0);			// Reply fail message
			if (rc < 0) 
				error("ERROR writing to socket");
		}

		// Two-Phase Commit Protocol Terminates
	}

	close(newsockfd);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	long sockfd, newsockfd, portno;
	socklen_t clilen;
	
	struct sockaddr_in serv_addr, cli_addr;

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
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
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	//To reuse socket address in case of crashes and failures
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)	//To reuse socket port in case of crashes and failures
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");
     	
	listen(sockfd,100);
	clilen = sizeof(cli_addr);

	pthread_t threads;

	newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);

	if(newsockfd < 0) 
		error("ERROR on accept");

	pthread_create(&threads, NULL, NewConnection, (void *)&newsockfd);	// Thread for connection with front-end server

	int rc = pthread_join(threads, NULL);
	if(rc)
	{
		printf("Error in joining thread :\n");
		cout << "Error: " << rc << endl;
		exit(-1);
	}
	
	close(sockfd);
	
	return 0; 
	
	pthread_exit(NULL);
}