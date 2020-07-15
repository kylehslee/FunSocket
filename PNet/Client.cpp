#include "Client.h"
#include <iostream>

namespace PNet
{
	bool Client::Connect(IPEndPoint ip)
	{
		isConnected = false;
		
		Socket socket = Socket(ip.getIpversion());
		if (socket.Create() == PResult::P_Success)
		{
			if (socket.SetBlocking(true) != PResult::P_Success)
				return false;

			std::cout << "Socket successfully created." << std::endl;
			if (socket.Connect(ip) == PResult::P_Success)
			{
				if (socket.SetBlocking(false) == PResult::P_Success)
				{
					this->connection = TCPConnection(socket, ip);
					this->master_fd.fd = connection.socket.GetHandle();
					this->master_fd.events = POLLRDNORM;
					this->master_fd.revents = 0;
					 
					isConnected = true;
					this->OnConnect();
					return true;
				}
			}
			else
			{
				std::cerr << "Failed to connect to server." << std::endl;
			}
			socket.Close();
		}
		else
		{
			std::cerr << "Socket failed to create. ??" << std::endl;
		}
		this->OnConnectFail();
		 
		return false;
	}

	bool Client::IsConnected()
	{
		return isConnected;
	}

	bool Client::Frame()
	{
		if (connection.pm_outgoing.HasPendingPacket())
		{
			master_fd.events = POLLRDNORM | POLLWRNORM;
		}

		this->use_fd = this->master_fd;

		if (WSAPoll(&this->use_fd, 1, 1) > 0)
		{

				if (use_fd.revents & POLLERR)
				{
					CloseConnection("POLLERR");
					return false;
				}

				if (use_fd.revents & POLLHUP)
				{
					CloseConnection("POLLHUP");
					return false;
				}

				if (use_fd.revents & POLLNVAL)
				{
					CloseConnection("POLLNVAL");
					return false;
				}

				if (use_fd.revents & POLLRDNORM)
				{

					int bytesReceived = 0;

					if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
					{
						bytesReceived = recv(use_fd.fd, (char*)&connection.pm_incoming.currentPacketSize + connection.pm_incoming.currentPacketExtractionOffset, sizeof(uint16_t) - connection.pm_incoming.currentPacketExtractionOffset, 0);
					}
					else //Process Packet Contents
					{
						bytesReceived = recv(use_fd.fd, (char*)&connection.buffer + connection.pm_incoming.currentPacketExtractionOffset, connection.pm_incoming.currentPacketSize - connection.pm_incoming.currentPacketExtractionOffset, 0);
					}


					if (bytesReceived == 0) //If connection was lost
					{
						CloseConnection("Recv==0");
						return false;
					}


					if (bytesReceived == SOCKET_ERROR)
					{
						int error = WSAGetLastError();
						if (error != WSAEWOULDBLOCK)
						{
							CloseConnection( "RECV<0");
							return false;
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
									CloseConnection( "PacketSize too large");
									return false;
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

				if (use_fd.revents & POLLWRNORM) //If normal data can be written without blocking
				{

					PacketManager& pm = connection.pm_outgoing;
					while (pm.HasPendingPacket())
					{
						if (pm.currentTask == PacketManagerTask::ProcessPacketSize) //Sending packet size
						{
							pm.currentPacketSize = pm.Retrieve()->buffer.size();
							uint16_t bigEndianPacketSize = htons(pm.currentPacketSize);
							int bytesSent = send(use_fd.fd, (char*)(&bigEndianPacketSize) + pm.currentPacketExtractionOffset, sizeof(uint16_t) - pm.currentPacketExtractionOffset, 0);
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
							int bytesSent = send(use_fd.fd, (char*)(bufferPtr)+pm.currentPacketExtractionOffset, pm.currentPacketSize - pm.currentPacketExtractionOffset, 0);
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
								break; //Added after tutorial was made 2019-06-24
							}
						}
					}
					if (!connection.pm_outgoing.HasPendingPacket())
					{
						master_fd.events = POLLRDNORM;
					}
				}
			}

			while (this->connection.pm_incoming.HasPendingPacket())
			{
				std::shared_ptr<Packet> frontPacket = this->connection.pm_incoming.Retrieve();
				if (!this->ProcessPacket(frontPacket))
				{
					this->CloseConnection("Failed to process incoming packet");
					return false;
				}
				this->connection.pm_incoming.Pop();
			}
	}

	bool Client::ProcessPacket(std::shared_ptr<Packet> packet)
	{
		std::cout << "Packet received with size: " << packet->buffer.size() << std::endl;
		return true;
	}

	void Client::OnConnect()
	{
		std::cout << "Successfully connected!" << std::endl;
	}

	void Client::OnConnectFail()
	{
		std::cout << "Failed to connect." << std::endl;
	}

	void Client::OnDisconnect(std::string reason)
	{
		std::cout << "Lost connection. Reason: " << reason << "." << std::endl;
	}

	void Client::CloseConnection(std::string reason)
	{
		OnDisconnect(reason);
		master_fd.fd = 0;
		isConnected = false;
		connection.Close();
	}
}

