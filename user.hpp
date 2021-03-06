#ifndef USER_HPP
#define USER_HPP
#include "args.hpp"
#include <vector>

class User
{
private:
	std::string _nickname;
	std::vector<std::string> _channels;
	int		_rights;
	std::string	_username;
	std::string _realname;
	std::string _tmpPassword;
	std::string _tmpRequest;
	std::string	_operName;
	bool		_isRegistered;
public:
	~User();
	User();
	User(User const & src);
	User & operator=(User const & rhs);
	void	setTmpPwd(std::string tmpPwd);
	void	setNickname(std::string nickname);
	void	setUsername(std::string username);
	void	setRealname(std::string realname);
	void	setOperName(std::string realname);
	void	appendTmpRequest(std::string request);
	std::string & getTmpPwd() ;
	std::string const & getUsername() const;
	std::string const & getRealName() const;
	std::string const & getNickname() const;
	std::string & getTmpRequest() ;
	std::string const & getOperName() const;
	bool	isRegistered();
	void	setRegistered(int val);
	void	cleanTmpRequest();
};

#endif

/*
PASS abcd
NICK prownie1
USER o o o o

PASS abcd
NICK prownie2
USER o o o o

PASS abcd
NICK prownie3
USER o o o o
*/
