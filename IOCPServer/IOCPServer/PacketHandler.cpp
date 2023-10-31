#include "PreCompile.h"
#include "PacketManager.h"
#include "IOCPSession.h"

bool PacketManager::HandlePacket(IOCPSession& session, Ping& packet)
{
	Pong pong;
	session.SendPacket(pong);

	return true;
}

bool PacketManager::HandlePacket(IOCPSession& session, RequestFileStream& packet)
{
	UNREFERENCED_PARAMETER(packet);

	return true;
}