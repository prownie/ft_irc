#include "args.hpp"
#include "ircServer.hpp"

int main(int ac, char **av) {
	try
	{
		Args ircArgs(ac, av);
		std::cout << ircArgs << std::endl;
		ircServer serv(ircArgs);
		serv.config();
		serv.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

}
