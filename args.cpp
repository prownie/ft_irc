#include "args.hpp"

Args::Args(int ac, char **av) {
	if (ac < 2 || ac > 4)
		throw Args::Bad_arg_number_exception();
	int i = ac - 1;
	_password = std::string(av[i--]);
	if	(_password.empty())
		throw Args::Empty_password_exception();
	_port = atoi(av[i]);
	std::string tmp (av[i--]); // check if there is non-digits char in the port slot
	if ((_port < 0) || (_port > 65535) || (tmp.find_first_not_of("0123456789") != std::string::npos) || (tmp.empty()))
		throw Args::Bad_port_exception();
	if (i) {
		std::string data(av[i]);
		if (std::count(data.begin(), data.end(), ':') != 2)
			throw Bad_network_data_exception();
		unsigned first = data.find_first_of(':');
		unsigned last = data.find_last_of(':');
		_networkHost = data.substr(0, first);
		if (_networkHost.empty())
			throw Args::Bad_network_data_exception();
		_networkPort = atoi(data.substr(first, last - first - 1).c_str());
		if (data.substr(first, last - first - 1).empty())
			throw Args::Bad_network_data_exception();
		_networkPassword = data.substr(last);
	}
}

Args::~Args() {

}

Args & Args::operator=(Args const & rhs) {
	if (this == &rhs)
		return (*this);
	_password = rhs._password;
	_port = rhs._port;
	_networkHost = rhs._networkHost;
	_networkPort = rhs._networkPort;
	_networkPassword = rhs._networkPassword;
	return (*this);
}

Args::Args(Args const & src) {
	_password = src.getPassword();
	_port = src.getPort();
	_networkHost = src.getNetworkHost();;
	_networkPort = src.getNetworkPort();
	_networkPassword = src.getNetworkPassword();
}

std::string const & Args::getPassword() const {return _password;}
int	Args::getPort() const {return _port;}
std::string const & Args::getNetworkHost() const {return _networkHost;}
int	Args::getNetworkPort() const {return _networkPort;}
std::string const & Args::getNetworkPassword() const {return _networkPassword;}

char const * Args::Bad_arg_number_exception::what( void ) const throw()
{
	return "Wrong arg number, use ./ircserv [host:port_network:password_network] <port> <password>";
}

char const * Args::Empty_password_exception::what( void ) const throw()
{
	return "Password can't be empty, come on ...";
}

char const * Args::Bad_port_exception::what( void ) const throw()
{
	return "Port needs to contain digits only, and must be between 0 and 65535";
}

char const * Args::Bad_network_data_exception::what( void ) const throw()
{
	return "Network data must be written with the format host:port:password";
}

std::ostream & operator<<(std::ostream & o, Args const & a)
{
	o << "----Args data----" << std::endl;
	o << "Password: " << a.getPassword() << std::endl;
	o << "Port: " << a.getPort() << std::endl;
	o << "NetworkHost: " << a.getNetworkHost() << std::endl;
	o << "NetworkPort: " << a.getNetworkPort() << std::endl;
	o << "NetworkPassword: " << a.getNetworkPassword() << std::endl;
	o << "-----------------" << std::endl;
	return o;
}
