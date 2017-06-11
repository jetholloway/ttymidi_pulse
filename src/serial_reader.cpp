#include "serial_reader.hh"
#include "utils.hh"

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

// Seconds to wait between successive attempts to re-open the serial device
#define SERIAL_DEVICE_REOPEN_SECONDS 1
//   Seconds to wait for a message to be read from the device before we close and
// re-open it
#define NO_MESSAGE_RECEIVED_TIMEOUT  3
#define MAX_MSG_SIZE                 1024

using namespace std;

extern bool program_running;

//==============================================================================

void SerialMIDIReader::close_serial_device()
{
	tcsetattr( this->serial_fd, TCSANOW, &this->oldtio );

	close( this->serial_fd );

	device_open = false;
}

//   Attempts to open a serial device's file.  This will fail if the serial
// device is not connected, so in that case, it will return false.  Upon
// success, it will also set 'device_open', so that the SerialReader knows that
// the device file is indeed open.
bool SerialMIDIReader::open_serial_device( )
{
	struct termios newtio;

	//   This should never happen.  If it does then that means the programmer
	// has made an error.
	if ( device_open )
	{
		cerr << current_time() << "open_serial_device(): device_open = true (i.e. device already open)" << endl;
		exit(1);
	}

	// Open modem device.
	// O_RDWR:   open for reading and writing.
	// O_NOCTTY: not as controlling tty because we don't  want to get killed
	//           if linenoise sends CTRL-C.
	serial_fd = open(arguments.serialdevice.c_str(), O_RDWR | O_NOCTTY );

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

//   Attempts to read from a serial device's file.
//   Since a serial device could be removed at any time, this is not a reliable
// operation.  So, if the read fails, it will tell the SerialReader that the
// device is no longer open, and will return false, so that you can attempt to
// re-open it later.
//   Also, if the serial device returns no data within some timeout, it will be
// closed.  (The device can then be re-opened in read_midi_from_serial_port().)
bool SerialMIDIReader::attempt_serial_read( void *buf, size_t count )
{
	// If the device is not open, then just return with error
	if ( !this->device_open )
		return false;

	// Define a set of files to watch with select()
	fd_set file_set;
	FD_ZERO(&file_set);                 // Clear the set
	FD_SET(this->serial_fd, &file_set); // Add our file descriptor to the set

	// Define a timeout
	struct timeval timeout;
	timeout.tv_sec = NO_MESSAGE_RECEIVED_TIMEOUT;
	timeout.tv_usec = 0;

	// Wait to the device to become readable,
	int ret_select = select(this->serial_fd + 1, &file_set, NULL, NULL, &timeout);

	if ( !this->arguments.silent )
	{
		if( ret_select == 0 )
		// A timeout occurred
			cerr << current_time() << "Timeout reading from serial device" << endl;
		else if( ret_select == -1 )
		// An error occurred
			cerr << current_time() << "SerialReader::attempt_serial_read(): Error from select()" << endl;
	}

	if ( ret_select == 0 or ret_select == -1 )
	// An error/timeout occurred
	{
		// Close the file, and return with error
		this->close_serial_device();

		return false;
	}

	// Perform the actual read, and handle errors
	ssize_t ret_read = read(this->serial_fd, buf, count);

	if ( !this->arguments.silent )
	{
		if ( ret_read == 0 )
		// Unable to read any bytes from the device
			cerr << current_time() << "No bytes read from serial device. Will try to re-open." << endl;
		else if ( ret_read == -1 )
		// An error occurred
			cerr << current_time() << "Error reading from serial device. Will try to re-open." << endl;
	}

	if ( ret_read == 0 or ret_read == -1 )
	{
		this->device_open = false;
		return false;
	}
	else	// Successful read
		return true;
}

//   This does an iteration of the main loop of the program (when the
// 'printonly' option is selected).  It waits for bytes from the serial device,
// and prints them.  It also handles the re-opening of the serial device if it
// gets closed for whatever reason.
void SerialMIDIReader::main_loop_iteration_printonly( )
{
	unsigned char c;

	//   Super-debug mode: only print to screen whatever comes through
	// the serial port.
	if ( attempt_serial_read(&c, 1) )
		cout << hex << (int)c << "\t" << flush;
	else
	// Read failed
	{
		cerr << current_time() << "Attempting to open serial device... ";
		if ( this->open_serial_device() )
			cerr << "OK." << endl;
		else
		{
			cerr << "Failed." << endl;

			//   Don't try to re-open device until some time passes (so we don't eat
			// all of the CPU).
			sleep(SERIAL_DEVICE_REOPEN_SECONDS);
		}
	}
}

//   This does an iteration of the main loop of the program (when the
// 'printonly' option is not selected).  It waits for bytes from the serial
// device, and gives them to MIDICommandHandler::parse_midi_command() to decode.
// It also handles the re-opening of the serial device if it gets closed for
// whatever reason.
void SerialMIDIReader::main_loop_iteration_normal( )
{
	static unsigned char buf[3];

	// Get MIDI bytes as long as the device is open
	if ( this->device_open )
	{
		// So let's align to the beginning of a MIDI command.
		int i = 1;

		while ( i < 3 )
		{
			unsigned char c;

			if ( !attempt_serial_read(&c, 1) )
				return;

			// Status byte has MSb set, and will always be the first byte
			if ( (c & 0x80) == 0x80 )
				i = 0;

			buf[i] = c;

			//   Two MIDI commands ('program change' or 'mono key pressure')
			// only require 2 bytes, not 3.  So let's figure out are we done
			// or should we read one more byte.
			if ( i == 1 and ((buf[0] & 0xF0) == 0xC0 or (buf[0] & 0xF0) == 0xD0 ) )
				break;

			i++;
		}

		// Print comment message (the ones that start with 0xFF 0x00 0x00)
		if ( buf[0] == 0xFF and buf[1] == 0x00 and buf[2] == 0x00 )
		{
			if ( !attempt_serial_read(buf, 1) )
				return;

			size_t msglen;
			char msg[MAX_MSG_SIZE];

			msglen = buf[0];
			if ( msglen > MAX_MSG_SIZE - 1 )
				msglen = MAX_MSG_SIZE - 1;

			if ( !attempt_serial_read(msg, msglen) )
				return;

			// Make sure the string ends with a null character
			msg[msglen] = 0;

			if ( !arguments.silent )
				cerr << current_time() << "0xFF Non-MIDI message: " << msg << endl;
		}
		else
		// We have received a full MIDI message
			midi_command_handler->parse_midi_command(buf, arguments);
	}
	else
	// Device is not open
	{
		if ( this->open_serial_device() )
		// We just successfully opened the device
		{
			if ( !this->arguments.silent )
				cerr << current_time()  << "Connected to serial device." << endl;

			// Fast-forward to first status byte...
			//   This must be done every time the device is opened, so it makes
			// sense to put this here.
			do
			{
				if ( !attempt_serial_read(buf, 1) )
					return;
			} while ( program_running and (buf[0] & 0x80) == 0x00 );
		}
		else
		{
			if ( this->arguments.verbose )
				cerr << current_time()  << "Failed to reconnect to serial device." << endl;

			//   Don't try to re-open device until some time passes (so we don't eat
			// all of the CPU).
			sleep(SERIAL_DEVICE_REOPEN_SECONDS);
		}
	}
}

void SerialMIDIReader::main_loop_iteration( )
{
	if ( arguments.printonly )
		main_loop_iteration_printonly();
	else
		main_loop_iteration_normal();
}
