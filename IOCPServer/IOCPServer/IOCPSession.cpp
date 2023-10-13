#include "PreCompile.h"
#include "IOCPSession.h"
#include <iostream>
#include "PacketManager.h"

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

void IOCPSession::OnReceived(NetBuffer& recvPacket)
{
	PacketId packetId;
	recvPacket >> packetId;

	auto packetHandler = PacketManager::GetInst().GetPacketHandler(packetId);
	if (packetHandler == nullptr)
	{
		return;
	}

	auto packet = PacketManager::GetInst().MakePacket(packetId);
	if (packet == nullptr)
	{
		return;
	}

	if (packet->GetPacketSize() < recvPacket.GetUseSize())
	{
		return;
	}

	char* targetPtr = reinterpret_cast<char*>(packet.get()) + sizeof(char*);
	memcpy(targetPtr, recvPacket.GetReadBufferPtr(), recvPacket.GetUseSize());
	std::any anyPacket = std::any(packet.get());
	packetHandler(*this, anyPacket);
}

void IOCPSession::OnSessionReleased()
{

}
