#pragma once
#include <PNet/includeMe.h>

using namespace std;

class MyServer : public Server
{
private:
	void OnConnect(TCPConnection& newConnection) override;
	void OnDisconnect(TCPConnection& lostConnection, std::string reason) override;
	bool ProcessPacket(std::shared_ptr<Packet> packet, std::string& ip) override;
};
