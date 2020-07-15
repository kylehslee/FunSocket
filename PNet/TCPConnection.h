#pragma once
#include "Socket.h"
#include "PacketManager.h"

namespace PNet
{
	class TCPConnection
	{
	public:
		TCPConnection(Socket socket, IPEndPoint endpoint);
		TCPConnection() :socket(Socket()){}
		void Close();
		std::string ToString();
		Socket socket;

		PacketManager pm_incoming;
		PacketManager pm_outgoing;

		char buffer[PNet::g_MaxPacketSize];

	private:
		IPEndPoint endpoint;
		std::string stringRepresentation = "";
	};
}