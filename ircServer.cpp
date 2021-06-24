#include "ircServer.hpp"

ircServer::ircServer(Args args) : _args(args), _nb_fds(0){

}

ircServer::~ircServer() {
	std::map<int, User>::iterator it;

	for (it = _userList.begin(); it != _userList.end(); it++)
	{
		close(it->first);
	}
	_userList.clear();
	close(_pollfds[0].fd);
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
	while(request.size() && isspace(request.front())) request.erase(request.begin()); // removes first spaces
	while(request.size() && isspace(request.back())) request.pop_back(); //remove last spaces
	if (whichCommand(request) > -1) {
		std::cout << "It is a command" << std::endl;
		void		(ircServer::*ptr[])(std::string &, int) = {
		&ircServer::passCommand,
		&ircServer::nickCommand,
		&ircServer::userCommand,
		&ircServer::joinCommand,
		&ircServer::operCommand,
		&ircServer::quitCommand,
		&ircServer::privmsgCommand,
		&ircServer::lusersCommand,
		&ircServer::motdCommand
		};
		(this->*ptr[whichCommand(request)]) (request, fd);
	}
	else {
		std::cout << "it is not a command wesh" << std::endl;
		std::istringstream iss(request);
		std::string command;
		iss >> command;
		std::cout << "command = |" << command << "|" << std::endl;
		send_to_fd("421", std::string(command) +" :Unknown command", _userList[fd], fd, false);
	}
}

int	ircServer::whichCommand(std::string & request) {
	const char* arr[] = {"PASS","NICK","USER","JOIN","OPER","QUIT","PRIVMSG","LUSERS", "MOTD"};
	std::istringstream iss(request);
	std::string firstWord;
	std::vector<std::string>::iterator it;

	iss >> firstWord;
	std::transform(firstWord.begin(), firstWord.end(),firstWord.begin(), ::toupper);
	std::vector<std::string> commandList(arr, arr + sizeof(arr)/sizeof(arr[0]));
	if (find(commandList.begin(), commandList.end(), firstWord) != commandList.end())
		for (int i = 0; i < commandList.size() - 1; i++)
			if (firstWord == commandList[i])
				return i;
	return -1;
}

void ircServer::passCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("PASS"));

	if (str.empty()){
		send_to_fd("461", "QUIT :Syntax error", _userList[fd], fd, false);
		return;
	}
	str = str.substr(str.find_first_not_of(" "));
	if (std::count(str.begin(), str.end(), ' ') > 0 && str[0] != ':') {//there is more than one word, not rfc compliant
		send_to_fd("461", "QUIT :Syntnax error", _userList[fd], fd, false);
		return;
	}
	std::map<int, User>::iterator it = _userList.find(fd);
	str.erase( std::remove(str.begin(), str.end(), '\n'), str.end() );
	if (_userList[fd].getNickname().compare("*") != 0 || !_userList[fd].getUsername().empty()) // already registered if nickname or username not empty
	{
		send_to_fd("462", ":Connection already registered", it->second, fd, false);
		return;
	}
	it->second.setTmpPwd(str);
}

void ircServer::nickCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("NICK"));
	std::stringstream 	stream(str);
	std::string		oneWord;
	unsigned int	countParams = 0;
	while(stream >> oneWord) { ++countParams;}

	if (countParams < 1 || countParams > 2){
		send_to_fd("461", "NICK :Syntax error", _userList[fd], fd, false);
		return;
	}
	for (std::map<int, User>::iterator it = _userList.begin(); it != _userList.end(); it++)
		if (it->second.getNickname().compare(oneWord) == 0) { // already same nickname
			send_to_fd("462", " :Nickname is already in use", it->second, fd, false);
			return;
		}
	_userList[fd].setNickname(oneWord);
	std::cout << "nickname = " << _userList[fd].getNickname() << ", username = " << _userList[fd].getUsername() << std::endl;
	if (_userList[fd].getNickname().compare("*") != 0 && !(_userList[fd].getUsername().empty()))
		if (!checkRegistration(fd))
		{
			std::string rep("ERROR :Access denied: Bad password?\n");
			send(fd, rep.c_str(), rep.length(), 0);
			close_fd(fd);
			return;
		}
}

void ircServer::userCommand(std::string & request, int fd) {
	std::string str = request.substr(strlen("USER"));
	std::stringstream 	stream(str);
	std::string		oneWord, firstword;;
	unsigned int	countParams = 0;
	if (stream >> firstword) ++countParams;
	while(stream >> oneWord) { ++countParams;}

	/*bad user syntax, doesnt respect  <username> * * <realname> */
	if (countParams != 4){
		send_to_fd("461", "USER :Syntax error", _userList[fd], fd, false);
		return;
	}
	std::map<int, User>::iterator it = _userList.find(fd);
	if (_userList[fd].getNickname().compare("*") != 0 && !it->second.getRealName().empty())
	{
		send_to_fd("462", ":Connection already registered", it->second, fd, false);
		return;
	}
	std::cout << "oneWord = " << oneWord << std::endl;
	it->second.setUsername(firstword);
	it->second.setRealname(oneWord.substr(1));
	/*everything fine, answer to the client*/
	std::cout << "usernqme in usercmd = " << it->second.getUsername() << std::endl;
	if (_userList[fd].getNickname().compare("*") != 0 && !(_userList[fd].getUsername().empty()))
		if (!checkRegistration(fd))
		{
			std::string rep("ERROR :Access denied: Bad password?\n");
			send(fd, rep.c_str(), rep.length(), 0);
			close_fd(fd);
			return;
		}
}

void ircServer::joinCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;
	std::string str = request.substr(strlen("JOIN"));
	std::stringstream 	stream(str);
	std::string		oneWord;
	unsigned int	countParams = 0;
	while(stream >> oneWord) { ++countParams;}

	std::string chans, keys, firstchan, firstkey;
	std::map<int, User>::iterator it = _userList.find(fd);
	size_t i;

	if (countParams < 1 || countParams > 2) { //BAD SYNTAX
		send_to_fd("461", "JOIN :Syntax error", it->second, fd, false);
	}
	if ((i = str.find(" ")) !=  std::string::npos) {// there is keys(2nd params)
		chans = str.substr(0,i-1);
		keys = str.substr(i+1);
	}
	else
		chans = str;
	while (!chans.empty()) { //can have more chan than keys, so we dont care
		if (chans.find(',')) //more than 1 chan
			firstchan = chans.substr(0, chans.find(',') - 1);
		else
			firstchan = chans;

		if (keys.find(',')) //more than 1 keys
			firstkey = keys.substr(0, chans.find(',') - 1);
		else
			firstkey = keys;

		std::map<std::string, std::pair<std::vector<int>, std::string> >::iterator itchan = _channels.find(firstchan);
		if (itchan != _channels.end()) {
			itchan->second.first.push_back(fd);
			joinMsgChat(it->second, firstchan, fd, "JOIN", std::string(""));
			std::cout << "add to existing chan" << std::endl;
			//send_to_fd("");
		}
		else { //chan must begin with & or #, cant contain spaces/ctrl G/comma
			if (firstchan.find_first_of("&#") == 0  || firstchan.find(" ,\x07") != std::string::npos) {
				_channels[firstchan] = std::pair<std::vector<int>, std::string>(std::vector<int>(1, fd), firstkey);
				joinMsgChat(it->second, firstchan, fd, "JOIN", std::string(""));
				std::cout << "new chan" << std::endl;
			}
			else //bad chan name
			{
				send_to_fd("403", std::string(firstchan)+" :No such channel", it->second, fd, false);
				std::cout << std::string(firstchan)+" :No such channel" << std::endl;
			}
		}
		if (chans.find(',') != std::string::npos) //more than 1 chan
			chans = chans.substr(chans.find(',')+1);
		else
		{ chans.erase(); std::cout << "chans erased" << std::endl; }
	}
	std::cout << "chan empty, leave functionlist of vector in chan :" << std::endl;

	std::map<std::string, std::pair<std::vector<int>, std::string> >::iterator itchan = _channels.find(firstchan);
	std::vector<int> users = itchan->second.first;
	for (std::vector<int>::iterator it = users.begin(); it != users.end(); it++)
	{
		std::cout << "value fd = " << (*it) << std::endl;
	}
}

void ircServer::operCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;
	std::string str = request.substr(strlen("OPER"));
	std::stringstream 	stream(str);
	std::string		oneWord;
	unsigned int	countParams = 0;
	while(stream >> oneWord) { ++countParams;}
}

void ircServer::quitCommand(std::string & request, int fd) {
	std::vector<int> users_to_contact;
	std::string str = request.substr(strlen("QUIT"));
	std::string message;

	if (str.empty()) //no message, init with nickname
		message = _userList[fd].getNickname();
	else
		message = str.substr(str.find_first_not_of(" "));
	if (std::count(message.begin(), message.end(), ' ') > 0 && message[0] != ':') {//there is more than one word, : needed
		send_to_fd("461", "QUIT :Syntax error", _userList[fd], fd, false);
		return;
	}
	std::map<std::string, std::pair<std::vector<int>, std::string> >::iterator it = _channels.begin();
	std::map<std::string, std::pair<std::vector<int>, std::string> >::iterator ite = _channels.end();

	for (; it != ite; it++) //fill users to contact with users who shares channels with the leaver
	{
		std::vector<int> tmp_users = it->second.first;
		for (std::vector<int>::iterator itusers = tmp_users.begin(); itusers != tmp_users.end(); itusers++)
		{
			if (find(users_to_contact.begin(), users_to_contact.end(),(*itusers)) != users_to_contact.end())
				users_to_contact.push_back((*itusers));
		}
	}
	for (std::vector<int>::iterator contact = users_to_contact.begin(); contact != users_to_contact.end(); contact++)
	{
		if (*contact != fd) {
			std::string rep(":");
			rep += _userList[fd].getNickname();
			rep += "!~";
			rep += _userList[fd].getUsername();
			rep += "@localhost QUIT ";
			rep += message;
			rep += "\n";
			send(fd, rep.c_str(), rep.length(), 0);
		}
	}
}

void ircServer::privmsgCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;
	std:: string str = request.substr(strlen("PRIVMSG"));
	if (str.empty()) {
		send_to_fd("411", ":No recipient given (PRIVMSG)", _userList[fd], fd, false);
		return;
	}
	std::string target = str.substr(str.find_first_not_of(" "));
	if (target.find(" ") == std::string::npos) {
		send_to_fd("412", "No text to send", _userList[fd], fd, false); //only dest, no params
		return;
	}
	std::string message = target.substr(target.find_first_of(" ")+1);
	message = message.substr(message.find_first_not_of(" "));
	target = target.substr(0, target.find(" "));
	if (std::count(message.begin(), message.end(), ' ') > 0 && message[0] != ':') {//there is more than one word, : needed
		send_to_fd("461", "PRIVMSG :Syntnax error", _userList[fd], fd, false);
		return;
	}
	std::map<std::string, std::pair<std::vector<int>, std::string> >::iterator itchan = _channels.find(target);
	if (itchan != _channels.end())
	{
		std::vector<int> users = itchan->second.first;
		for (std::vector<int>::iterator it = users.begin(); it != users.end(); it++)
		{
			std::cout << "value fd = " << (*it) << std::endl;
			joinMsgChat(_userList[fd], target, (*it), "PRIVMSG", message);
		}
		return;
	}
	for (std::map<int, User>::iterator it = _userList.begin(); it != _userList.end(); it++)
	{
		if (it->second.getNickname() == target)
		{
			joinMsgChat(_userList[fd], target, it->first, "PRIVMSG", message);
			return;
		}
	}

	send_to_fd("401", ":No such nick or channel name",_userList[fd],fd,false);
}

void ircServer::lusersCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;
	std::map<int, User>::iterator it = _userList.find(fd);
	send_to_fd("251",std::string(":There are ")+ getNbUsers() + " users, 0 servuces and 1 servers", it->second, fd, false);
	send_to_fd("254",std::string(getNbChannels()) + " :channels formed", it->second, fd, false);
	send_to_fd("255",std::string(":I have ")+ getNbUsers() + " users, 0 service and 0 servers", it->second, fd, false);
}

void ircServer::motdCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;

}

int	ircServer::checkPassword(User user){
	if (user.getTmpPwd() != _args.getPassword())
	{
		std::cout << "user pwd = |" << user.getTmpPwd() << "| server pwd = |"
			<< _args.getPassword() << "|" << std::endl;
		return false;
	}
	std::cout << "SAME PASSWORD, INSANE" << std::endl;
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

std::string	ircServer::getNbUsers() const{
	std::stringstream ss;
	ss << _userList.size();
	return ss.str();
}

std::string	ircServer::getNbChannels() const{
	std::stringstream ss;
	ss << _channels.size();
	return ss.str();
}

void	ircServer::send_to_fd(std::string code, std::string message,
User const & user, int fd, bool dispRealName) {
	std::string rep(":");
	rep += SERVER_NAME;
	rep += " ";
	rep += code;
	rep += " ";
	rep += user.getNickname();
	rep += " ";
	rep += message;
	if (dispRealName){
		rep += " ";
		rep += user.getNickname();
		rep += "!~";
		rep += user.getUsername();
		rep += "@localhost";
	}
	rep += "\n";
	send(fd, rep.c_str(), rep.length(), 0);
	std::cout << message << std::endl;
	return;
}

void	ircServer::joinMsgChat(User const & user, std::string channel, int fd, std::string command, std::string message) {
	std::string rep(":");
	rep += user.getNickname();
	rep += "!~";
	rep += user.getUsername();
	rep += "@localhost ";
	rep += command;
	if (command.compare("PRIVMSG") == 0)
		rep += (std::string(" ") + channel + " " + message);
	else
		rep += (" :" + channel);
	rep += "\n";
	std::cout << "rep=" << rep << "with command=" << command << std::endl;
	send(fd, rep.c_str(), rep.length(), 0);
}

int		ircServer::checkRegistration(int fd) {
	if (!checkPassword(_userList[fd])) {
		std::cout << "i return ZERO, COMME MON QI" << std::endl;
		return 0;
	}
	send_to_fd("0001", ":Welcome to our FT_IRC project !", _userList[fd], fd, true);
	send_to_fd("251",std::string(":There are ")+ getNbUsers() + " users, 0 servuces and 1 servers", _userList[fd], fd, false);
	send_to_fd("254",std::string(getNbChannels()) + " :channels formed", _userList[fd], fd, false);
	send_to_fd("255",std::string(":I have ")+ getNbUsers() + " users, 0 service and 0 servers", _userList[fd], fd, false);
	return 1;
}

int	ircServer::check_unregistered(int fd){
	if (_userList[fd].getUsername().empty() || (_userList[fd].getNickname().compare("*") == 0)) {
		std::string rep(":");
		rep += SERVER_NAME;
		rep += " 451 ";
		rep += _userList[fd].getNickname();
		rep += "  :Connection not registered\n";
		send(fd, rep.c_str(), rep.length(), 0);
		return 1;
	}
	std::cout << "first condition" << _userList[fd].getUsername().empty() << "second one " << (_userList[fd].getNickname().compare("*") == 0) << std::endl;
	return 0;
}

void	ircServer::close_fd(int fd){
	shutdown(fd, 2);
	_userList.erase(fd);
}
