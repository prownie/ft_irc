#ifndef USER_HPP
#define USER_HPP
#include "args.hpp"

class User
{
private:
	std::string _nickname;

public:
	~User();
	User(std::string nickname);
	User(User const & src);
	User & operator=(User const & rhs);

};

#endif
