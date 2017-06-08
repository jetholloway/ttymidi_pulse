#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <string>

struct Arguments
{
	bool silent, verbose, printonly;
	unsigned int baudrate;
	std::string serialdevice;

	Arguments();
};

Arguments parse_all_the_arguments(int argc, char** argv);

#endif // ARGUMENTS_H
