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

#include "arguments.hh"

#include <termios.h>
#include <argp.h>
#include <iostream>

using namespace std;

static error_t parse_opt(int key, char *arg, struct argp_state *state);
void arg_set_defaults(Arguments *arguments_local);

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
			if (arg == NULL)
				break;
			arguments->serialdevice = arg;
			break;
		case 'b':
			if (arg == NULL)
				break;
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
						cerr << "Baud rate " << baud_temp << " is not supported." << endl;
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

// These are actual global variables which argp wants
const char *argp_program_version     = "ttymidi 0.60";
const char *argp_program_bug_address = "tvst@hotmail.com";

Arguments::Arguments()
{
	this->printonly = false;
	this->silent    = false;
	this->verbose   = false;
	this->baudrate  = B115200;
	this->serialdevice = "/dev/ttyUSB0";
}

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
