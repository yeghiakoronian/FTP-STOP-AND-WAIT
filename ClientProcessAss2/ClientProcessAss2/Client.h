#pragma once

#define TRACE

#ifdef TRACE
#define print(x) { stringstream str; str << "Client: "<< x; outputMessage(str.str()); }
#else
#define print(x) { cout << "Client: "<< x << endl; }
#endif


class Client
{

	
public:
	int m_listeningPort;
	int m_routerListeningPort;
	string m_routerHostName;
	string m_hostname;
	WSADATA m_wdata;
	SOCKET m_listeningSocket;
	SOCKADDR_IN m_localSocAddr; 
	SOCKADDR_IN m_remoteSocAddr;
	char m_recBuff[MAXPACKETSIZE];
	int m_localGeneratedNumber;
	int m_remotGeneratedNumber;
	int m_localSequenceNumber;
	int m_remoteSequenceNumber;
	MessageHeader* m_lastSendPacket;

	ofstream m_logFile;
	bool m_acknowledgementState;// when it is falce no acknowledgment is received for a current request
	void getFilesList();
	bool bindToListeningPort();
	MessageHeader* sendUDPPacket(MessageHeader* message);
	bool initializeLocalSocAdd();
	bool initializeRemoteSocAdd();
	MessageHeader* receiveUDPPacket();
	MessageHeader*runSelectMethod();
	void handShaking();
	int generateRandomNum();
	int processClientMessages();
	string readCommand();
	bool getFile(string fileName);
	string getFileList();
	bool setFile(string fileName);

	void listFilesInDirectory();
	void updateSequenceNumber(int& sequence);
	void sendACK(string messageBody);
	void resendLastPacket();
	MessageHeader*  waitForAcknowledgement();
	MessageHeader* waitForData(int currentChanck);
	void updateLastSendPacket(MessageHeader* lastSentPacket);

	int getLastSignificantBit(int number);
	void split(string fileList, vector<string>* fileItems);
	void parsServerFileList(string list);
	string intToStr(int intValue);
	void resetState();

	void freeLogFile(string log);
	bool fileExists(std::string fileName);



	Client(void);
	~Client(void);


	static void writeIntoLogFile(string log);
	static void outputMessage(string message);
	
	class OperationError
	{
	public:
		string Message;
		OperationError(string message) : Message(message)
		{
		}
	};


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

