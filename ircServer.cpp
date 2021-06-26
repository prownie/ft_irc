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
		std::cout << "\nCurrently listening to " << _nb_fds - 1 <<" clients \n" << std::endl;
		if (poll(_pollfds, _nb_fds, -1) == -1)
			throw std::runtime_error("Error during poll\n");
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
						_nb_fds--;
					}
					else {
						std::string data(buf);
						parseRequest(data, (_pollfds + fd)->fd);
						memset(buf, 0, DATA_BUFFER);
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
		void		(ircServer::*ptr[])(std::string &, int) = {
		&ircServer::passCommand,
		&ircServer::nickCommand,
		&ircServer::userCommand,
		&ircServer::joinCommand,
		&ircServer::operCommand,
		&ircServer::quitCommand,
		&ircServer::privmsgCommand,
		&ircServer::lusersCommand,
		&ircServer::motdCommand,
		&ircServer::helpCommand,
		&ircServer::killCommand
		};
		(this->*ptr[whichCommand(request)]) (request, fd);
	}
	else {
		std::istringstream iss(request);
		std::string command;
		iss >> command;
		send_to_fd("421", std::string(command) +" :Unknown command", _userList[fd], fd, false);
	}
}

int	ircServer::whichCommand(std::string & request) {
	const char* arr[] = {"PASS","NICK","USER","JOIN","OPER","QUIT","PRIVMSG","LUSERS", "MOTD", "HELP", "KILL"};
	std::istringstream iss(request);
	std::string firstWord;
	std::vector<std::string>::iterator it;

	iss >> firstWord;
	std::transform(firstWord.begin(), firstWord.end(),firstWord.begin(), ::toupper);
	std::vector<std::string> commandList(arr, arr + sizeof(arr)/sizeof(arr[0]));
	if (find(commandList.begin(), commandList.end(), firstWord) != commandList.end())
		for (int i = 0; i < commandList.size(); i++)
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
	str.erase( std::remove(str.begin(), str.end(), '\n'), str.end() );
	if (_userList[fd].getNickname().compare("*") != 0 || !_userList[fd].getUsername().empty()) // already registered if nickname or username not empty
	{
		send_to_fd("462", ":Connection already registered", _userList[fd], fd, false);
		return;
	}
	_userList[fd].setTmpPwd(str);
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
	if (_userList[fd].getNickname().compare("*") != 0 && !_userList[fd].getRealName().empty())
	{
		send_to_fd("462", ":Connection already registered", _userList[fd], fd, false);
		return;
	}
	_userList[fd].setUsername(firstword);
	_userList[fd].setRealname(oneWord.substr(1));
	/*everything fine, answer to the client*/
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
	std::string		chans;
	std::string		keys;
	unsigned int	countParams = 0;
	if(stream >> chans) { ++countParams;}
	if(stream >> keys) { ++countParams;}
	if(stream >> chans) { ++countParams;}

	std::string firstchan, firstkey;
	size_t i;

	if (countParams < 1 || countParams > 2) { //BAD SYNTAX
		send_to_fd("461", "JOIN :Syntax error",_userList[fd], fd, false);
	}
	while (!chans.empty()) { //can have more chan than keys, so we dont care
		if (chans.find(',')) //more than 1 chan
			firstchan = chans.substr(0, chans.find(','));
		else
			firstchan = chans;

		if (keys.find(',')) //more than 1 keys
			firstkey = keys.substr(0, chans.find(',') - 1);
		else
			firstkey = keys;
		std::cout << "first chan = " << firstchan << std::endl;
		std::map<std::string, Channel >::iterator itchan = _channels.find(firstchan);
		if (itchan != _channels.end()) { //add to existing chan
			itchan->second.addUser(fd);
			joinMsgChat(_userList[fd], firstchan, fd, "JOIN", std::string(""));
			std::cout << "add to existing chan" << std::endl;

			std::vector<int> users = itchan->second.getUsers();
			for (std::vector<int>::iterator it = users.begin(); it != users.end(); it++)
				if ((*it) != fd)
					joinMsgChat(_userList[fd], firstchan, (*it), "JOIN", std::string(""));
			return;
		}
		else { //chan must begin with & or #, cant contain spaces/ctrl G/comma
			if ((firstchan.find_first_of("&#") == 0) && (firstchan.find_first_of(" ,\x07") == std::string::npos)) {
				_channels.insert(std::pair<std::string, Channel>(firstchan, Channel(fd, firstchan, firstkey)));
				joinMsgChat(_userList[fd], firstchan, fd, "JOIN", std::string(""));
				std::cout << "Creating new chan : " << firstchan << std::endl;
			}
			else //bad chan name
				send_to_fd("403", std::string(firstchan)+" :No such channel", _userList[fd], fd, false);
		}
		if (chans.find(',') != std::string::npos) //more than 1 chan
			chans = chans.substr(chans.find(',')+1);
		else
			chans.erase();
	}
}

void ircServer::operCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;
	std::string str = request.substr(strlen("OPER"));
	std::stringstream 	stream(str);
	std::string		user;
	std::string		password;
	unsigned int	countParams = 0;
	if (stream >> user) { ++countParams;}
	if (stream >> password) { ++countParams;}
	if (stream >> password) { ++countParams;}

	if (countParams != 2) {
		send_to_fd("461", "OPER :Syntax error", _userList[fd], fd, false);
		return;
	}
	if (password.compare(PWD_OPER) == 0) {
		_userList[fd].setOperName(user);
		send_to_fd("381", ":You are now an IRC operator", _userList[fd], fd, false);
	}
	else {
		send_to_fd("464", ":Password incorrect", _userList[fd], fd, false);
	}
}

void ircServer::quitCommand(std::string & request, int fd) {
	std::vector<int> users_to_contact;
	std::string str = request.substr(strlen("QUIT"));
	std::string message;

	if (str.empty()) //no message, init with nickname
		message = "Client closed connection";
	else
		message = str.substr(str.find_first_not_of(" "));
	if (std::count(message.begin(), message.end(), ' ') > 0 && message[0] != ':') {//there is more than one word, : needed
		send_to_fd("461", "QUIT :Syntax error", _userList[fd], fd, false);
		return;
	}
	std::map<std::string, Channel >::iterator it = _channels.begin();
	std::map<std::string, Channel >::iterator ite = _channels.end();

	for (; it != ite; it++) //fill users to contact with users who shares channels with the leaver
	{
		std::vector<int> tmp_users = it->second.getUsers();
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
	std::map<std::string, Channel >::iterator itchan = _channels.find(target);
	if (itchan != _channels.end())
	{
		std::vector<int> users = itchan->second.getUsers();
		for (std::vector<int>::iterator it = users.begin(); it != users.end(); it++)
			if ((*it) != fd)
				joinMsgChat(_userList[fd], target, (*it), "PRIVMSG", message);
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
	send_to_fd("251",std::string(":There are ")+ getNbUsers() + " users, 0 servuces and 1 servers", _userList[fd], fd, false);
	send_to_fd("254",std::string(getNbChannels()) + " :channels formed", _userList[fd], fd, false);
	send_to_fd("255",std::string(":I have ")+ getNbUsers() + " users, 0 service and 0 servers", _userList[fd], fd, false);
}

void ircServer::motdCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;

}

void ircServer::helpCommand(std::string & request, int fd) {
	std::string rep("Hello, need help  ? I will guide you on what to do and what you can do\n\n");
	rep += "The recommended order of orders for registering a customer is as follows:\n";
	rep += "PASS <password>\n";
	rep += "(The PASS command is used to set the 'login password')\n\n";
	rep += "NICK <pseudonyme>\n";
	rep += "(The NICK message is used to give a user a nickname)\n\n";
	rep += "USER <username> . . <real name>\n";
	rep += "(The USER message is used at the start of a connection to specify the user name, host name, server name, and real name of a new user)\n\n";
	rep += "JOIN <channel1,channel2> <key1,key2>\n";
	rep += "(The JOIN command is used by a client to start listening to a specific channel)\n\n";
	rep += "OPER <user> <password>\n";
	rep += "(The OPER message is used by a normal user to obtain the operator privilege)\n\n";
	rep += "QUIT [<Quit message>]\n";
	rep += "(A client session ends with a QUIT message can add a leave message)\n\n";
	rep += "PRIVMSG <recipient>(1 or more) <:text to send>\n";
	rep += "(PRIVMSG is used to send a private message between users)\n\n";
	rep += "OPER (OPER command)\n";
	rep += "OPER is used to have operator privileges\n\n";
	rep += "KICK <channel> <user>\n";
	rep += "(The KICK command is used to remove a user from a channel)\n\n";
	send(fd, rep.c_str(), rep.length(), 0);
}

void	ircServer::killCommand(std::string & request, int fd) {
	if (check_unregistered(fd)) return;
	std::string str = request.substr(strlen("KILL"));
	std::stringstream 	stream(str);
	std::string		target;
	std::string		message;
	unsigned int	countParams = 0;
	if (stream >> target) { ++countParams;}
	if (stream >> message) { ++countParams;}

	if (countParams < 2) {
		send_to_fd("461", "KILL :Syntax error", _userList[fd], fd, false);
		return;
	}
	if (_userList[fd].getOperName().empty()) {
		send_to_fd("481", ":Permission Denied- You're not an IRC operator",_userList[fd],fd,false);
		return;
	}
	for (std::map<int, User>::iterator it = _userList.begin(); it != _userList.end(); it++){
		if (target == it->second.getNickname()) {
			std::string rep("ERROR : KILLed by ");
			rep += _userList[fd].getNickname();
			rep += ": ";
			rep += str.substr(str.find_first_not_of(" ")).substr(str.find_first_of(" ")+1);
			rep += "\n";
			send(it->first, rep.c_str(), rep.length(), 0);
			close_fd(it->first);
			return;
		}
	}
	return;
}
int	ircServer::checkPassword(User user){
	if (user.getTmpPwd() != _args.getPassword())
		return false;
	return true;
}

void	ircServer::parseRequest(std::string request, int fd){
	std::string parse;
	std::map<int, User>::iterator it = _userList.find(fd);

	request.erase(std::remove(request.begin(), request.end(), '\r'), request.end()); //erase \r, to work with netcat or irc client
	while (!request.empty())
	{
		if (request.find('\n') == std::string::npos) {// no \n found, incomplete request, add to user
			_userList[fd].appendTmpRequest(request);
			break;
		}
		else { //\n found, but maybe more than 1, check User._tmpRequest to append with it
			parse = it->second.getTmpRequest().append(request.substr(0, request.find_first_of("\n")));
			_userList[fd].cleanTmpRequest(); //request is complete, we can clean tmpReq;
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
User const & user, int fd, bool dispRealName) const {
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
	send(fd, rep.c_str(), rep.length(), 0);
}

int		ircServer::checkRegistration(int fd) {
	if (!checkPassword(_userList[fd]))
		return 0;
	std::cout << "User " << _userList[fd].getNickname() << " registered !" << std::endl;
	send_to_fd("001", ":Welcome to our FT_IRC project !", _userList[fd], fd, false);
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
		rep += " :Connection not registered\n";
		send(fd, rep.c_str(), rep.length(), 0);
		return 1;
	}
	return 0;
}

void	ircServer::close_fd(int fd){
	int i = 0;
	for (; i < _nb_fds; i++)
		if (_pollfds[i].fd == fd)
			break;
	if (i == _nb_fds - 1) { // last poll, just close and decr fd number
		close(fd);
	}
	else { //switch the one to delete with the last one
		_pollfds[i] = _pollfds[_nb_fds - 1];
		close(fd);
	}
	std::cout << "fd closed = " << fd << std::endl;
	_nb_fds--;
}

