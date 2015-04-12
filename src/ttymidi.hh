/*
	This file is part of ttymidi.

	ttymidi is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ttymidi is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ttymidi.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TTYMIDI_H
#define TTYMIDI_H

#define MAX_DEV_STR_LEN               32
#define MAX_MSG_SIZE                1024


typedef struct _arguments
{
	int  silent, verbose, printonly;
	char serialdevice[MAX_DEV_STR_LEN];
	unsigned int  baudrate;
	char name[MAX_DEV_STR_LEN];
} arguments_t;

typedef struct _DataForThread
{
	int serial_fd;
	arguments_t args;
} DataForThread;

void exit_cli(int sig);
arguments_t parse_all_the_arguments(int argc, char** argv);

#endif // TTYMIDI_H
