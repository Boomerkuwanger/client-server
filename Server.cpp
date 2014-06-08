#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//------------------------------------------
#include "Request.h"

#define MAX_LENGTH 255

using namespace std;

struct find_id : std::unary_function<clientEntry, bool> {
    int client;
    find_id(int client):client(client) { }
    bool operator()(clientEntry const& myEntry) const {
        return myEntry.client == client;
    }
};

void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

bool compareByClient(const clientEntry &a, const clientEntry &b)
{
	return a.client < b.client;
}

int main(int argc, char* argv[])
{
	int sock;                       /* Socket */
    struct sockaddr_in serverAddr;	/* Local address */
    struct sockaddr_in clientAddr;	/* Client address */
    unsigned int clientAddrLength;  /* Length of incoming message */
	request newRequest;				/* Request received from client */
	vector<clientEntry> clientTable;/* The client table of all the clients */
	clientEntry *clientTableNomad;	/* Client for client table lookup */
	char echoBuffer[MAX_LENGTH];	/* Raw data to receive from the client */
    unsigned short serverPort;		/* Server port */
    int recvMsgSize;                /* Size of received message */
	
	if (argc != 2)
	{
		DieWithError("Failed to start server, please provide a port number to listen on...");
	}	

	serverPort = atoi(argv[1]);
	
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		DieWithError("Failed to create socket...");
	}

	/* Construct local address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));		/* Zero out structure */
    serverAddr.sin_family = AF_INET;                /* Internet address family */
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    serverAddr.sin_port = htons(serverPort);		/* Local port */

	/* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        DieWithError("Failed to bind socket to the local address...");
    }

	for (;;) /* Run forever */
    {
		clientTableNomad = (clientEntry*)malloc(sizeof(clientEntry));

        /* Set the size of the in-out parameter */
        clientAddrLength = sizeof(clientAddr);
		memset(&echoBuffer, 0, sizeof(echoBuffer));

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, echoBuffer, MAX_LENGTH, 0,
        	(struct sockaddr *) &clientAddr, &clientAddrLength)) < 0)
        {
            DieWithError("Failed to recieve message from client...");
        }
       
		
		//const unsigned char * const px = (unsigned char*)&echoBuffer;
		/*
		unsigned int i;
		cout << "HEX DUMPING" << endl;
		for (i = 0; i < sizeof(echoBuffer); ++i) printf("%02X ", px[i]);
		cout << "DONE" << endl;
		*/
		
		/* Parsing buffer to newRequest */
		memset(&newRequest, 0, sizeof(request));
		memcpy(&newRequest, &echoBuffer, sizeof(request));
		/* Lookup the client in the client talbe */
		vector<clientEntry>::iterator iter = find_if(clientTable.begin(), clientTable.end(), find_id(newRequest.client));

		/* If the client is not found, add it */
		if (iter == clientTable.end()) {
			clientTableNomad->process_id = getpid();
			clientTableNomad->inc = newRequest.inc;
			clientTableNomad->client = newRequest.client;			
			clientTableNomad->requestNum = newRequest.req;
			strcpy(clientTableNomad->sendMsg, "     ");						
			clientTable.push_back(*clientTableNomad);
			iter = clientTable.end() - 1;
			PrintRequest(&newRequest);
		}
 		cout << "Handling request from [" << inet_ntoa(clientAddr.sin_addr) << "] with client number [" << iter->client << "]" << endl;
		/* Else the iterator is pointing to the discovered client */
		// FAILURE CODE:
		// srand(time(NULL));
		// bool Failure = (rand() % 100) < 10;//may need time seed
		
		/* These cases compare 'R' (client request number) to 'r' (server request number) */
		if (newRequest.req < iter->requestNum) {
			cout << "Case 1: " << endl;			
			cout << "The request from " << inet_ntoa(clientAddr.sin_addr) << " was already recieved." << endl;
			cout << "The client's request number [" << newRequest.req << "] is not equal to ";
			cout << "the server's request number [" << iter->requestNum << "]" << endl;
			++iter->requestNum;
		}
		else if (newRequest.req == iter->requestNum) {
			/* Send received datagram back to the client */
			cout << "Case 2: " << endl;
			for (int i = REPLY_SIZE - 1; i > 0; i--)
			{
				iter->sendMsg[i] = iter->sendMsg[i - 1];
			}
			iter->sendMsg[0] = newRequest.c;

			unsigned char buffer[sizeof(iter->sendMsg)];
			memcpy(&buffer, &iter->sendMsg, sizeof(iter->sendMsg) + 1);
			cout << "Sending message [" << buffer << "] to client. " << endl;
			
			bool responseFailure = false; //(rand() % 100) < 10;//seed by time
			if(responseFailure == false)
			{
				if(sendto(sock, buffer, sizeof(buffer), 0,
					(struct sockaddr *) &clientAddr, sizeof(clientAddr)) == -1)
				{
					DieWithError("Failed to send message, sendto sent a different number of bytes than expected...");
				}
			}
			else
			{
				perror("The server performed the request but failed to return a response to client.");
			}

			cout << "The request from " << inet_ntoa(clientAddr.sin_addr) << " was recieved and acknowledged" << endl;
			cout << "The client's request number [" << newRequest.req << "] is equal to ";
			cout << "the server's request number [" << iter->requestNum << "]" << endl;
			++iter->requestNum;
		}
		else if (newRequest.req > iter->requestNum) {
			cout << "Case 3: " << endl;
			for (int i = REPLY_SIZE - 1; i > 0; i--)
			{
				iter->sendMsg[i] = iter->sendMsg[i - 1];
			}
			iter->sendMsg[0] = newRequest.c;

			/* Send received datagram back to the client */
			unsigned char buffer[sizeof(iter->sendMsg)];
			memcpy(&buffer, &iter->sendMsg, sizeof(iter->sendMsg));
			
			bool responseFailure = (rand() % 100) < 10;//seed by time
			if(responseFailure == false)
			{
				if(sendto(sock, buffer, sizeof(buffer), 0,
					(struct sockaddr *) &clientAddr, sizeof(clientAddr)) == -1)
				{
					DieWithError("Failed to send message, sendto sent a different number of bytes than expected...");
				}
			}
			else
			{
				perror("The server performed the request but failed to return a response to client.");
			}
			cout << "The request from " << inet_ntoa(clientAddr.sin_addr) << " was recieved and acknowledged" << endl;
			cout << "The client's request number [" << newRequest.req << "] was greater than ";
			cout << "the server's request number [" << iter->requestNum << "]" << endl;
			++iter->requestNum;
		}
		else
		{
			perror("The server dropped the request");
		}
		cout << endl;
		/* Sort the client table in the interest of lookup times */
		sort(clientTable.begin(), clientTable.end(), compareByClient);
    }

	return 0;
}
