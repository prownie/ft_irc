#include "args.hpp"

int main(int ac, char **av) {
	try
	{
		Args ircArgs(ac, av);
		std::cout << ircArgs << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

}
