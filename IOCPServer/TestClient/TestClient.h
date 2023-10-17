#pragma once
#include "NetClient.h"
#include "NetServerSerializeBuffer.h"
#include <string>

class TestClient : public CNetClient
{
public:
	TestClient() = delete;
	explicit TestClient(const std::wstring& optionFile);
	virtual ~TestClient();


public:
	virtual void OnConnectionComplete();
	virtual void OnRecv(CNetServerSerializationBuf* OutReadBuf);
	virtual void OnSend(int sendsize);

	virtual void OnWorkerThreadBegin();
	virtual void OnWorkerThreadEnd();
	virtual void OnError(st_Error* OutError);
};