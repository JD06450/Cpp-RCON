#pragma once
#ifndef _CPP_RCON_LOGGER_
#define _CPP_RCON_LOGGER_

#include <iostream>
#include <string>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <memory>
#include <type_traits>

enum class LOG_LEVEL
{
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	CRITICAL,
	FATAL
};

/**
 * @brief Converts an integer type into a string in hexadecimal format. This function also prepends a "0x" to the output.
 * @tparam T Any integer type. Used to define how long the resulting output string will be.
 * @param num The number to convert to hex.
 */
template <typename T>
std::string num_to_hex(T num)
{
	static_assert(std::is_arithmetic<T>::value && !std::is_floating_point<T>::value, "Type must be an integer type.");
	std::stringstream out_stream;
	out_stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << num;
	return out_stream.str();
}

/**
 * @brief Converts a byte or character to a string in hexadecimal format. This function also prepends a "0x" to the output.
 * @param num The number to convert to hex.
 */
template <>
inline std::string num_to_hex<uint8_t>(uint8_t num)
{
	std::stringstream out_stream;
	out_stream << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(num);
	return out_stream.str();
}

std::string pad_spaces(const std::string &input, size_t desired_length);

std::string trunc_zeros(const std::string &input, size_t num_digits);

std::string log_level_to_string(LOG_LEVEL level);

std::string log_level_to_escape_seq(LOG_LEVEL level);

class Logger
{
private:
	std::string _label;
	bool _should_print_header;

public:
	LOG_LEVEL log_level;

	Logger(
		std::string label,
		LOG_LEVEL init_level = LOG_LEVEL::WARNING) : _label(label),
													 log_level(init_level),
													 _should_print_header(true){};

	/**
	 * @brief Gets the current date and time as a formatted string
	 * @returns The current timestamp in the following format: `mm/dd/yyyy hh:mm:ss.mmm tz`
	 */
	std::string static get_timestamp();

	/**
	 * @brief Prints a log header to stdout. This header includes a trailing space.
	 * 
	 * The header is formatted like so:
	 * `[ timestamp ][ label ][ log level ]: `
	 * @param level The log level to display in the header.
	 */
	void print_header(LOG_LEVEL level);

	/**
	 * @brief Writes the specified output to stdout if the level meets or exceeds the current minimum logging level.
	 * If this function is called multiple times in succession, then only the first call will output a log header.
	 * @param level The logging level of the logged message.
	 * @param output The message to be logged.
	 */
	void print(LOG_LEVEL level, const std::string &output);

	/**
	 * @brief Same as @ref print but will add a newline character at the end of the output message.
	 * Can also be used to terminate a sequence of @ref print calls with a newline.
	 * @param level The logging level of the logged message.
	 * @param output The message to be logged.
	 */
	void println(LOG_LEVEL level, const std::string &output);

	/**
	 * @brief Runs the @ref println function with a logging level of `DEBUG`
	 * @param output The message to be logged.
	 */
	void debug(const std::string &output)
	{
		this->println(LOG_LEVEL::DEBUG, output);
	}

	/**
	 * @brief Runs the @ref println function with a logging level of `INFO`
	 * @param output The message to be logged.
	 */
	void info(const std::string &output)
	{
		this->println(LOG_LEVEL::INFO, output);
	}

	/**
	 * @brief Runs the @ref println function with a logging level of `WARN`
	 * @param output The message to be logged.
	 */
	void warn(const std::string &output)
	{
		this->println(LOG_LEVEL::WARNING, output);
	}

	/**
	 * @brief Runs the @ref println function with a logging level of `ERROR`
	 * @param output The message to be logged.
	 */
	void error(const std::string &output)
	{
		this->println(LOG_LEVEL::ERROR, output);
	}

	/**
	 * @brief Runs the @ref println function with a logging level of `CRITICAL`
	 * @param output The message to be logged.
	 */
	void critical(const std::string &output)
	{
		this->println(LOG_LEVEL::CRITICAL, output);
	}

	/**
	 * @brief Runs the @ref println function with a logging level of `FATAL`
	 * @param output The message to be logged.
	 */
	void fatal(const std::string &output)
	{
		this->println(LOG_LEVEL::FATAL, output);
	}
};

#endif