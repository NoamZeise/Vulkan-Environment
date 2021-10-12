#include "app.h"
#include <stdexcept>
#include <iostream>


int main()
{

	try
	{
		#ifndef NDEBUG
		std::cout << "In debug mode" << std::endl;
		#endif
		App* app = new App();
		app->run();
		delete app;
		app = nullptr;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	int x = 0;
	std::cin >> x;
	return EXIT_SUCCESS;
}
