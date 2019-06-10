/*
	Copyright 2019 Jet Holloway

	This file is part of ttymidi_pulse.

	ttymidi_pulse is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ttymidi_pulse is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ttymidi_pulse.  If not, see <http://www.gnu.org/licenses/>.
*/

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
