#include "user.hpp"

User::~User() {
}

User::User() : _nickname(std::string("*")){
	std::cout << "User created" << std::endl;
}

User::User(User const & src) {
	_nickname = src._nickname;
	_channels = src._channels;
	_rights = src._rights;
	_realname = src._realname;
	_tmpPassword = src._tmpPassword;
}

void	User::setTmpPwd(std::string tmpPwd) {_tmpPassword = tmpPwd;}
void	User::setNickname(std::string nickname) {_nickname = nickname;}
void	User::setUsername(std::string username) {_username = username;}
void	User::setRealname(std::string realname) {_realname = realname;}
void	User::setOperName(std::string opername) {_operName = opername;}
void	User::appendTmpRequest(std::string request) {_tmpRequest.append(request);}
std::string const & User::getUsername() const {return _username;}
std::string const & User::getRealName() const{return _realname;}
std::string const & User::getNickname() const{return _nickname;}
std::string & User::getTmpPwd()  {return _tmpPassword;}
std::string & User::getTmpRequest() {return _tmpRequest;}
std::string const & User::getOperName()  const{return _operName;}
void		User::cleanTmpRequest(){_tmpRequest.clear();}
