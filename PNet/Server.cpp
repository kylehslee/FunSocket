#include "Server.h"
#include "Network.h"

bool Server::Initialize(IPEndPoint ip)
{
	std::cout << "SHIBAL22333" << std::endl;

	this->master_fd.clear();
	this->connections.clear();

	std::cout << "Winsock successfully initialized!\n";

	this->listeningSocket = Socket(ip.getIpversion());

	if (this->listeningSocket.Create() == PResult::P_Success)
	{
		std::cout << "Socket created!\n";

		if (this->listeningSocket.Listen(ip) == PResult::P_Success)
		{
			WSAPOLLFD listeningSocketFD = {};
			listeningSocketFD.fd = this->listeningSocket.GetHandle();
			listeningSocketFD.events = POLLRDNORM;
			listeningSocketFD.revents = 0;

			this->master_fd.push_back(listeningSocketFD);

			std::cout << "Bind Success Listening on port 4790!" << std::endl;
			return true;
		}
		else
		{
			std::cerr << "Fail to listen!" << std::endl;
		}
		this->listeningSocket.Close();
	}
	else
	{
		std::cerr << "Socket failed to create WHy?!\n";
	} 
	
	return false;
}

void Server::Frame()
{
	for (int i = 0; i < this->connections.size(); i++)
	{
		if (this->connections[i].pm_outgoing.HasPendingPacket())
		{
			this->master_fd[i + 1].events = POLLRDNORM | POLLWRNORM;
		}
	}

	this->use_fd = this->master_fd;
	if (WSAPoll(use_fd.data(), use_fd.size(), 100) > 0)
	{
#pragma region listener
		WSAPOLLFD& listeningSocketFD = use_fd[0];
		if (listeningSocketFD.revents & POLLRDNORM)
		{
			Socket newConnectionSocket;
			IPEndPoint newConnectionEndpoint;
			if (this->listeningSocket.Accept(newConnectionSocket, &newConnectionEndpoint) == PResult::P_Success)
			{
				this->connections.emplace_back(TCPConnection(newConnectionSocket, newConnectionEndpoint));
				TCPConnection& acceptedConnection = this->connections[this->connections.size() - 1];

				WSAPOLLFD newConnectionFD = {};
				newConnectionFD.fd = newConnectionSocket.GetHandle();
				newConnectionFD.events = POLLRDNORM;
				newConnectionFD.revents = 0;

				this->master_fd.push_back(newConnectionFD);

				this->OnConnect(acceptedConnection);

			}
			else
			{
				std::cerr << "Fail to accept new connection" << std::endl;
			}
		}
#pragma endregion Code specific	to the listening socket

		for (int i = use_fd.size() - 1; i >= 1; i--)
		{
			int connectionIndex = i - 1;
			TCPConnection& connection = this->connections[connectionIndex];

			if (use_fd[i].revents & POLLERR)
			{
				this->CloseConnection(connectionIndex, "POLLERR");
				continue;
			}

			if (use_fd[i].revents & POLLHUP)
			{
				this->CloseConnection(connectionIndex, "POLLHUP");
				continue;
			}

			if (use_fd[i].revents & POLLNVAL)
			{
				this->CloseConnection(connectionIndex, "POLLNVAL");
				continue;
			}

			if (use_fd[i].revents & POLLRDNORM)
			{

				int bytesReceived = 0;

				if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
				{
					bytesReceived = recv(use_fd[i].fd, (char*)&connection.pm_incoming.currentPacketSize + connection.pm_incoming.currentPacketExtractionOffset, sizeof(uint16_t) - connection.pm_incoming.currentPacketExtractionOffset, 0);
				}
				else //Process Packet Contents
				{
					bytesReceived = recv(use_fd[i].fd, (char*)&connection.buffer + connection.pm_incoming.currentPacketExtractionOffset, connection.pm_incoming.currentPacketSize - connection.pm_incoming.currentPacketExtractionOffset, 0);
				}

				if (bytesReceived == 0) //If connection was lost
				{
					CloseConnection(connectionIndex, "Recv==0");
					continue;
				}

				if (bytesReceived == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					if (error != WSAEWOULDBLOCK)
					{
						this->CloseConnection(connectionIndex, "RECV<0");
						continue;
					}
				}
				if (bytesReceived > 0)
				{
					connection.pm_incoming.currentPacketExtractionOffset += bytesReceived;

					if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
					{
						if (connection.pm_incoming.currentPacketExtractionOffset == sizeof(uint16_t))
						{
							connection.pm_incoming.currentPacketSize = ntohs(connection.pm_incoming.currentPacketSize);

							if (connection.pm_incoming.currentPacketSize > PNet::g_MaxPacketSize)
							{
								this->CloseConnection(connectionIndex, "PacketSize too large");
								continue;
							}
							connection.pm_incoming.currentPacketExtractionOffset = 0;
							connection.pm_incoming.currentTask = PacketManagerTask::ProcessPacketContents;
						}
					}
					else
					{
						if (connection.pm_incoming.currentPacketExtractionOffset == connection.pm_incoming.currentPacketSize)
						{
							std::shared_ptr<Packet> packet = std::make_shared<Packet>();
							packet->buffer.resize(connection.pm_incoming.currentPacketSize);
							memcpy(&packet->buffer[0], connection.buffer, connection.pm_incoming.currentPacketSize);

							connection.pm_incoming.Append(packet);

							connection.pm_incoming.currentPacketSize = 0;
							connection.pm_incoming.currentPacketExtractionOffset = 0;
							connection.pm_incoming.currentTask = PacketManagerTask::ProcessPacketSize;
						}
					}
				}
			}

			if (use_fd[i].revents & POLLWRNORM) //If normal data can be written without blocking
			{
				PacketManager& pm = connection.pm_outgoing;
				while (pm.HasPendingPacket())
				{
					if (pm.currentTask == PacketManagerTask::ProcessPacketSize) //Sending packet size
					{
						pm.currentPacketSize = pm.Retrieve()->buffer.size();
						uint16_t bigEndianPacketSize = htons(pm.currentPacketSize);
						int bytesSent = send(use_fd[i].fd, (char*)(&bigEndianPacketSize) + pm.currentPacketExtractionOffset, sizeof(uint16_t) - pm.currentPacketExtractionOffset, 0);
						if (bytesSent > 0)
						{
							pm.currentPacketExtractionOffset += bytesSent;
						}

						if (pm.currentPacketExtractionOffset == sizeof(uint16_t)) //If full packet size was sent
						{
							pm.currentPacketExtractionOffset = 0;
							pm.currentTask = PacketManagerTask::ProcessPacketContents;
						}
						else //If full packet size was not sent, break out of the loop for sending outgoing packets for this connection - we'll have to try again next time we are able to write normal data without blocking
						{
							break;
						}
					}
					else //Sending packet contents
					{
						char* bufferPtr = &pm.Retrieve()->buffer[0];
						int bytesSent = send(use_fd[i].fd, (char*)(bufferPtr)+pm.currentPacketExtractionOffset, pm.currentPacketSize - pm.currentPacketExtractionOffset, 0);
						if (bytesSent > 0)
						{
							pm.currentPacketExtractionOffset += bytesSent;
						}

						if (pm.currentPacketExtractionOffset == pm.currentPacketSize) //If full packet contents have been sent
						{
							pm.currentPacketExtractionOffset = 0;
							pm.currentTask = PacketManagerTask::ProcessPacketSize;
							pm.Pop(); //Remove packet from queue after finished processing
						}
						else
						{
							break;
						}
					}
				}
				if (!pm.HasPendingPacket())
				{
					this->master_fd[i].events = POLLRDNORM;
				}
			}
		}
		for (int i = this->connections.size() - 1; i >= 0; i--)
		{
			while (this->connections[i].pm_incoming.HasPendingPacket())
			{
				std::shared_ptr<Packet> frontPacket = this->connections[i].pm_incoming.Retrieve();
				if (!this->ProcessPacket(frontPacket))
				{
					this->CloseConnection(i, "Failed to process incoming packet");
					break;
				}
				this->connections[i].pm_incoming.Pop();
			}
		}
	}
}

void Server::OnConnect(TCPConnection& newConnection)
{
	std::cout << newConnection.ToString() << " - New connection accepted" << std::endl;	
}

void Server::OnDisconnect(TCPConnection& lostConnection, std::string reason)
{
	std::cout << "[" << reason << "] Connection lost: " << lostConnection.ToString() << std::endl;
}

void Server::CloseConnection(int connectionIndex, std::string reason)
{
	TCPConnection& connection = connections[connectionIndex];
	this->OnDisconnect(connection, reason);

	master_fd.erase(master_fd.begin() + (connectionIndex + 1));
	use_fd.erase(use_fd.begin() + (connectionIndex + 1));

	connection.Close();
	connections.erase(connections.begin() + connectionIndex);
}

bool Server::ProcessPacket(std::shared_ptr<Packet> packet)
{
	std::cout << "Packet received with size: " << packet->buffer.size() << std::endl;
	return true;
}
