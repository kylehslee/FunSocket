#pragma once

#include "Packet.h"

#include <queue>
#include <memory>
#include <iostream>

namespace PNet
{
	enum PacketManagerTask
	{
		ProcessPacketSize,
		ProcessPacketContents
	};

	class PacketManager
	{
	private:
		std::queue<std::shared_ptr<Packet>> packets;

	public:
		void Clear();
		bool HasPendingPacket();
		void Append(std::shared_ptr<Packet> p);
		std::shared_ptr<Packet> Retrieve();
		void Pop();

		void HowMany();

		uint16_t currentPacketSize = 0;
		int currentPacketExtractionOffset = 0;
		PacketManagerTask currentTask = PacketManagerTask::ProcessPacketSize;
	};
}