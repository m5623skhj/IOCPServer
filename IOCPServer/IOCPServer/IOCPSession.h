#pragma once
#include "Ringbuffer.h"
#include "DefineType.h"
#include <atomic>

class IOCPServer;

class IOCPSession
{
	friend IOCPServer;

public:
	IOCPSession() = delete;
	IOCPSession(SOCKET inSocket, SessionId inSessionId);
	virtual ~IOCPSession();

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
#pragma endregion IO
};