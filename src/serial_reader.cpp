#include "serial_reader.hh"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <cstdio>

using namespace std;

extern int run;
void set_mpd_volume( unsigned int vol_in );

//==============================================================================

int SerialReader::get_fd()
{
	return serial_fd;
}

void SerialReader::open_serial_device( )
{
	struct termios newtio;

	// Open modem device.
	// O_RDWR:   open for reading and writing.
	// O_NOCTTY: not as controlling tty because we don't  want to get killed
	//           if linenoise sends CTRL-C.
	serial_fd = open(arguments.serialdevice, O_RDWR | O_NOCTTY );

	if (serial_fd < 0)
	{
		perror(arguments.serialdevice);
		exit(-1);
	}

	// clear struct for new port settings
	bzero(&newtio, sizeof(newtio));

	/*
	 * BAUDRATE : Set bps rate. You could also use cfsetispeed and cfsetospeed.
	 * CRTSCTS  : output hardware flow control (only used if the cable has
	 * all necessary lines. See sect. 7 of Serial-HOWTO)
	 * CS8      : 8n1 (8bit, no parity, 1 stopbit)
	 * CLOCAL   : local connection, no modem contol
	 * CREAD    : enable receiving characters
	 */
	newtio.c_cflag = arguments.baudrate | CS8 | CLOCAL | CREAD; // CRTSCTS removed

	/*
	 * IGNPAR  : ignore bytes with parity errors
	 * ICRNL   : map CR to NL (otherwise a CR input on the other computer
	 * will not terminate input)
	 * otherwise make device raw (no other input processing)
	 */
	newtio.c_iflag = IGNPAR;

	// Raw output
	newtio.c_oflag = 0;

	/*
	 * ICANON  : enable canonical input
	 * disable all echo functionality, and don't send signals to calling program
	 */
	newtio.c_lflag = 0; // non-canonical

	// Set up: we'll be reading 4 bytes at a time.
	newtio.c_cc[VTIME]    = 0;     // inter-character timer unused
	newtio.c_cc[VMIN]     = 1;     // blocking read until n character arrives

	// now clean the modem line and activate the settings for the port
	tcflush(serial_fd, TCIFLUSH);
	tcsetattr(serial_fd, TCSANOW, &newtio);

	// Linux-specific: enable low latency mode (FTDI "nagling off")
//	struct serial_struct ser_info;
//	ioctl(serial_fd, TIOCGSERIAL, &ser_info);
//	ser_info.flags |= ASYNC_LOW_LATENCY;
//	ioctl(serial_fd, TIOCSSERIAL, &ser_info);
}

void SerialReader::read_midi_from_serial_port( )
{
	DataForThread dft;
	dft.serial_fd = serial_fd;
	dft.args = arguments;

	::read_midi_from_serial_port((void*)&dft);
}
