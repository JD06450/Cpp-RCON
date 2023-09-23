#include "logger.hpp"

std::string pad_spaces(const std::string &input, size_t desired_length)
{
	if (input.length() >= desired_length)
		return input;

	size_t zerosToAdd = desired_length - input.length();

	std::string padding(zerosToAdd, ' ');

	return padding + input;
}

std::string trunc_zeros(const std::string &input, size_t num_zeros)
{
	auto point_position = input.find('.');
	if (point_position == std::string::npos)
		return input;
	return input.substr(0, std::min(point_position + num_zeros, input.length()));
}

std::string log_level_to_string(LOG_LEVEL level)
{
	switch (level)
	{
	case LOG_LEVEL::DEBUG:
		return "DEBUG";
	case LOG_LEVEL::INFO:
		return "INFO";
	case LOG_LEVEL::ERROR:
		return "ERROR";
	case LOG_LEVEL::CRITICAL:
		return "CRITICAL";
	case LOG_LEVEL::FATAL:
		return "FATAL";
	case LOG_LEVEL::WARNING:
	default:
		return "WARNING";
	}
}

std::string log_level_to_escape_seq(LOG_LEVEL level)
{
	switch (level)
	{
	case LOG_LEVEL::DEBUG:
		// gray
		return "\033[38;5;248m";
	case LOG_LEVEL::WARNING:
		// yellow
		return "\033[38;5;220m";
	case LOG_LEVEL::ERROR:
		// red
		return "\033[38;5;9m";
	case LOG_LEVEL::CRITICAL:
		// dark red
		return "\033[38;5;124m";
	case LOG_LEVEL::FATAL:
		// white on red
		return "\033[37;41m";
	case LOG_LEVEL::INFO:
	default:
		// white
		return "\033[97m";
	}
}

std::string Logger::get_timestamp()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// Convert the time point to a time_t type (Unix timestamp)
	std::time_t current_time = std::chrono::system_clock::to_time_t(now);

	// Convert the time_t to a tm struct for formatting
	std::tm time_info;
	localtime_r(&current_time, &time_info);

	// Extract individual components (month, day, year, hour, minute, second, millisecond)
	int month = time_info.tm_mon + 1; // tm_mon is zero-based
	int day = time_info.tm_mday;
	int year = time_info.tm_year + 1900; // tm_year is years since 1900
	int hour = time_info.tm_hour;
	int minute = time_info.tm_min;
	int second = time_info.tm_sec;
	std::string time_zone(time_info.tm_zone);

	// Get the fractional part in milliseconds of the current time in seconds.
	long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

	// Format the time in this format: mm/dd/yyyy hh/mm/ss.mmm
	std::stringstream formatted_time;
	formatted_time << std::setfill('0');
	formatted_time << std::setw(2) << month << "/";
	formatted_time << std::setw(2) << day << "/";
	formatted_time << year << " ";
	formatted_time << std::setw(2) << hour << ":";
	formatted_time << std::setw(2) << minute << ":";
	formatted_time << std::setw(2) << second << ".";
	formatted_time << std::setw(3) << milliseconds << " ";
	formatted_time << time_zone;
	return formatted_time.str();
}

void Logger::print_header(LOG_LEVEL level)
{
	std::string time = Logger::get_timestamp();

	std::cout << "[ " << time << " ]"
			  << "[ " << this->_label << " ]"
			  << "[ \033[1m" << log_level_to_escape_seq(level) << log_level_to_string(level) << "\033[0m ]: ";
}

void Logger::print(LOG_LEVEL level, const std::string &output)
{
	if (level < this->log_level)
		return;
	if (this->_should_print_header)
		this->print_header(level);
	std::cout << output;
};

void Logger::println(LOG_LEVEL level, const std::string &output)
{
	if (level < this->log_level)
		return;
	std::string time = Logger::get_timestamp();

	if (this->_should_print_header)
		this->print_header(level);
	else
		this->_should_print_header = true;

	std::cout << output << '\n';
}