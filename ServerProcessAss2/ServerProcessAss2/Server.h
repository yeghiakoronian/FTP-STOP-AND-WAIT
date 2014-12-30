#pragma once


#define TRACE

#ifdef TRACE
#define print(x) { stringstream str; str << "Server: "<< x; outputMessage(str.str()); }
#else
#define print(x) { cout << "Server: "<< x << endl; }
#endif



class Server
{
public:
	WSADATA m_wdata;
	int m_listeningPort;
	int m_routerListeningPort;
	string m_routerHostName;
	SOCKET m_listeningSocket;
	char m_recBuff[MAXPACKETSIZE];
	int m_localGeneratedNumber;
	int m_remotGeneratedNumber;
	

	int m_localSequence;
	int m_remoteSequence;
	int m_requestNumber;
	bool m_handshackeIsDone;

	MessageHeader* m_lastSendPacket;
	char* m_sendBuffer;

	SOCKADDR_IN m_localSocAddr; 
	SOCKADDR_IN m_remoteSocAddr;

	bool sendUDPPacket(MessageHeader* message);
	bool initializeLocalSocAdd();
	bool initializeRemoteSocAdd();
	MessageHeader* receiveUDPPacket();
	MessageHeader* runSelectMethod();
	void handShaking();
	int generateRandomNum();

	int getLastSignificantBit(int number);
	void processIncommingMessages();
	bool fileExists(std::string fileName);
	void receiveFile(std::string fileName);
	void sendRequestedFile(std::string filePath);
	void updateSequenceNumber(int& sequence);
	void sendACK(string messageBody);
	void resendLastPacket();
	MessageHeader*  waitForAcknowledgement();
	MessageHeader* waitForData();
	void updateLastSendPachet(MessageHeader* lastSentPacket);
	static void writeIntoLogFile(string log);
	static void outputMessage(string message);
	string intToStr(int intValue);
	MessageHeader*  waitForHandShack();
	void sendErrorMessage(string messageBody);
	void resetState();
	void listFilesInDirectory();
	void sendFileList(string list);
	void freeLogFile(string log);
	


	Server(void);
	~Server(void);
	bool bindToListeningPort();


		void resetStats()
	{
		sentPackets =
		sentBytes =
		recvPackets =
		recvBytes = 0;
	}

	int sentPackets;
	int sentBytes;
	int recvPackets;
	int recvBytes;

	int MsgSize(MessageHeader * msg)
	{
		return sizeof(MessageHeader) + msg->msgSize;
	}



	
};

