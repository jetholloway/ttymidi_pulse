/*
	This file is part of ttymidi.

	ttymidi is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ttymidi is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ttymidi.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TTYMIDI_H
#define TTYMIDI_H

#define MAX_DEV_STR_LEN               32

typedef struct _arguments
{
	bool silent, verbose, printonly;
	char serialdevice[MAX_DEV_STR_LEN];
	unsigned int  baudrate;
	char name[MAX_DEV_STR_LEN];
} arguments_t;

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

	void parse_midi_command(unsigned char *buf, const arguments_t arguments );
};

void exit_cli(int sig);
arguments_t parse_all_the_arguments(int argc, char** argv);

#endif // TTYMIDI_H
