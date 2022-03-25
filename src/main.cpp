#include "app.h"
#include <stdexcept>
#ifndef NDEBUG
#include <iostream>
#endif
#include <fstream>


int main()
{
	try
	{
		#ifndef NDEBUG
		std::cout << "In debug mode" << std::endl;
		#endif
		App app;
		app.run();
	}
	catch (const std::exception& e)
	{
		#ifndef NDEBUG
		std::cerr << e.what() << std::endl;
		#else
		std::ofstream crashFile("CrashInfo.txt");
		if (crashFile.is_open())
		{
			crashFile.seekp(0);
			crashFile << e.what();
			crashFile.close();
		}
		#endif
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
