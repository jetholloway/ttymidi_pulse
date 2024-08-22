/*
	Copyright 2019 Jet Holloway

	This file is part of ttymidi_pulse.

	ttymidi_pulse is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ttymidi_pulse is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ttymidi_pulse.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "utils.hh"

#include <ctime>

using namespace std;

string current_time()
{
	// Note: We could use 'put_time' from C++, but this is hardly neater as we would have to use a silly ostringstream, and it is slower anyway.
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 80, "[%Y-%m-%d %H:%M:%S] ", timeinfo);

	return string(buffer);
}
