#ifndef ARGS_HPP
#define ARGS_HPP
#include <exception>
#include <iostream>
#include <string>
#include <algorithm>

class Args
{
private:
	Args();
	std::string _password;
	int			_port;
	std::string _networkHost;
	int 		_networkPort;
	std::string _networkPassword;
public:
	Args(int ac, char **av);
	~Args();
	Args & operator=(Args const & rhs);
	Args(Args const & src);
	std::string const & getPassword() const;
	int	getPort() const;
	std::string const & getNetworkHost() const;
	int	getNetworkPort() const;
	std::string const & getNetworkPassword() const;
	class Bad_arg_number_exception : public std::exception
	{
		virtual const char* what() const throw();
	};
	class Empty_password_exception : public std::exception
	{
		virtual const char* what() const throw();
	};
	class Bad_port_exception : public std::exception
	{
		virtual const char* what() const throw();
	};
	class Bad_network_data_exception : public std::exception
	{
		virtual const char* what() const throw();
	};
};

std::ostream & operator<<(std::ostream& o, Args const & b);

#endif
