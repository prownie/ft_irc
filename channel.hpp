#ifndef CHANNELS_HPP
#define CHANNELS_HPP
#include "user.hpp"
class Channel {
private:
	std::vector<int>	_users;
	std::string		_name;
	std::string		_key;
	Channel();
public:
	Channel(std::string name, std::string key);
	Channel(int user, std::string name, std::string key);
	Channel & operator=(Channel const & rhs);
	~Channel();
	Channel(Channel const & src);
	std::vector<int> getUsers() const;
	std::string	getName() const;
	std::string getKey() const;
	void	addUser(int fd);

};
#endif
