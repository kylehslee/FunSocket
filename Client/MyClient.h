#pragma once
#include <PNet/includeMe.h>

class MyClient : public Client
{
	bool ProcessPacket(std::shared_ptr<Packet> packet) override;
	void OnConnect() override;
	void SendText() override;

	std::string StringIP = this->connection.ToString();
};