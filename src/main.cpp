#include "pulse_dbus.hh"

using namespace std;

int main()
{
	vector<string> clients, mpd_stream_paths;
	GError *error =  nullptr;

	// Open the connection
	GDBusConnection *pulse_conn = get_pulseaudio_bus();

	// Get the clients
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
			v = 32768;

		set_volume( pulse_conn, stream_path.c_str(), new_vols );
	}

	// Clean up
	g_dbus_connection_close_sync(pulse_conn, nullptr, &error );
	print_errors(error);

	return 0;
}
