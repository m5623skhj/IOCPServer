#include "PreCompile.h"
#include "IOCPSession.h"
#include <iostream>

IOCPSession::IOCPSession(SOCKET inSocket, SessionId inSessionId)
	: socket(inSocket)
	, sessionId(inSessionId)
{
	if (sessionId == INVALID_SESSION_ID)
	{
		std::cout << "Error : Session id is invalid" << std::endl;
	}
}

IOCPSession::~IOCPSession()
{

}

void IOCPSession::Initialize()
{
	recvIOData.bufferCount = 0;
	recvIOData.ioMode = 0;
	recvIOData.ringBuffer.InitPointer();
	ZeroMemory(&recvIOData.overlapped, sizeof(OVERLAPPED));

	sendIOData.bufferCount = 0;
	sendIOData.ioMode = 0;
	ZeroMemory(&sendIOData.overlapped, sizeof(OVERLAPPED));
}

void IOCPSession::OnReceived(NetBuffer& recvBuffer)
{

}

void IOCPSession::OnSend()
{

}
