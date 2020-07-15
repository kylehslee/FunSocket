#pragma once
#define WIN32_LEAN_AND_MEAN

#include "Constants.h"
#include "PacketException.h"
#include "PacketType.h"

#include <WinSock2.h>

#include <vector>
#include <string>

namespace PNet
{
	class Packet
	{
	private:
		

	public:
		
		Packet(PacketType packetType = PacketType::PT_Invalid);
		PacketType GetPacketType();
		void AssignPacketType(PacketType packetType);

		void Clear();
		void Append(const void* data, uint32_t size);

		Packet& operator << (uint32_t data);
		Packet& operator >> (uint32_t& data);

		Packet& operator << (const std::string& data);
		Packet& operator >> (std::string& data);

		uint32_t extractionOffset = 0;
		std::vector<char> buffer;

	};
}