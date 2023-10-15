#include "PreCompile.h"
#include "PacketManager.h"
#include "IOCPSession.h"

bool PacketManager::HandlePacket(IOCPSession& session, Ping& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}