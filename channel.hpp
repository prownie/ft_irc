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
	void	 setKey(std::string key);
	void	addUser(int fd);
	void	eraseUser(int fd);

};
#endif

/*
Currently listening to 2 clients
1str is empty ?
message = :Client closed connection
I PUSHED BACK number :4
I PUSHED BACK number :5
:prownie!~o@localhost QUIT :Client closed connection
:prownie!~o@localhost QUIT :Client closed connection
User prownie is closed
fd closed = 5
*/

/*
Currently listening to 2 clients
Connection on fd[5] closed by client
User prownie is closed
IM LEAVING HERE
1str is empty ?
message = :Client closed connection
I PUSHED BACK number :4
I PUSHED BACK number :5
User created
:*!~@localhost QUIT :Client closed connection
:*!~@localhost QUIT :Client closed connection
User * is closed
fd closed = 5*/
