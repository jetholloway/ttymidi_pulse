#ifndef SERIAL_READER_HH
#define SERIAL_READER_HH

#include "ttymidi.hh"

#include <cstdlib>
#include <termios.h>

struct SerialReader
{
	const arguments_t arguments;

	SerialReader( arguments_t args_in ) :
	arguments(args_in), serial_fd(-1)
	{ }

	void open_serial_device( );
	void close_serial_device();
	bool attempt_serial_read( void *buf, size_t count );
	void read_midi_from_serial_port( );
	int get_fd();

private:
	int serial_fd;
	struct termios oldtio;
};


#endif // SERIAL_READER_HH
