#include "PreCompile.h"
#include "TestClient.h"
#include "../IOCPServer/Protocol.h"

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
    NetBuffer& buffer = *NetBuffer::Alloc();
    UINT packetId = static_cast<UINT>(PACKET_ID::PING);
    buffer << packetId;

    SendPacket(&buffer);
}

void TestClient::OnRecv(CNetServerSerializationBuf* OutReadBuf)
{
    UINT outputId;
    *OutReadBuf >> outputId;

    if (outputId != static_cast<UINT>(PACKET_ID::PONG))
    {
        std::cout << "Invalid packet id" << std::endl;
        return;
    }

    NetBuffer& buffer = *NetBuffer::Alloc();
    UINT packetId = static_cast<UINT>(PACKET_ID::PING);
    buffer << packetId;

    SendPacket(&buffer);
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