#include "serial_reader.hh"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

using namespace std;

extern int run;
void set_mpd_volume( unsigned int vol_in );
void parse_midi_command(unsigned char *buf, const arguments_t arguments );

//==============================================================================

void SerialReader::close_serial_device()
{
	cout << "Restoring old terminal attributes, and closing device" << endl;
	tcsetattr(this->serial_fd, TCSANOW, &this->oldtio);

	close(this->serial_fd);
}

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

	// save current serial port settings
	tcgetattr(this->serial_fd, &this->oldtio);

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

bool SerialReader::attempt_serial_read( void *buf, size_t count )
{
	long int ret = read(this->serial_fd, buf, count);

	// If ret is 0, then we were unable to read any bytes from the device
	if ( ret == 0 )
	{
		if (!this->arguments.silent && this->arguments.verbose)
			cerr << "No bytes could be read from the device file. Quitting." << endl;
		run = false;
		return false;
	}
	else	// Successful read
		return true;
}

void SerialReader::read_midi_from_serial_port( )
{
	unsigned char buf[3];
	char msg[MAX_MSG_SIZE];
	size_t msglen;

	// Lets first fast forward to first status byte...
	if (!arguments.printonly) {
		do
		{
			if ( !attempt_serial_read(buf, 1) )
				break;
		}
		while (buf[0] >> 7 == 0);
	}

	// Note: run can be set to 0 by the function attempt_serial_read()
	while (run)
	{
		//   super-debug mode: only print to screen whatever comes through the
		// serial port.
		if (arguments.printonly)
		{
			if ( !attempt_serial_read(buf, 1) )
				break;

			printf("%x\t", (int) buf[0]&0xFF);
			fflush(stdout);
			continue;
		}

		// So let's align to the beginning of a midi command.
		int i = 1;

		while (i < 3)
		{
			if ( !attempt_serial_read(buf+i, 1) )
				break;

			if (buf[i] >> 7 != 0) {
				// Status byte received and will always be first bit!
				buf[0] = buf[i];
				i = 1;
			} else {
				// Data byte received
				if (i == 2) {
					// It was 2nd data byte so we have a MIDI event process!
					i = 3;
				} else {
					//   Lets figure out are we done or should we read one more
					// byte.
					if ((buf[0] & 0xF0) == 0xC0 || (buf[0] & 0xF0) == 0xD0) {
						i = 3;
					} else {
						i = 2;
					}
				}
			}
		}

		// Print comment message (the ones that start with 0xFF 0x00 0x00)
		if (buf[0] == 0xFF && buf[1] == 0x00 && buf[2] == 0x00)
		{
			if ( !attempt_serial_read(buf, 1) )
				break;

			msglen = buf[0];
			if (msglen > MAX_MSG_SIZE-1) msglen = MAX_MSG_SIZE-1;

			if ( !attempt_serial_read(msg, msglen) )
				break;

			if (arguments.silent) continue;

			// Make sure the string ends with a null character
			msg[msglen] = 0;

			puts("0xFF Non-MIDI message: ");
			puts(msg);
			putchar('\n');
			fflush(stdout);
		}
		else // Parse MIDI message
			parse_midi_command(buf, arguments);
	}

	if (!arguments.silent && arguments.verbose)
		cout << "Exitted loop in read_midi_from_serial_port()" << endl;
}
