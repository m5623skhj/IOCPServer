#pragma once
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include <thread>
#include <vector>
#include "Enum.h"
#include "DefineType.h"
#include "NetServerSerializeBuffer.h"
#include <map>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

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
	SOCKET listenSocket = INVALID_SOCKET;
	HANDLE iocpHandle;

#pragma region thread
public:
	void Accepter();
	void Worker(BYTE inThreadId);

private:
	void GetQueuedCompletionStatusSuccess(LPOVERLAPPED overlapped, SessionId sessionId, DWORD transferred);
	void GetQueuedCompletionStatusFailed(LPOVERLAPPED overlapped, SessionId sessionId, DWORD transferred);

	std::shared_ptr<IOCPSession> GetSession(SessionId sessionId);
	bool IsClosedSession(DWORD transferred, IOCPSession& session);

	void IOCompletedProcess(LPOVERLAPPED overlapped, IOCPSession& session, DWORD transferred);

	IO_POST_ERROR IORecvPart();
	IO_POST_ERROR IOSendPart(IOCPSession& session);
	IO_POST_ERROR IOSendPostPart();

private:
	void RunThreads();

private:
	IO_POST_ERROR RecvCompleted(IOCPSession& session, DWORD transferred);
	IO_POST_ERROR SendCompleted(IOCPSession& session);

	WORD GetPayloadLength(OUT NetBuffer& buffer, int restSize);

private:
	std::thread accepterThread;
	std::vector<std::thread> workerThreads;

	bool* workerOnList;
#pragma endregion thread

#pragma region session
private:
	bool MakeNewSession(SOCKET enteredClientSocket);
	std::shared_ptr<IOCPSession> GetNewSession(SOCKET enteredClientSocket);
	bool ReleaseSession(OUT IOCPSession& releaseSession);

	void IOCountDecrement(OUT IOCPSession& session);

private:
	std::map<UINT64, std::shared_ptr<IOCPSession>> sessionMap;
	std::mutex sessionMapLock;

	UINT sessionCount = 0;
	SessionId nextSessionId = INVALID_SESSION_ID + 1;

#pragma endregion session

#pragma region io
private:
	IO_POST_ERROR RecvPost(OUT IOCPSession& session);
	IO_POST_ERROR SendPost(OUT IOCPSession& session);
#pragma endregion io

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