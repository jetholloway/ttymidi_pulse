#ifndef PULSE_DBUS_HH
#define PULSE_DBUS_HH

#include "arguments.hh"

#include <vector>
#include <map>
#include <gio/gio.h>		// for g_dbus_*

void throw_glib_errors( GError *e );

struct DBusPulseAudio
{
	const Arguments arguments;

	DBusPulseAudio( const Arguments & args_in ) :
	arguments(args_in)
	{ }

	bool connect();

	void set_client_volume( unsigned int vol_in, const char *prop_name, const char *prop_val );

	void disconnect();

private:
	bool conn_open = false;

	GDBusConnection *pulse_conn;

	std::vector<std::string> get_clients( );

	std::vector<std::string> get_sinks( );

	std::vector<std::string> get_playback_streams(
		const char * path );

	std::vector<uint32_t> get_volume( const char * path );

	void set_volume(
		const char * path,
		const std::vector<uint32_t> & vols );

	std::map<std::string,std::string> get_property_list(
		const char *interface,
		const char *path );
};

#endif  // PULSE_DBUS_HH
