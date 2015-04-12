#include "pulse_dbus.hh"
#include "ttymidi.hh"
#include "serial_reader.hh"

#include <termios.h>
#include <signal.h>
#include <unistd.h>   // For read, sleep

#include <iostream>
#include <thread>

using namespace std;

// This is a global variable so you know when the threads have to stop running
int run;
GDBusConnection *pulse_conn;

void set_mpd_volume( unsigned int vol_in );

void set_mpd_volume( unsigned int vol_in )
{
	vector<string> clients, mpd_stream_paths;

	// Get the pulse clients
	clients = get_clients(pulse_conn);

	// Go through each client
	for ( const string & c : clients )
	{
		map<string,string> properties = get_property_list(pulse_conn, "org.PulseAudio.Core1.Client", c.c_str() );

		if ( properties["application.name"] == (string)"Music Player Daemon" )
			for ( const string & path : get_playback_streams( pulse_conn, c.c_str() ) )
				mpd_stream_paths.push_back(path);
	}

	// Go through each stream and set the volume
	for ( const string & stream_path : mpd_stream_paths )
	{
		// Note that the maximum volume is supposedly 65535
		vector<uint32_t> old_vols, new_vols;

		old_vols = get_volume(pulse_conn, stream_path.c_str() );

		new_vols = old_vols;
		for ( uint32_t & v : new_vols )
			v = vol_in;

		set_volume( pulse_conn, stream_path.c_str(), new_vols );
	}
}

int main(int argc, char** argv)
{
	GError *error =  nullptr;
	arguments_t arguments;

	// Parse the arguments
	arguments = parse_all_the_arguments(argc, argv);

	// Open the serial port device
	SerialReader serial_reader(arguments);
	serial_reader.open_serial_device();

	if (arguments.printonly)
	{
		cout << "Super debug mode: Only printing the signal to screen. Nothing else." << endl;
	}

	// Open the DBus connection
	pulse_conn = get_pulseaudio_bus();

	//------------------------------------------------------
	// Start the thread that polls serial data
	run = true;
	// Thread for polling serial data. As serial is currently read in blocking
	//  mode, by this we can enable ctrl+c quiting and avoid zombie alsa ports
	//  when killing app with ctrl+z
	thread thr(&SerialReader::read_midi_from_serial_port, serial_reader);
	thr.detach();

	signal(SIGINT, exit_cli);
	signal(SIGTERM, exit_cli);

	while (run)
	{
		sleep(1);
		cout << "running main loop. run = " << run << endl;

		if ( thr.joinable() )
		{
			cout << "joining thread" << endl;
			thr.join();
			cout << "joined thread" << endl;
		}
	}

	cout << "exitted loop in main()" << endl;

	//------------------------------------------------------
	// restore the old port settings
	serial_reader.close_serial_device();
	cout << endl << "done!" << endl;

	// Clean up DBus things
	g_dbus_connection_close_sync(pulse_conn, nullptr, &error );
	print_errors(error);

	return 0;
}
