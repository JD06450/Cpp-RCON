#include <index.hpp>

namespace po = boost::program_options;

std::string help_text =
	"Usage: open-rcom [OPTIONS]\n\n"
	
	"	Connects to a remote RCON server and opens an interactive console\n"
	"	which takes input from stdin.\n\n";

bool is_IPv4(const std::string& str) {
	std::regex ipv4_regex(R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))");
	std::smatch match;
	
	if (!std::regex_match(str, match, ipv4_regex)) {
		return false;
	}

	for (int i = 1; i <= 4; ++i) {
		int num = std::stoi(match[i]);
		if (num < 0 || num > 255) {
			return false;
		}
	}
	
	return true;
}

bool attempt_reconnect(Rcon *session)
{
	for (int tries = 0; tries < 3; tries++) {
		session->connect();
		if (session->is_connected()) return true;
	}
	return false;
}

int main(int argc, char *argv[])
{
	auto logger = std::make_unique<Logger>("  RCON CLI  ", LOG_LEVEL::DEBUG);
	rcon_addr_t server_address{"127.0.0.1", 27015};
	std::string server_password = "";

	po::options_description ops_desc("Options");
	ops_desc.add_options()
		("help,h", "Displays this help screen and exits.")
		("ip,i", po::value<std::string>(&server_address.ip)->default_value("127.0.0.1"), "The remote IP address of the RCON server.")
		("port,p", po::value<uint16_t>(&server_address.port)->default_value(27015), "The port that the server is listening on. This must be an IPv4 address.")
		("password,pass,P", po::value<std::string>(&server_password)->implicit_value(""), "The password used for authenticating with the server. Specifying this option and leaving it blank will bypass the \"no password prompt\".");
	
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, ops_desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << help_text << ops_desc << std::endl;
		return 0;
	}

	logger->debug("IP: " + server_address.to_string());
	// logger->debug("Password: " + server_password);
	
	if (!is_IPv4(server_address.ip)) {
		std::cout << help_text << ops_desc << std::endl;
		return 1;
	}

	if (!vm.count("password")) {
		std::cout << "You have not entered a password. Are you sure you want to continue? (y/N): ";
		char response = getchar();
		if (response != 'Y' && response != 'y') {
			exit(1);
		}
	}

	Rcon *rcon_session = new Rcon(server_address);
	rcon_session->connect();
	if (!rcon_session->is_connected()) return 0;

	rcon_session->authenticate(server_password);
	std::string line;

	while (true) {
		std::cout << "$ ";
		std::getline(std::cin, line);
		if (std::cin.eof()) break;

		std::cout << rcon_session->send_command(line) << '\n';

		if (!rcon_session->is_connected() && !attempt_reconnect(rcon_session)) break;
	}
	rcon_session->close();
}