#pragma once
#include "Ringbuffer.h"
#include "DefineType.h"
#include <atomic>
#include "NetServerSerializeBuffer.h"
#include "LockFreeQueue.h"

class IOCPServer;
class IPacket;

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
	void OnReceived(NetBuffer& recvPacket);
	void OnSessionReleased();
	virtual void OnClientEntered() {}
	virtual void OnClientLeaved() {}

public:
	void SendPacket(IPacket& packet);
	void SendPacket(NetBuffer& packet);
	void SendPacketAndDisconnect(IPacket& packet);
	void SendPacketAndDisconnect(NetBuffer& packet);

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

	UINT nowPostQueueing = NONSENDING;
	NetBuffer* storedBuffer[ONE_SEND_WSABUF_MAX];
#pragma endregion IO
};