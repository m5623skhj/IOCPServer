#pragma once
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include <thread>
#include <vector>
#include "Enum.h"
#include "DefineType.h"
#include "NetServerSerializeBuffer.h"

#pragma comment(lib, "ws2_32.lib")

#define NONSENDING	0
#define SENDING		1

#define SESSION_INDEX_SHIFT 48

#define POST_RETVAL_ERR_SESSION_DELETED		0
#define POST_RETVAL_ERR						1
#define POST_RETVAL_COMPLETE				2

#define ONE_SEND_WSABUF_MAX					200

#define df_RELEASE_VALUE					0x100000000

struct st_Error;

class IOCPSession;

class IOCPServer
{
public:
	IOCPServer();
	~IOCPServer() = default;

public:
	static IOCPServer& GetInst()
	{
		static IOCPServer instance;
		return instance;
	}

public:
	bool StartServer(const std::wstring& optionFileName);
	void StopServer();

private:
	SOCKET listenSocket;

#pragma region thread
public:
	void Accepter();
	void Worker(BYTE inThreadId);

private:
	void RunThreads();

private:
	IO_POST_ERROR RecvCompleted(IOCPSession& session, DWORD transferred);
	IO_POST_ERROR SendCompleted(IOCPSession& session);

	WORD GetPayloadLength(OUT NetBuffer& buffer, int restSize);

private:
	std::thread accepterThread;
	std::vector<std::thread> workerThreads;
#pragma endregion thread

#pragma region serverOption
private:
	bool ServerOptionParsing(const std::wstring& optionFileName);
	bool SetSocketOption();

private:
	BYTE numOfWorkerThread = 0;
	bool nagleOn = false;
	WORD port = 0;
	UINT maxClientCount = 0;
#pragma endregion serverOption
};