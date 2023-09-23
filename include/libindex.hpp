#pragma once
#ifndef _CPP_RCON_LIB_INDEX_
#define _CPP_RCON_LIB_INDEX_

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <random>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#include "logger.hpp"

/**
 * @def MAX_PACKET_LENGTH
 * @brief The maximum length of a single RCON packet.
 * 
 * This specifies the max value that the packet size field can have.
 * The maximum length that a single RCON packet can be is 4096 bytes, excluding the packet size field.
 * The minimum length of an RCON packet is 10 bytes. Four for the packet id, four for the packet type,
 * and 2 null bytes (`0x00`) at the end, one to signify an empty body and another to signify the end of the packet.
*/
#define MAX_PACKET_LENGTH 4096
/**
 * @def PACKET_PADDING_SIZE
 * @brief The size of all the fields surrounding the packet body that are included in calculating the total size of the packet.
 * 
 * (i.e. the packet id and type fields at the beginning, as well as the 2 terminator bytes at the end)
*/
#define PACKET_PADDING_SIZE sizeof(int32_t) * 2 + 2

typedef struct
{
	std::string ip;
	uint16_t port;

	std::string to_string();
} rcon_addr_t;

class Rcon {
private:
	std::unique_ptr<Logger> _logger;
	rcon_addr_t _rcon_addr;
	int _rcon_socket;
	/// Whether or not the \ref _rcon_socket is connected to an RCON server
	bool _connected;
	/// The number of consecutive failed packets
	int _failed_packets = 0;

	std::random_device _rd;
	std::mt19937 _rng;
	std::uniform_int_distribution<std::mt19937::result_type> _default_dist;

	bool _send_data(const std::string &data);
public:

	enum class PACKET_TYPE {
		SERVERDATA_AUTH = 3,
		SERVERDATA_EXECCOMMAND = 2,
		SERVERDATA_AUTH_RESPONSE = 2,
		SERVERDATA_RESPONSE_VALUE = 0
	};

	Rcon(rcon_addr_t addr);
	~Rcon() {close();};
	
	/**
	 * @brief Establishes a connection to a remote RCON server.
	*/
	void connect();

	bool is_connected() const {return this->_connected;}

	/**
	 * @brief Authenticates with the RCON server
	 * @returns Whether the authentication was successful
	*/
	bool authenticate(std::string &server_password);

	std::string send_command(const std::string &command, PACKET_TYPE type = PACKET_TYPE::SERVERDATA_EXECCOMMAND);

	/**
	 * @brief Will retrieve any data packets waiting to be read by the socket.
	 * @returns All of the retrieved data separated by the associated packet ID.
	*/
	std::map<uint32_t, std::vector<std::string>> get_pending_data();

	void get_socket_status();

	/**
	 * @brief Will close the active socket.
	 */
	void close() {
		if (this->_connected)
		{
			::close(this->_rcon_socket);
			puts("Connection closed.");
		}
		this->_connected = false;
	};
};

#endif // _CPP_RCON_LIB_INDEX