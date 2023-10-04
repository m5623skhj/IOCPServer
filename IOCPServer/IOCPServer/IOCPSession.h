#pragma once
#include "Ringbuffer.h"
#include "DefineType.h"

class IOCPSession
{
public:
	IOCPSession() = delete;
	IOCPSession(SOCKET inSocket, SessionId inSessionId);
	virtual ~IOCPSession();

public:

private:

};