#include "Packet.h"

namespace PNet 
{
	Packet::Packet(PacketType packetType)
	{
		this->Clear();
		this->AssignPacketType(packetType);
	}

	PacketType Packet::GetPacketType()
	{
		PacketType* packetTypePtr = reinterpret_cast<PacketType*>(&this->buffer[0]);
		return static_cast<PacketType>(ntohs(*packetTypePtr));
	}

	void Packet::AssignPacketType(PacketType packetType)
	{
		PacketType* packetTypePtr = reinterpret_cast<PacketType*>(&this->buffer[0]);
		*packetTypePtr = static_cast<PacketType>(htons(packetType));
	}

	void Packet::Clear()
	{
		this->buffer.resize(sizeof(PacketType));
		this->AssignPacketType(PacketType::PT_Invalid);
		this->extractionOffset = sizeof(PacketType);
	}
	 
	void Packet::Append(const void* data, uint32_t size)
	{
		if ((this->buffer.size() + size) > PNet::g_MaxPacketSize)
			throw PacketException("[Packet::Append(const void*, uint32_t)] - Packet size exceeded max packet size\n");

		this->buffer.insert(buffer.end(), (char*)data, (char*)data + size);
	}

	Packet& Packet::operator<<(uint32_t data)
	{
		data = htonl(data);
		this->Append(&data, sizeof(uint32_t));
		return *this;
	}

	Packet& Packet::operator>>(uint32_t& data)
	{
		if (this->extractionOffset + sizeof(uint32_t) > this->buffer.size())
			throw PacketException("[Packet::Operator >>(uint32_t &)] - ExtractionOffset size exceeded max buffer size\n");

		data = *reinterpret_cast<uint32_t*>(&this->buffer[this->extractionOffset]);
		data = ntohl(data);	
		this->extractionOffset += sizeof(uint32_t);
		return *this;
	}

	Packet& Packet::operator<<(const std::string& data)
	{
		*this << (uint32_t)data.size();
		this->Append(data.data(), data.size());
		return *this;
	}

	Packet& Packet::operator>>(std::string& data)
	{
		data.clear();

		uint32_t stringSize = 0;
		*this >> stringSize;

		if (this->extractionOffset + stringSize > this->buffer.size())
			throw PacketException("[Packet::Operator >>(std::string &) ] - ExtractionOffset size exceeded max buffer size\n");

		data.resize(stringSize);
		data.assign(&this->buffer[this->extractionOffset], stringSize);
		this->extractionOffset += stringSize;

		return *this;
	}	
}


