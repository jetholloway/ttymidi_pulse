/*
	Copyright 2011 Thiago Teixeira
	          2012 Jari Suominen
	          2019 Jet Holloway

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

#ifndef MIDI_COMMAND_HANDLER_H
#define MIDI_COMMAND_HANDLER_H

#include "arguments.hh"

//   This is a struct which does something with MIDI commands.  You need to
// instantiate a concrete class which inherits from this, because otherwise
// the program won't have anything to do with all of the MIDI commands it is
// recieving.
struct MIDICommandHandler
{
	virtual void note_on(__attribute__((unused)) int channel, __attribute__((unused)) int key, __attribute__((unused)) int velocity) {}
	virtual void note_off(__attribute__((unused)) int channel, __attribute__((unused)) int key, __attribute__((unused)) int velocity) {}
	virtual void aftertouch(__attribute__((unused)) int channel, __attribute__((unused)) int key, __attribute__((unused)) int pressure) {}
	virtual void controller_change(__attribute__((unused)) __attribute__((unused)) int channel, __attribute__((unused)) int controller_nr, __attribute__((unused)) int controller_value) {}
	virtual void program_change(__attribute__((unused)) int channel, __attribute__((unused)) int program_nr) {}
	virtual void channel_pressure(__attribute__((unused)) int channel, __attribute__((unused)) int pressure) {}
	virtual void pitch_bend(__attribute__((unused)) int channel, __attribute__((unused)) int pitch) {}

	void parse_midi_command(unsigned char *buf, const Arguments & arguments );
};

#endif // MIDI_COMMAND_HANDLER_H
