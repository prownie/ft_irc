#include "ircServer.hpp"

ircServer::ircServer(Args args) : _args(args), _nb_fds(0){
}

ircServer::~ircServer() {

}

void	ircServer::config() {
	_pollfds[0].fd = ircServer::create_tcp_server_socket();
	_pollfds[0].events = POLLIN;
	_pollfds[0].revents = 0;
	_nb_fds = 1;
}

int		ircServer::create_tcp_server_socket() {
	struct sockaddr_in saddr;
	int fd, ret_val;

	/*Create tcp socket*/
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1)
		throw std::runtime_error("Creating server socket failed\n");
	std::cout << "Server socked created with fd [" << fd << "]" << std::endl;
	fcntl(fd, F_SETFL, O_NONBLOCK);

	/*init socket address structure + bind*/
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(_args.getPort());
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	std::cout << "port value = " << _args.getPort() << std::endl;
	ret_val = bind(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret_val != 0) {
		close(fd);
		throw std::runtime_error("Binding failed, socket has been closed\n");
	}

	/*listen for incoming connections*/
	ret_val = listen(fd, 10);
	if (ret_val != 0) {
		close(fd);
		throw std::runtime_error("Listen failed, socket has been closed\n");
	}
	return fd;
}

void	ircServer::run() {
	socklen_t addrlen = sizeof(struct sockaddr_storage);
	struct sockaddr_storage client_saddr;
	int ret_val;
	char buf[DATA_BUFFER];

	while (1) {
		std::cout << ("-----------------------------------------------------------") << std::endl;
		std::cout << ("\nUsing poll() to listen for incoming events\n") << std::endl;
		if (poll(_pollfds, _nb_fds, -1) == -1)
			std::cout << "Error during poll" << std::endl;
		for (int fd = 0; fd < (_nb_fds); fd++) {
			if ((_pollfds + fd)->fd  <= 0)
				continue;
			if (((_pollfds + fd)->revents & POLLIN) == POLLIN){
				if (fd == 0) { //if event occured on the socket server
					int new_fd;
					if ((new_fd = accept(_pollfds[0].fd, (struct sockaddr *) &client_saddr, &addrlen)) == -1)
						std::cerr << "Accept failed" << std::endl;
					std::cout << "New connection accepted on fd[" << new_fd << "]" << std::endl;
					_userList.insert(std::pair<int, User>(new_fd, User()));
					(_pollfds + _nb_fds)->fd = new_fd;
					(_pollfds + _nb_fds)->events = POLLIN;
					(_pollfds + _nb_fds)->revents = 0;
					if (_nb_fds < MAX_FDS)
						_nb_fds++;
				}
				else {
					ret_val = recv((_pollfds + fd)->fd, buf, DATA_BUFFER, 0);
					if (ret_val == -1)
						std::cerr << "Recv failed" << std::endl;
					else if (ret_val == 0) {
						std::cout << "Connection on fd[" << (_pollfds + fd)->fd << "] closed by client" << std::endl;
						_userList.erase((_pollfds + fd)->fd);
						if (close((_pollfds + fd)->fd) == -1)
							std::cerr << "Client close failed" << std::endl;
						(_pollfds + fd)->fd *= -1;
					}
					else {
						std::cout << "SOME DATA HAS BEEN RECEVEID, YOUH:\n" << buf << std::endl;
						std::string data(buf);
						parseRequest(data, (_pollfds + fd)->fd);
						memset(buf, 0, DATA_BUFFER);
						std::cout << "AFtER  REQ PROCESSING" << std::endl;
					}
				}
			}
		}
	}
}

void	ircServer::processRequest(std::string & request, int fd) {

	if (whichCommand(request) > -1) {
		std::cout << "It is a command" << std::endl;
		void		(ircServer::*ptr[])(std::string &, int) = {
		&ircServer::passCommand,
		&ircServer::nickCommand,
		&ircServer::userCommand,
		&ircServer::joinCommand,
		&ircServer::operCommand,
		&ircServer::quitCommand,
		&ircServer::privmsgCommand
		};
		(this->*ptr[whichCommand(request)]) (request, fd);
	}
	else {
		std::cout << "it is not a command wesh" << std::endl;
	}
	/*for (int i = 0; i < request.length(); i++)
	{
		std::cout << "value char [" << i << "] = " << (int)request[i] << std::endl;
	}*/
}

int	ircServer::whichCommand(std::string & request) {
	const char* arr[] = {"PASS","NICK","USER","JOIN","OPER","QUIT","PRIVMSG"};
	std::istringstream iss(request);
	std::string firstWord;
	std::vector<std::string>::iterator it;

	iss >> firstWord;
	std::vector<std::string> commandList(arr, arr + sizeof(arr)/sizeof(arr[0]));
	if (find(commandList.begin(), commandList.end(), firstWord) != commandList.end())
		for (int i = 0; i < commandList.size() - 1; i++)
			if (firstWord == commandList[i])
				return i;
	return -1;
}

void ircServer::passCommand(std::string & request, int fd) {
	std::string str = request.substr(request.find_first_of(" \t") + 1);

	std::map<int, User>::iterator it = _userList.find(fd);
	//it->second.setTmpPwd(str);
	str.erase( std::remove(str.begin(), str.end(), '\n'), str.end() );
	it->second.setTmpPwd(str);
	// if (str != _args.getPassword())
	// {
	// 	char *buf = ":irc.example.com 464 chris :Password Incorrect";
	// 	send(fd,buf,strlen(buf),0);
	// 	std::cout << "sent" << std::endl;
	// }
}

void ircServer::nickCommand(std::string & request, int fd) {
	std::string str = request.substr(request.find_first_of(" \t") + 1);

	std::map<int, User>::iterator it = _userList.find(fd);
	it->second.setNickname(str);
}

void ircServer::userCommand(std::string & request, int fd) {
	std::string str = request.substr(request.find_first_of(" \t") + 1);

	std::map<int, User>::iterator it = _userList.find(fd);
	std::cout << "username empty = |" << !it->second.getUsername().empty() << "|, realname = |" << !it->second.getRealName().empty() << "|" << std::endl;
	if (checkPassword(it->second) != 1) // BAD PASSWORD
	{
		std::string rep("ERROR :Access denied: Bad password?\n");
		send(fd, rep.c_str(), rep.length(), 0);
		return;
	}
	if (!it->second.getUsername().empty() && !it->second.getRealName().empty())
	{
		std::string rep(":ft_irc.com 462");
		rep += " ";
		rep += it->second.getNickname();
		rep += " :Connection already registered";
		send(fd, rep.c_str(), rep.length(), 0);
		std::cout << "Client already registered" << std::endl;
		return;
	}
	if ( (std::count(str.begin(), str.end(), ':') != 1) || (std::count(str.begin(), str.begin() + str.find_first_of(':'), ' ') != 3 ) )
	{
		std::string rep(":ft_irc.com 461");
		rep += " ";
		rep += it->second.getNickname();
		rep += "USER :Syntax error";
		send(fd, rep.c_str(), rep.length(), 0);
		std::cout << "USER :Syntax error" << std::endl;
		return;
	}
	it->second.setUsername(str.substr(0, str.find_first_of(" \t")));
	it->second.setRealname(str.substr(str.find_last_of(":")));


}

void ircServer::joinCommand(std::string & request, int fd) {
	std::string str = request.substr(request.find_first_of(" \t") + 1);

	std::map<std::string, std::vector<int>>::iterator it = _channels.find()
}

void ircServer::operCommand(std::string & request, int fd) {

}

void ircServer::quitCommand(std::string & request, int fd) {

}

void ircServer::privmsgCommand(std::string & request, int fd) {

}

int	ircServer::checkPassword(User user){
	if (user.getTmpPwd() != _args.getPassword())
	{
		std::cout << "user pwd = |" << user.getTmpPwd() << "| server pwd = |"
			<< _args.getPassword() << "|" << std::endl;
		return false;
	}
	return true;
}

void	ircServer::parseRequest(std::string request, int fd){
	std::string parse;
	std::map<int, User>::iterator it = _userList.find(fd);

	request.erase(std::remove(request.begin(), request.end(), '\r'), request.end()); //erase \r, to work with netcat or irc client
	while (!request.empty())
	{
		if (request.find('\n') == std::string::npos) {// no \n found, incomplete request, add to user
			it->second.appendTmpRequest(request);
			break;
		}
		else { //\n found, but maybe more than 1, check User._tmpRequest to append with it
			parse = it->second.getTmpRequest().append(request.substr(0, request.find_first_of("\n")));
			it->second.cleanTmpRequest(); //request is complete, we can clean tmpReq;
			std::cout << "PARSED REQUEST IN PARSEREQUEST : |" << parse << "|" << std::endl;
			processRequest(parse, fd);
		}
		request = request.substr(request.find_first_of("\n") + 1);
	}
}
