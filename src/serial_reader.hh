#ifndef SERIAL_READER_HH
#define SERIAL_READER_HH

#include "midi_command_handler.hh"

#include <termios.h>

struct SerialMIDIReader
{
	const Arguments arguments;
	MIDICommandHandler * const midi_command_handler;

	SerialMIDIReader( const Arguments & args_in, MIDICommandHandler * const handler_in ) :
	arguments(args_in), midi_command_handler(handler_in), serial_fd(-1), device_open(false)
	{ }

	bool open_serial_device( );
	void close_serial_device();
	bool attempt_serial_read( void *buf, size_t count );
	void main_loop_iteration( );

private:
	int serial_fd;
	struct termios oldtio;
	bool device_open;

	void main_loop_iteration_printonly( );
	void main_loop_iteration_normal( );
};

#endif // SERIAL_READER_HH
