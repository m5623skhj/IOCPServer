#pragma once
#include <functional>
#include <any>
#include <map>
#include <memory>
#include "Protocol.h"

class IOCPSession;

using PacketHandler = std::function<bool(IOCPSession&, std::any&)>;
using PacketFactoryFunction = std::function<std::shared_ptr<IPacket>()>;

class PacketManager
{
private:
	PacketManager() = default;
	~PacketManager() = default;

	PacketManager(const PacketManager&) = delete;
	PacketManager& operator=(const PacketManager&) = delete;

public:
	static PacketManager& GetInst();
	std::shared_ptr<IPacket> MakePacket(PacketId packetId);
	PacketHandler GetPacketHandler(PacketId packetId);

	void Init();

#pragma region PacketRegister
public:
	template <typename PacketType>
	void RegisterPacket()
	{
		static_assert(std::is_base_of<IPacket, PacketType>::value, "RegisterPacket() : PacketType must inherit from IPacket");
		PacketFactoryFunction factoryFunc = []()
		{
			return std::make_shared<PacketType>();
		};

		PacketType packetType;
		packetFactoryFunctionMap[packetType.GetPacketId()] = factoryFunc;
	}

	template <typename PacketType>
	void RegisterPacketHandler()
	{
		static_assert(std::is_base_of<IPacket, PacketType>::value, "RegisterPacketHandler() : PacketType must inherit from IPacket");
		auto handler = [](IOCPSession& session, std::any& packet)
		{
			auto realPacket = static_cast<PacketType*>(std::any_cast<IPacket*>(packet));
			return HandlePacket(session, *realPacket);
		};

		PacketType packetType;
		packetHandlerMap[packetType.GetPacketId()] = handler;
	}

	std::map<PacketId, PacketFactoryFunction> packetFactoryFunctionMap;
	std::map<PacketId, PacketHandler> packetHandlerMap;
#pragma endregion PacketRegister

#pragma region PakcetHandler
public:
	// Declare Packet Handler()
#pragma endregion PakcetHandler
};