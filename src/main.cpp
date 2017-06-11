#include "pulse_dbus.hh"
#include "serial_reader.hh"

#include <signal.h>
#include <unistd.h>   // For read, sleep

#include <iostream>
#include <thread>
#include <cmath>

using namespace std;

// This is a global variable so you know when the threads have to stop running
bool program_running;
DBusPulseAudio dbus_pulse;

// Function to quit program upon receiving a SIGINT or SIGTERM
void exit_cli(int sig);
void exit_cli(__attribute__((unused)) int sig)
{
	program_running = false;
	cout << "ttymidi closing down ... " << endl;
}

void set_pulse_client_volume( unsigned int vol_in, const char *prop_name, const char *prop_val );

void set_pulse_client_volume( unsigned int vol_in, const char *prop_name, const char *prop_val )
{
	vector<string> clients, mpd_stream_paths;

	// Get the pulse clients
	clients = dbus_pulse.get_clients();

	// Go through each client
	for ( const string & c : clients )
	{
		map<string,string> properties = dbus_pulse.get_property_list("org.PulseAudio.Core1.Client", c.c_str() );

		if ( properties.find(prop_name) != properties.end() and properties[prop_name] == (string)prop_val )
			for ( const string & path : dbus_pulse.get_playback_streams( c.c_str() ) )
				mpd_stream_paths.push_back(path);
	}

	// Go through each stream and set the volume
	for ( const string & stream_path : mpd_stream_paths )
	{
		// Note that the maximum volume is supposedly 65535
		vector<uint32_t> old_vols, new_vols;

		old_vols = dbus_pulse.get_volume( stream_path.c_str() );

		new_vols = old_vols;
		for ( uint32_t & v : new_vols )
			v = vol_in;

		dbus_pulse.set_volume( stream_path.c_str(), new_vols );
	}
}

struct Fader_Program_Mapping
{
	int channel;
	const char *prop_name, *prop_val;
};

//   This is a concrete example of a MIDICommandHandler.  When we get a MIDI
// command, we will use PulseAudio to control some volumes.  This is the only
// piece of code which connects the 'ttymidi' side with the 'Pulse DBus' side.
struct MIDIHandler_Program_Volume : MIDICommandHandler
{
	const size_t nr_rules = 3;
	const Fader_Program_Mapping rules[3] =
	{
		// MIDI Channel nr, pulse property, pulse property value
		{0, "application.name", "Music Player Daemon"},
		{1, "application.process.binary", "mplayer2"},
		{2, "application.name", "CubebUtils"},
	};

	virtual void pitch_bend(int channel, int pitch)
	{
		for ( const auto & rule : rules )
		{
			if ( channel == rule.channel )
			{
				double x = 4*(pitch+8192); // number 0-65535
				//   All of the following constants I got from a Log fit in
				// gnumeric.  I plotted the data of 'fader level' vs 'fader
				// travel in mm'.
				double y = 18864.560759108*log(x+2046.27968)-144258.687272491 ;
				int y2 = (int)y;
				if ( y2 < 0 )
					y2 = 0;
				if ( y2 > 65535)
					y2 = 65535;
				try
				{
					set_pulse_client_volume(y2, rule.prop_name, rule.prop_val);
				}
				catch ( GError * e )
				{
					// We only really want to catch these errors:
					// "GDBus.Error:org.freedesktop.DBus.Error.UnknownMethod"
					// This is the error that occurs when a pulseaudio client
					// disappears half way through set_pulse_client_volume()
					if ( e->domain == g_dbus_error_quark() and
					     e->code == G_DBUS_ERROR_UNKNOWN_METHOD )
						g_error_free(e);
					else
						throw e;
				}
			}
		}
	}
};

void main_loop(SerialMIDIReader &serial_reader);
void main_loop(SerialMIDIReader &serial_reader)
{
	//   Note: program_running can be set to 0 by the function exit_cli().  This
	// happens when the program gets a SIGINT or SIGTERM signal.  Until that
	// happens, just keep running in a loop.
	while (program_running)
	{
		serial_reader.main_loop_iteration();
	}

	if (!serial_reader.arguments.silent)
		cout << "Exited loop in main_loop()" << endl;
}

int main(int argc, char** argv)
{
	Arguments arguments;
	MIDIHandler_Program_Volume handler;

	// Parse the command-line arguments
	arguments = parse_all_the_arguments(argc, argv);

	// Create an object to handle the serial device
	SerialMIDIReader serial_reader(arguments, &handler);

	if (arguments.printonly)
		cout << "Super debug mode: Only printing the signal to screen. Nothing else." << endl;

	// Open the DBus connection
	dbus_pulse.connect();

	//------------------------------------------------------
	// Start the thread that polls serial data
	program_running = true;
	//   Thread for polling serial data.  As serial is currently read in
	// blocking mode, by this we can enable ctrl+C quitting and avoid zombie
	// alsa ports when killing app with ctrl+Z
	thread thr(&main_loop, ref(serial_reader));
	thr.detach();

	signal(SIGINT, exit_cli);
	signal(SIGTERM, exit_cli);

	//   Do nothing.  This thread just waits until program_running=false (which
	// is set by exit_cli() when we get a SIGINT or SIGTERM.
	while (program_running)
		sleep(1);

	if ( !arguments.silent )
		cout << "exitted loop in main()" << endl;

	//------------------------------------------------------
	// restore the old port settings
	if ( !arguments.silent )
		cout << "Restoring old terminal attributes, and closing device" << endl;
	serial_reader.close_serial_device();
	if ( !arguments.silent )
		cout << endl << "done!" << endl;

	// Clean up DBus things
	dbus_pulse.disconnect();

	return 0;
}
