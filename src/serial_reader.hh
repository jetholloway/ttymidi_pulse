#ifndef SERIAL_READER_HH
#define SERIAL_READER_HH

#include "ttymidi.hh"

struct SerialReader
{
	const arguments_t arguments;

	SerialReader( arguments_t args_in ) :
	arguments(args_in)
	{ }

	void open_serial_device( );
	void read_midi_from_serial_port( );
	int get_fd();

private:
	int serial_fd;
};


#endif // SERIAL_READER_HH
