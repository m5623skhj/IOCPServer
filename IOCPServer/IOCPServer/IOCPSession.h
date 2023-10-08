#pragma once
#include "Ringbuffer.h"
#include "DefineType.h"
#include <atomic>
#include "NetServerSerializeBuffer.h"
#include "LockFreeQueue.h"

class IOCPServer;

struct RecvOverlappedData
{
	WORD bufferCount;
	UINT ioMode;
	OVERLAPPED overlapped;
	CRingbuffer ringBuffer;
};

struct SendOverlappedData
{
	LONG bufferCount;
	UINT ioMode;
	OVERLAPPED overlapped;
	CLockFreeQueue<NetBuffer*> sendQ;
};

class IOCPSession
{
	friend IOCPServer;

public:
	IOCPSession() = delete;
	IOCPSession(SOCKET inSocket, SessionId inSessionId);
	virtual ~IOCPSession();

	void Initialize();

public:
	void OnReceived();
	void OnSend();
	virtual void OnClientEntered() {}
	virtual void OnClientLeaved() {}

private:
	SOCKET socket;
	SessionId sessionId = INVALID_SESSION_ID;

	bool isSendAndDisconnect = false;
	std::atomic_bool isReleasedSession = false;

#pragma region IO
private:
	LONG ioCount = 0;
	bool ioCancel = false;

	RecvOverlappedData recvIOData;
	SendOverlappedData sendIOData;
	OVERLAPPED postQueueOverlapped;
#pragma endregion IO
};