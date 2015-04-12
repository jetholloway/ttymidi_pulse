#include "serial_reader.hh"

#include <stdlib.h>

#include <unistd.h>   // For read, sleep
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
	serial_fd = ::open_serial_device(arguments.serialdevice, arguments.baudrate);
}

void SerialReader::read_midi_from_serial_port( )
{
	DataForThread dft;
	dft.serial_fd = serial_fd;
	dft.args = arguments;

	::read_midi_from_serial_port((void*)&dft);
}
