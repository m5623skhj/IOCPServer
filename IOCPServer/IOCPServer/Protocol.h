#pragma once
#include "Enum.h"

using PacketId = unsigned int;

#define GET_PACKET_SIZE() virtual int GetPacketSize() override { return sizeof(*this) - 8; }
#define GET_PACKET_ID(packetId) virtual PacketId GetPacketId() const override { return static_cast<PacketId>(packetId); }

#pragma pack(push, 1)
class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	virtual PacketId GetPacketId() const = 0;
	virtual int GetPacketSize() = 0;
};

class Ping : public IPacket
{
public:
	Ping() = default;
	~Ping() = default;
	GET_PACKET_ID(PACKET_ID::PING);
	GET_PACKET_SIZE();
};

class Pong : public IPacket
{
public:
	Pong() = default;
	~Pong() = default;
	GET_PACKET_ID(PACKET_ID::PONG);
	GET_PACKET_SIZE();
};

class RequestFileStream : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::REQUEST_FILE_STREAM);
	GET_PACKET_SIZE();
};

class ResponseFileStream: public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::RESPONSE_FILE_STREAM);
	GET_PACKET_SIZE();

public:
	char fileStream[4096];
};

#pragma pack(pop)

#define REGISTER_PACKET(PacketType){\
	RegisterPacket<PacketType>();\
}

#pragma region PacketHandler

#define REGISTER_HANDLER(PacketType)\
	RegisterPacketHandler<PacketType>();

#define DECLARE_HANDLE_PACKET(PacketType)\
	static bool HandlePacket(IOCPSession& session, PacketType& packet);\

#define REGISTER_ALL_HANDLER()\
	REGISTER_HANDLER(Ping)\
	REGISTER_HANDLER(RequestFileStream)\

#define DECLARE_ALL_HANDLER()\
	DECLARE_HANDLE_PACKET(Ping)\
	DECLARE_HANDLE_PACKET(RequestFileStream)\

#pragma endregion PacketHandler

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(Ping)\
	REGISTER_PACKET(RequestFileStream)\
}