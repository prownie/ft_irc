#ifndef IRCSERVER_HPP
#define IRCSERVER_HPP
#include "args.hpp"
#include "user.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <sstream>
#include <map>
#include "user.hpp"

#define MAX_FDS 1000
#define DATA_BUFFER 10000

class ircServer
{
private:
	ircServer();
	Args _args;
	int		create_tcp_server_socket();
	struct pollfd _pollfds[MAX_FDS];
	int	_nb_fds;
	std::map<int, User> _userList;
public:
	ircServer(Args args);
	ircServer(ircServer const & src);
	ircServer & operator=(ircServer const & rhs);
	~ircServer();
	void	config();
	void	run();
	void	processRequest(std::string & request, int fd);
	int		whichCommand(std::string & request);
	void 	passCommand(std::string & request, int fd);
	void 	nickCommand(std::string & request, int fd);
	void 	userCommand(std::string & request, int fd);
	void 	joinCommand(std::string & request, int fd);
	void 	operCommand(std::string & request, int fd);
	void 	quitCommand(std::string & request, int fd);
	void 	privmsgCommand(std::string & request, int fd);
};
#endif
