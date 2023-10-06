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

void IOCPSession::OnReceived()
{

}

void IOCPSession::OnSend()
{

}
