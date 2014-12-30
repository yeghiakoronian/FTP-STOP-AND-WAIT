#pragma once


enum MessageType { DATA = 1, GET = 2, PUT = 3,ERR = 4, LIST = 6, ACK = 7, USERNAME = 8,  HANDSHAKE = 9};
#define MAX_THREADS 5
#define FILEPATH ".\\files\\"
#define FILENAME	"log.txt"
//#define maxDataSize 80
//#define MAXPACKETSIZE (sizeof(MessageHeader) + maxDataSize)
#define MAXPACKETSIZE 80
#define maxDataSize (MAXPACKETSIZE - sizeof(MessageHeader))
#define RETRY 30
#define waitall 8
#define TIMEOUT 300

struct ServerParams
{
	ServerParams(std::string hostname = "localhost", int port = 2500)
	{
		
		serverListeningPort = port;
		this->hostname = hostname;
	}
	std::string hostname;
	int serverListeningPort;

};

struct CommunicationMessage{
	std::string messageType;
	std::string messageBody;
	
};

struct MessageHeader{
	MessageType msgType;
	int sequenceNumber;
	unsigned int msgSize;
	int isLastChunck;
	int ACK; // in caseif we need to send a data and acknowledge at the same time.
						//it keeps the sequence number of the message it wants to acknowlegde	
	//for debug 
	int msgnumber;
	char data[0];
	
};

struct DataMessage{
	unsigned int size;
	byte isLast;
	unsigned char* chunck;
};

struct HandshackMessage{
	MessageType msgType;
	int acknowledge;
	char data[0];
	
};