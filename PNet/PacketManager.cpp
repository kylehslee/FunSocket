#include "PacketManager.h"


namespace PNet
{
	void PacketManager::Clear()
	{
		this->packets = std::queue<std::shared_ptr<Packet>>();
	}

	bool PacketManager::HasPendingPacket()
	{
		return (!this->packets.empty());
	}

	void PacketManager::Append(std::shared_ptr<Packet> p)
	{
		this->packets.push(std::move(p));
	}

	std::shared_ptr<Packet> PacketManager::Retrieve()
	{
		std::shared_ptr<Packet> p = this->packets.front();
		return p;
	}

	void PacketManager::Pop()
	{
		this->packets.pop();
	}

}
