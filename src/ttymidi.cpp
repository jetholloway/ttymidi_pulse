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

#include "ttymidi.hh"

#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <argp.h>
#include <iostream>

using namespace std;

// This is a global variable so you know when the threads have to stop running
extern bool program_running;

static error_t parse_opt(int key, char *arg, struct argp_state *state);
void arg_set_defaults(Arguments *arguments_local);

void set_mpd_volume( unsigned int vol_in );

//------------------------------------------------------------------------------
// Program options

static struct argp_option options[] =
{
	{"serialdevice" , 's', "DEV" , 0, "Serial device to use. Default = /dev/ttyUSB0", 0 },
	{"baudrate"     , 'b', "BAUD", 0, "Serial port baud rate. Default = 115200", 0 },
	{"verbose"      , 'v', 0     , 0, "For debugging: Produce verbose output", 0 },
	{"printonly"    , 'p', 0     , 0, "Super debugging: Print values read from serial -- and do nothing else", 0 },
	{"quiet"        , 'q', 0     , 0, "Don't produce any output, even when the print command is sent", 0 },
	{ 0             , 0  , 0     , 0, 0,                                                 0 }
};

void exit_cli(__attribute__((unused)) int sig)
{
	program_running = false;
	cout << "ttymidi closing down ... " << endl;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	//   Get the input argument from argp_parse, which we know is a pointer to
	// our arguments structure.
	Arguments *arguments = (Arguments*)state->input;
	long unsigned int baud_temp;

	switch (key)
	{
		case 'p':
			arguments->printonly = true;
			break;
		case 'q':
			arguments->silent = true;
			break;
		case 'v':
			arguments->verbose = true;
			break;
		case 's':
			if (arg == NULL) break;
			arguments->serialdevice = arg;
			break;
		case 'b':
			if (arg == NULL) break;
			baud_temp = strtoul(arg, NULL, 0);
			if (baud_temp != EINVAL and baud_temp != ERANGE)
				switch (baud_temp)
				{
					case 1200   : arguments->baudrate = B1200  ; break;
					case 2400   : arguments->baudrate = B2400  ; break;
					case 4800   : arguments->baudrate = B4800  ; break;
					case 9600   : arguments->baudrate = B9600  ; break;
					case 19200  : arguments->baudrate = B19200 ; break;
					case 38400  : arguments->baudrate = B38400 ; break;
					case 57600  : arguments->baudrate = B57600 ; break;
					case 115200 : arguments->baudrate = B115200; break;
					default:
						cout << "Baud rate " << baud_temp << " is not supported." << endl;
						exit(1);
				}

		case ARGP_KEY_ARG:
		case ARGP_KEY_END:
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

Arguments::Arguments()
{
	this->printonly = false;
	this->silent    = false;
	this->verbose   = false;
	this->baudrate  = B115200;
	this->serialdevice = "/dev/ttyUSB0";
}

// These are actual global variables which argp wants
const char *argp_program_version     = "ttymidi 0.60";
const char *argp_program_bug_address = "tvst@hotmail.com";

Arguments parse_all_the_arguments(int argc, char** argv)
{
	Arguments answer;
	static char doc[] = "ttymidi - Connect serial port devices to ALSA MIDI programs!";
	static struct argp argp = { options, parse_opt, 0, doc, NULL, NULL, NULL };

	argp_parse(&argp, argc, argv, 0, 0, &answer);

	if ( answer.verbose and answer.silent )
	{
		cerr << "Options 'verbose' and 'silent' are mutually exclusive" << endl;
		exit(1);
	}

	if ( answer.printonly and answer.silent )
	{
		cerr << "Options 'printonly' and 'silent' are mutually exclusive" << endl;
		exit(1);
	}

	return answer;
}



//------------------------------------------------------------------------------
// MIDI stuff

void MIDICommandHandler::parse_midi_command(unsigned char *buf, const Arguments arguments )
{
	/*
	   MIDI COMMANDS
	   -------------------------------------------------------------------
	   name                 status      param 1          param 2
	   -------------------------------------------------------------------
	   note off             0x80+C       key #            velocity
	   note on              0x90+C       key #            velocity
	   poly key pressure    0xA0+C       key #            pressure value
	   control change       0xB0+C       control #        control value
	   program change       0xC0+C       program #        --
	   mono key pressure    0xD0+C       pressure value   --
	   pitch bend           0xE0+C       range (LSB)      range (MSB)
	   system               0xF0+C       manufacturer     model
	   -------------------------------------------------------------------
	   C is the channel number, from 0 to 15;
	   -------------------------------------------------------------------
	   source: http://ftp.ec.vanderbilt.edu/computermusic/musc216site/MIDI.Commands.html

	   In this program the pitch bend range will be transmitter as
	   one single 8-bit number. So the end result is that MIDI commands
	   will be transmitted as 3 bytes, starting with the operation byte:

	   buf[0] --> operation/channel
	   buf[1] --> param1
	   buf[2] --> param2        (param2 not transmitted on program change or key press)
   */

	int operation, channel, param1, param2;

	operation = buf[0] & 0xF0;
	channel   = buf[0] & 0x0F;
	param1    = buf[1];
	param2    = buf[2];

	switch (operation)
	{
		case 0x80:
			if (arguments.verbose)
				printf("Serial  0x%x Note off           %03u %03u %03u\n", operation, channel, param1, param2);
			this->note_on(channel, param1, param2);
			break;

		case 0x90:
			if (arguments.verbose)
				printf("Serial  0x%x Note on            %03u %03u %03u\n", operation, channel, param1, param2);
			this->note_off(channel, param1, param2);
			break;

		case 0xA0:
			if (arguments.verbose)
				printf("Serial  0x%x Pressure change    %03u %03u %03u\n", operation, channel, param1, param2);
			this->aftertouch(channel, param1, param2);
			break;

		case 0xB0:
			if (arguments.verbose)
				printf("Serial  0x%x Controller change  %03u %03u %03u\n", operation, channel, param1, param2);
			this->controller_change(channel, param1, param2);
			break;

		case 0xC0:
			if (arguments.verbose)
				printf("Serial  0x%x Program change     %03u %03u\n", operation, channel, param1);
			this->program_change(channel, param1);
			break;

		case 0xD0:
			if (arguments.verbose)
				printf("Serial  0x%x Channel pressure   %03u %03u\n", operation, channel, param1);
			this->channel_pressure(channel, param1);
			break;

		case 0xE0:
			param1 = (param1 & 0x7F) + ((param2 & 0x7F) << 7);
			if (arguments.verbose)
				printf("Serial  0x%x Pitch bend         %03u %05i\n", operation, channel, param1);
			this->pitch_bend(channel, param1 - 8192); // in alsa MIDI we want signed int
			break;

		// Not implementing system commands (0xF0)

		default:
			if (!arguments.silent)
				printf("0x%x Unknown MIDI cmd   %03u %03u %03u\n", operation, channel, param1, param2);
			break;
	}
}
