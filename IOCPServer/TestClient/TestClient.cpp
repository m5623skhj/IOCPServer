#include "PreCompile.h"
#include "TestClient.h"

TestClient::TestClient(const std::wstring& optionFile)
{
    Start(optionFile.c_str());
}

TestClient::~TestClient()
{
    Stop();
}


void TestClient::OnConnectionComplete()
{

}

void TestClient::OnRecv(CNetServerSerializationBuf* OutReadBuf)
{

}

void TestClient::OnSend(int sendsize)
{
    UNREFERENCED_PARAMETER(sendsize);
}

void TestClient::OnWorkerThreadBegin()
{

}

void TestClient::OnWorkerThreadEnd()
{

}

void TestClient::OnError(st_Error* OutError)
{
    std::cout << OutError->ServerErr << " / " << OutError->GetLastErr << std::endl;
}