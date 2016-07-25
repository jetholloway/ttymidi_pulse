#ifndef SERIAL_READER_HH
#define SERIAL_READER_HH

#include "ttymidi.hh"

#include <cstdlib>
#include <termios.h>

struct SerialReader
{
	const Arguments arguments;
	MIDICommandHandler * const midi_command_handler;

	SerialReader( const Arguments & args_in, MIDICommandHandler * const handler_in ) :
	arguments(args_in), midi_command_handler(handler_in), serial_fd(-1), device_open(false)
	{ }

	bool open_serial_device( );
	void close_serial_device();
	bool attempt_serial_read( void *buf, size_t count );
	void read_midi_from_serial_port( );

private:
	int serial_fd;
	struct termios oldtio;
	bool device_open;

	void main_loop_printonly( );
	void main_loop_normal( );
};

#endif // SERIAL_READER_HH
