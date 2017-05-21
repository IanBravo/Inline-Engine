#pragma once

#include "Definitions.hpp"
#include "NetworkBuffer.hpp"
#include "NetworkMessage.hpp"

namespace inl::net::tcp
{
	class TcpClient
	{
	public:
		TcpClient(const SOCKET &socket);
		TcpClient(const std::string &ip, int port);
		~TcpClient();

		inline void Stop() { stopping = true; }
		bool DataAvailable(int &size);

	public:
		inline const std::string &GetIP() { return ip; }
		inline void SetIP(const std::string &ip) { this->ip = ip; }

		inline int GetPort() { return port; }
		inline void SetPort(int port) { this->port = port; }

	private:
		bool initialize(const std::string &ip, int port);

		NetworkBuffer receive_buffer();

		bool send_net_buffer(const NetworkBuffer &net_buffer);

	private:
		std::string ip;
		int port;

		bool connected;

		bool stopping;

		SOCKET soc;
		struct addrinfo *result;
		struct addrinfo hints;
	};
}