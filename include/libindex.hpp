#pragma once
#ifndef _CPP_RCON_LIB_INDEX_
#define _CPP_RCON_LIB_INDEX_

#include <iostream>
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

#ifdef DEBUG
#define LOG(x) std::cout << x << '\n'
#define LOGF(...) printf(__VA_ARGS__)
#define ERR(x) std::cerr << x << '\n'
#define ERRF(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(x)
#define LOGF(...)
#define ERR(x)
#define ERRF(...)
#endif

#define MAX_PACKET_LENGTH 4096 + sizeof(int) + 1
// The size in bytes of the packet id and packet type as well as the 2 null terminator bytes at the end of the packet.
// The length is not included in this calculation
#define PACKET_PADDING_SIZE sizeof(uint32_t) * 2 + 2

typedef struct
{
	std::string ip;
	uint16_t port;
} rcon_addr_t;

class Rcon {
private:
	rcon_addr_t _rcon_addr;
	int _rcon_socket;
	bool _connected;

	std::random_device _rd;
	std::mt19937 _rng;
	std::uniform_int_distribution<std::mt19937::result_type> _default_dist;

	bool _send_data(const std::string &data);
public:

	typedef enum {
		SERVERDATA_AUTH = 3,
		SERVERDATA_EXECCOMMAND = 2,
		SERVERDATA_AUTH_RESPONSE = 2,
		SERVERDATA_RESPONSE_VALUE = 0
	} PACKET_TYPE;

	Rcon(rcon_addr_t addr);
	~Rcon() {close();};
	
	/**
	 * @brief Establishes a connection to a remote RCON server.
	*/
	void connect();

	bool is_connected() {return this->_connected;}

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