#include <libindex.hpp>

#include <vector>

std::string rcon_addr_t::to_string()
{
	return this->ip + ":" + std::to_string(this->port);
}

Rcon::Rcon(rcon_addr_t addr):
	_rcon_addr(addr),
	_connected(false),
	_logger(new Logger("RCON SESSION", LOG_LEVEL::DEBUG))
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
		this->_logger->error("Socket already connected to RCON server. Please disconnect before starting another connection.");
		return;
	}

	this->_rcon_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (this->_rcon_socket < 0)
	{
		this->_logger->error("Failed to create socket.");
		return;
	}

	struct sockaddr_in socket_address;

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(this->_rcon_addr.port);
	socket_address.sin_addr.s_addr = inet_addr(this->_rcon_addr.ip.c_str());

	int connect_status = ::connect(this->_rcon_socket, (struct sockaddr *) &socket_address, sizeof(socket_address));
	this->_logger->debug("CONNECT STATUS: " + std::to_string(connect_status));
	this->_logger->debug("ERRNO = " + std::to_string(errno) + ": " + strerror(errno));

	if (connect_status == -1 && errno != EINPROGRESS)
	{
		this->_logger->error("Failed to connect to the RCON server.");
		::close(this->_rcon_socket);
		return;
	}

	fcntl(this->_rcon_socket, F_SETFL, O_NONBLOCK);

	struct timeval select_timeout;
	select_timeout.tv_sec = 2;
	select_timeout.tv_usec = 0;

	// Define our set of file descriptors, zero it out, and add our socket to the set.

	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(this->_rcon_socket, &write_set);

	int ready = select(this->_rcon_socket + 1, NULL, &write_set, NULL, &select_timeout);
	this->_logger->debug("RCON socket select status (write): " + std::to_string(ready));
	if (ready == 0) {
		this->_logger->error("Socket timed out whilst waiting for connection.");
		return;
	}
	this->_connected = (ready == 1);
}

bool Rcon::authenticate(std::string &server_password)
{
	if (!this->_connected)
	{
		this->_logger->error("Socket not currently connected. Cannot authenticate.");
		return false;
	}
	int packet_id = this->_default_dist(this->_rng);

	std::string auth_packet;
	uint32_t packet_len = server_password.length() + PACKET_PADDING_SIZE;
	auth_packet += int_to_le_string(packet_len);
	auth_packet += int_to_le_string(packet_id);
	auth_packet += int_to_le_string((int32_t) PACKET_TYPE::SERVERDATA_AUTH);
	auth_packet += server_password;
	auth_packet += '\0';
	auth_packet += '\0';

	bool success = this->_send_data(auth_packet);
	if (success) this->_logger->info("Authentication successful.");
	else         this->_logger->error("Authentication failed.");
	printf("auth packet success: %s\n", (success ? "true" : "false"));
	std::map<uint32_t, std::vector<std::string>> data = this->get_pending_data();

	if (data.find(packet_id) != data.end() && data.at(packet_id).size() == 2)
	{
		this->_logger->info("Successfully authenticated to the remote RCON server at " + this->_rcon_addr.to_string());
		return true;
	}
	this->_logger->error("Failed to authenticate with the remote RCON server.");
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
			this->_logger->error("LIBC \"select\" error (" + std::to_string(errno) + "): " + strerror(errno));
			this->_logger->error("Ran out of tries. Automatically disconnecting socket.");
			close();
			break;
		}
		else if (ready == -1)
		{
			this->_logger->error("LIBC \"select\" error (" + std::to_string(errno) + "): " + strerror(errno));
			this->_logger->info("Trying again to read socket.");
			tries++;
			continue;
		}

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
		incoming_packets.at(packet_id).push_back(std::string(read_buff + sizeof(uint32_t), le32toh(packet_length)));
	}

	if (!ready && !num_packets) {

		this->_logger->warn("Timeout limit reached.");
		if (++this->_failed_packets == 3) {
			this->_logger->error("Too many failed packets. Closing connection...");
			this->close();
		}
	} else {
		this->_failed_packets = 0;
	}
	
	this->_logger->debug("Successfully read " + std::to_string(num_packets) + (num_packets == 1 ? " packet." : " packets."));
	return incoming_packets;
}

void Rcon::get_socket_status() {
	int error = 0;
	socklen_t len = sizeof(error);
	int result = getsockopt(this->_rcon_socket, SOL_SOCKET, SO_ERROR, &error, &len);
	this->_logger->debug("get_socket_status result: " + std::to_string(result) + " | error: " + std::to_string(error));
}

std::string Rcon::send_command(const std::string &command, Rcon::PACKET_TYPE packet_type)
{
	this->get_socket_status();
	if (!this->_connected)
	{
		this->_logger->error("Socket not currently connected. Socket must be connected to send data.");
		return "";
	}

	int32_t packet_id = this->_default_dist(this->_rng);
	packet_id = abs(packet_id);

	uint32_t packet_length = command.length() + PACKET_PADDING_SIZE;

	std::string packet = int_to_le_string(packet_length);
	packet += int_to_le_string(packet_id);
	packet += int_to_le_string((int32_t) packet_type);
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
	this->_logger->print(LOG_LEVEL::DEBUG, "RESPONSE: ");
	for (auto ch : final_data)
	{
		this->_logger->print(LOG_LEVEL::DEBUG, num_to_hex<uint8_t>(ch));
	}

	if (final_data.length() == 0) this->_logger->print(LOG_LEVEL::DEBUG, "(no response)");
	this->_logger->println(LOG_LEVEL::DEBUG, "");
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
			this->_logger->error("LIBC \"select\" error (" + std::to_string(errno) + "): " + strerror(errno));
			return false;
		}
		else if (ready == 0 && tries == 2)
		{
			this->_logger->error("Failed to send data. (Socket timed out)");
			return false;
		}
		else if (ready == 0)
		{
			this->_logger->warn("RCON Socket timed out. Trying again...");
			tries++;
			continue;
		}
	}

	int bytes_sent = 0;
	tries = 0;

	do {
		bytes_sent = ::send(this->_rcon_socket, data.c_str(), data.length(), 0);
		if (bytes_sent < 0)
		{
			this->_logger->error("LIBC \"send\" error (" + std::to_string(errno) + "): " + strerror(errno));
			usleep(20000);
			tries++;
		}
	} while (bytes_sent < 0 && tries < 3);
	return bytes_sent > 0;
}

std::string int_to_le_string(uint32_t number)
{
	char buffer[sizeof(uint32_t)];
	number = htole32(number);
	std::memcpy(&buffer, &number, sizeof(uint32_t));
	return std::string(&buffer[0], sizeof(uint32_t));
}