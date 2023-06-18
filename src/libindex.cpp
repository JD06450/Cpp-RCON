#include <libindex.hpp>

#include <vector>

Rcon::Rcon(rcon_addr_t addr):
_rcon_addr(addr),
_connected(false)
{
	// initialize the random number generator for picking the random packet ID.
	this->_rng = std::mt19937(this->_rd());
	this->_default_dist = std::uniform_int_distribution<std::mt19937::result_type>(1, __INT32_MAX__);
};

std::string int_to_le_string(uint32_t);

void Rcon::connect()
{
	if (this->_connected)
	{
		std::cerr << "Socket already connected to RCON server. Please disconnect before starting another connection.\n";
		return;
	}

	this->_rcon_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (this->_rcon_socket < 0)
	{
		std::cerr << "Failed to create socket!" << std::endl;
		return;
	}

	struct sockaddr_in socket_address;

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(this->_rcon_addr.port);
	socket_address.sin_addr.s_addr = inet_addr(this->_rcon_addr.ip.c_str());

	fcntl(this->_rcon_socket, F_SETFL, O_NONBLOCK);

	if (::connect(this->_rcon_socket, (struct sockaddr*) &socket_address, sizeof(socket_address)) == -1)
	{
		if (errno != EINPROGRESS) {
			std::cerr << "Failed to connect to the RCON server.\n";
			::close(this->_rcon_socket);
			return;
		}
	}


	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(this->_rcon_socket, &write_set);

	int status = select(this->_rcon_socket + 1, NULL, &write_set, NULL, &timeout);
	LOGF("[DEBUG]: RCON Socket select status (write): %i\n", status);
	this->_connected = status == 1;
}

bool Rcon::authenticate(std::string &server_password)
{
	if (!this->_connected)
	{
		std::cerr << "Socket not currently connected. Cannot authenticate.\n";
		return false;
	}
	int packet_id = this->_default_dist(this->_rng);

	std::string auth_packet;
	uint32_t packet_len = server_password.length() + PACKET_PADDING_SIZE;
	auth_packet += int_to_le_string(packet_len);
	auth_packet += int_to_le_string(packet_id);
	auth_packet += int_to_le_string(PACKET_TYPE::SERVERDATA_AUTH);
	auth_packet += server_password;
	auth_packet += '\0';
	auth_packet += '\0';

	bool success = this->_send_data(auth_packet);
	printf("auth packet success: %s\n", (success ? "true" : "false"));
	std::map<uint32_t, std::vector<std::string>> data = this->get_pending_data();

	if (data.find(packet_id) != data.end() && data.at(packet_id).size() == 2)
	{
		printf("Successfully authenticated to the remote RCON server at %s:%hu\n", this->_rcon_addr.ip.c_str(), this->_rcon_addr.port);
		return true;
	}
	return false;
}

std::map<uint32_t, std::vector<std::string>> Rcon::get_pending_data()
{
	std::map<uint32_t, std::vector<std::string>> incoming_packets;
	if (!this->_connected) return incoming_packets;

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(this->_rcon_socket, &read_set);

	struct timeval time_set;
	time_set.tv_sec = 0;
	time_set.tv_usec = 100 * 1000;

	int ready;
	int num_packets = 0;
	int tries = 0;

	while ((ready = select(this->_rcon_socket + 1, &read_set, NULL, NULL, &time_set))/* || tries < 3*/)
	{
		if (ready == -1 && tries == 2)
		{
			std::cerr << "Error reading the socket\nRan out of tries. Automatically disconnecting socket.\n";
			close();
			break;
		}
		else if (ready == -1)
		{
			std::cerr << "Error reading the socket. Trying again...\n";
			tries++;
			continue;
		}

		// if (!ready && tries == 2)
		// {
		// 	std::cerr << "Error reading the socket\nSocket timed out.\n";
		// 	break;
		// }
		// else if (!ready)
		// {
		// 	std::cerr << "Error reading the socket.\nSocket timed out.\nTrying again...\n";
		// 	tries++;
		// 	continue;
		// }

		char read_buff[MAX_PACKET_LENGTH];
		::recv(this->_rcon_socket, &read_buff, sizeof(read_buff), 0);
		num_packets++;

		uint32_t packet_id;
		std::memcpy(&packet_id, read_buff + sizeof(uint32_t), sizeof(uint32_t));
		packet_id = le32toh(packet_id);
		
		if (incoming_packets.find(packet_id) == incoming_packets.end())
		{
			incoming_packets.emplace(packet_id, std::vector<std::string>());
		}
		uint32_t packet_length;
		std::memcpy(&packet_length, read_buff, sizeof(uint32_t));
		incoming_packets.at(packet_id).push_back(std::string(read_buff, le32toh(packet_length) + sizeof(uint32_t)));
	}

	if (!ready && !num_packets) std::cerr << "Timeout limit reached.\n";
	
	LOG("[DEBUG]: Successfully read: " << num_packets << (num_packets == 1 ? " packet" : " packets"));
	return incoming_packets;
}

std::string Rcon::send_command(const std::string &command, Rcon::PACKET_TYPE type)
{
	if (!this->_connected)
	{
		std::cout << "Socket not currently connected. Socket must be connected to send data.\n";
		return "";
	}

	uint32_t packet_id = this->_default_dist(this->_rng);

	uint32_t packet_length = command.length() + PACKET_PADDING_SIZE;

	std::string packet = int_to_le_string(packet_length);
	packet += int_to_le_string(packet_id);
	packet += int_to_le_string(type);
	packet += command;
	packet += '\x00';
	packet += '\x00';

	bool success = this->_send_data(packet);
	if (!success) return "";

	std::map<uint32_t, std::vector<std::string>> received = get_pending_data();
	if (received.find(packet_id) == received.end()) return "";

	std::vector<std::string> data_chunks = received.at(packet_id);
	std::string final_data = "";
	for (auto chunk : data_chunks)
	{
		chunk = chunk.substr(sizeof(uint32_t) * 2);
		final_data += chunk.substr(0, chunk.length() - 2);
	}

	return final_data;
}

bool Rcon::_send_data(const std::string &data)
{
	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(this->_rcon_socket, &write_set);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	int tries = 0;
	int ready = 0;

	while (ready != 1 && tries < 3)
	{
		ready = select(this->_rcon_socket + 1, NULL, &write_set, NULL, &timeout);
		if (ready == -1)
		{
			LOGF("[DEBUG]: RCON Socket \"select\" error: %s (%i)\n", strerror(errno), errno);
			ERRF("[ERROR]: %s (%i)\n", strerror(errno), errno);
			return false;
		}
		else if (ready == 0 && tries == 2)
		{
			LOG("[DEBUG]: RCON Socket timed out. Out of tries.");
			ERR("[ERROR]: Failed to send data. (Socket timed out)");
			return false;
		}
		else if (ready == 0)
		{
			LOG("[DEBUG]: RCON Socket timed out. Trying again...");
			tries++;
			continue;
		}
	}

	int status = 0;
	tries = 0;

	do {
		status = send(this->_rcon_socket, data.c_str(), data.length(), 0);
		if (status < 0)
		{
			LOGF("[DEBUG]: RCON Socket \"send\" error: %s (%i)\n", strerror(errno), errno);
			ERRF("[ERROR]: %s (%i)\n", strerror(errno), errno);
			usleep(20000);
			tries++;
		}
	} while (status < 0 && tries < 3);
	return status > 0;
}

std::string int_to_le_string(uint32_t number)
{
	char buffer[sizeof(uint32_t)];
	number = htole32(number);
	std::memcpy(&buffer, &number, sizeof(uint32_t));
	return std::string(&buffer[0], sizeof(uint32_t));
}