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


#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <argp.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>   // For read, sleep
// Linux-specific

#define FALSE                         0
#define TRUE                          1

#define MAX_DEV_STR_LEN               32
#define MAX_MSG_SIZE                1024

// change this definition for the correct port
//#define _POSIX_SOURCE 1 // POSIX compliant source

// This is a global variable so you know when the threads have to stop running
int run;

//------------------------------------------------------------------------------
// Program options

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
void arg_set_defaults(arguments_t *arguments_local);
arguments_t parse_all_the_arguments(int argc, char** argv);
void parse_midi_command(unsigned char *buf, const arguments_t arguments );
void* read_midi_from_serial_port( void* data_for_thread );
int open_serial_device( const char * filename, unsigned int baudrate );

//------------------------------------------------------------------------------
// Main program

int main(int argc, char** argv)
{
	DataForThread data_for_thread;
	struct termios oldtio;

	// Parse the arguments
	data_for_thread.args = parse_all_the_arguments(argc, argv);

	// Open the serial port device
	data_for_thread.serial_fd = open_serial_device(data_for_thread.args.serialdevice, data_for_thread.args.baudrate);
	// save current serial port settings
	tcgetattr(data_for_thread.serial_fd, &oldtio);

	if (data_for_thread.args.printonly)
	{
		printf("Super debug mode: Only printing the signal to screen. Nothing else.\n");
	}

	// Start the thread that polls serial data
	pthread_t midi_in_thread;
	run = TRUE;
	// Thread for polling serial data. As serial is currently read in blocking
	//  mode, by this we can enable ctrl+c quiting and avoid zombie alsa ports
	//  when killing app with ctrl+z
	pthread_create(&midi_in_thread, NULL, read_midi_from_serial_port, (void*) &data_for_thread);
	signal(SIGINT, exit_cli);
	signal(SIGTERM, exit_cli);

	while (run)
	{
		sleep(100);
	}

	// restore the old port settings
	tcsetattr(data_for_thread.serial_fd, TCSANOW, &oldtio);
	printf("\ndone!\n");

	return 0;
}
