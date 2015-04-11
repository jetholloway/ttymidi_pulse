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

#include "ttymidi.h"

#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>   // For read, sleep

// This is a global variable so you know when the threads have to stop running
int run;

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
