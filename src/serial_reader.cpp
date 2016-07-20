#include "serial_reader.hh"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

// Seconds to wait between successive attempts to re-open the serial device
#define SERIAL_DEVICE_REOPEN_SECONDS 1
#define MAX_MSG_SIZE                 1024

using namespace std;

extern int run;

//==============================================================================

void SerialReader::close_serial_device()
{
	cout << "Restoring old terminal attributes, and closing device" << endl;
	tcsetattr( this->serial_fd, TCSANOW, &this->oldtio );

	close( this->serial_fd );

	device_open = false;
}

//   Attempts to open a serial device's file.  This will fail if the serial
// device is not connected, so in that case, it will return false.  Upon
// success, it will also set 'device_open', so that the SerialReader knows that
// the device file is indeed open.
bool SerialReader::open_serial_device( )
{
	struct termios newtio;

	//   This should never happen.  If it does then that means the programmer
	// has made an error.
	if ( device_open )
	{
		cerr << "open_serial_device(): device_open = true (i.e. device already open)" << endl;
		exit(1);
	}

	// Open modem device.
	// O_RDWR:   open for reading and writing.
	// O_NOCTTY: not as controlling tty because we don't  want to get killed
	//           if linenoise sends CTRL-C.
	serial_fd = open(arguments.serialdevice, O_RDWR | O_NOCTTY );

	// save current serial port settings
	tcgetattr( this->serial_fd, &this->oldtio );

	if ( serial_fd < 0 )
		return false;

	// clear struct for new port settings
	bzero( &newtio, sizeof(newtio) );

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
	newtio.c_cc[VTIME] = 0;     // inter-character timer unused
	newtio.c_cc[VMIN]  = 1;     // blocking read until n character arrives

	// now clean the modem line and activate the settings for the port
	tcflush(serial_fd, TCIFLUSH);
	tcsetattr(serial_fd, TCSANOW, &newtio);

	// Linux-specific: enable low latency mode (FTDI "nagling off")
//	struct serial_struct ser_info;
//	ioctl(serial_fd, TIOCGSERIAL, &ser_info);
//	ser_info.flags |= ASYNC_LOW_LATENCY;
//	ioctl(serial_fd, TIOCSSERIAL, &ser_info);

	device_open = true;

	return true;
}

//   Attemps to read from a serial device's file.  Since a serial device could
// be removed at any time, this is not a reliable operation.  So, if the read
// fails, it will tell the SerialReader that the device is no longer open, and
// will return false, so that you can attempt to re-open it later
bool SerialReader::attempt_serial_read( void *buf, size_t count )
{
	long int ret = read(this->serial_fd, buf, count);

	if ( !this->arguments.silent )
	{
		if ( ret == 0 )
		// Unable to read any bytes from the device
			cerr << "No bytes could be read from the device file.  Will try to re-open the device file." << endl;
		else if ( ret == -1 )
		// An error occurred
			cerr << "Error reading from serial device.  Will try to re-open the device file." << endl;
	}

	if ( ret == 0 or ret == -1 )
	{
		this->device_open = false;
		return false;
	}
	else	// Successful read
		return true;
}

//   This is the main loop of the program.  It waits for bytes from the serial
// device, and gives them to MIDICommandHandler::parse_midi_command() to decode.
// It also handles the re-opening of the serial device if it gets closed for
// whatever reason.
void SerialReader::read_midi_from_serial_port( )
{
	unsigned char buf[3];
	char msg[MAX_MSG_SIZE];
	size_t msglen;

	//   Note: run can be set to 0 by the function exit_cli().  This happens
	// when the program gets a SIGINT or SIGTERM signal.  Until that happens,
	// just keep running in a loop
	while (run)
	{
		if ( !this->arguments.silent )
		{
			cerr << "Attempting to open device... ";
			if ( this->open_serial_device() )
				cerr << "OK." << endl;
			else
				cerr << "Failed." << endl;
		}
		else
			this->open_serial_device();

		// Lets first fast forward to first status byte...
		//   This must be done every time the device is opened, so it makes
		// sense to put this here.
		if ( this->device_open and !arguments.printonly )
		{
			do
			{
				if ( !attempt_serial_read(buf, 1) )
					break;
			}
			while ( (buf[0] & 0x80) == 0x00 );
		}

		//   Keep getting MIDI bytes as long as the device is open, and we are
		// running.
		while ( run && this->device_open )
		{
			//   Super-debug mode: only print to screen whatever comes through
			// the serial port.
			if ( arguments.printonly )
			{
				if ( !attempt_serial_read(buf, 1) )
					break;

				cout << hex << (int)buf[0] << "\t" << flush;
				continue;
			}

			// So let's align to the beginning of a MIDI command.
			int i = 1;

			while ( i < 3 )
			{
				if ( !attempt_serial_read(buf+i, 1) )
					break;

				if ( (buf[i] & 0x80) == 0x80 )
				{
					// Status byte received and will always be first byte!
					buf[0] = buf[i];
					i = 1;
				}
				else
				{
					// Data byte received
					if ( i == 2 )
						// It was 2nd data byte so we have a MIDI event process!
						i = 3;
					else
					{
						//   Lets figure out are we done or should we read one
						// more byte.
						if ( (buf[0] & 0xF0) == 0xC0 || (buf[0] & 0xF0) == 0xD0 )
							i = 3;
						else
							i = 2;
					}
				}
			}

			// Print comment message (the ones that start with 0xFF 0x00 0x00)
			if ( buf[0] == 0xFF && buf[1] == 0x00 && buf[2] == 0x00 )
			{
				if ( !attempt_serial_read(buf, 1) )
					break;

				msglen = buf[0];
				if ( msglen > MAX_MSG_SIZE - 1 )
					msglen = MAX_MSG_SIZE - 1;

				if ( !attempt_serial_read(msg, msglen) )
					break;

				if ( !arguments.silent )
				{
					// Make sure the string ends with a null character
					msg[msglen] = 0;

					cout << "0xFF Non-MIDI message: " << msg << endl;
				}
			}
			else
			// We have received a full MIDI message
				midi_command_handler->parse_midi_command(buf, arguments);
		}

		//   Don't try to re-open device until some time passes (so we don't eat
		// all of the CPU).
		sleep(SERIAL_DEVICE_REOPEN_SECONDS);
	}

	if (!arguments.silent)
		cout << "Exited loop in read_midi_from_serial_port()" << endl;
}
