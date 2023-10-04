#pragma once
#include "ServerCommon.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"

#define NONSENDING	0
#define SENDING		1

#define SESSION_INDEX_SHIFT 48

#define POST_RETVAL_ERR_SESSION_DELETED		0
#define POST_RETVAL_ERR						1
#define POST_RETVAL_COMPLETE				2

#define ONE_SEND_WSABUF_MAX					200

#define df_RELEASE_VALUE					0x100000000

struct st_Error;

class CNetServerSerializationBuf;
class IOCPSession;

class IOCPServer
{
public:
	IOCPServer();
	virtual ~IOCPServer() = default;

private:

};