#include "utils.hh"

using namespace std;

string current_time()
{
	/// FIXME: GCC doesn't implement "std::put_time()" until version 5
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 80, "[%Y-%m-%d %H:%M:%S] ", timeinfo);

	return string(buffer);
}
