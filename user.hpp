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
public:
	~User();
	User();
	User(std::string nickname);
	User(User const & src);
	User & operator=(User const & rhs);
	void	setTmpPwd(std::string tmpPwd);
	void	setNickname(std::string nickname);
	void	setUsername(std::string username);
	void	setRealname(std::string realname);
	std::string getUsername() const;
	std::string getRealName() const;
	std::string getNickname() const;
};

#endif
