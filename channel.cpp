#include "channel.hpp"

Channel::Channel(std::string name, std::string key) : _name(name), _key(key){}

Channel::Channel(int user, std::string name, std::string key) :  _name(name), _key(key){
	_users.push_back(user);
}

Channel & Channel::operator=(Channel const & rhs){
	if (this != &rhs) {
		_users = rhs.getUsers();
		_name = rhs.getName();
		_key = rhs.getKey();
	}
	return *this;
}

Channel::Channel(Channel const & src){
	*this = src;
}

Channel::~Channel(){

}

std::vector<int> Channel::getUsers() const {return _users;}
std::string	Channel::getName() const{return _name;}
std::string Channel::getKey() const{return _key;}
void	Channel::addUser(int fd) {_users.push_back(fd);}
