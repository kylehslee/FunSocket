#include "TCPConnection.h"


namespace PNet
{
	TCPConnection::TCPConnection(Socket socket, IPEndPoint endpoint)
		: socket(socket), endpoint(endpoint)
	{
		this->stringRepresentation = "[" + this->endpoint.getIpString() + ":" + std::to_string(this->endpoint.getPort()) + "]";
	}

	void TCPConnection::Close()
	{
		this->socket.Close();
	}

	std::string TCPConnection::ToString()
	{
		return this->stringRepresentation;
	}

}

