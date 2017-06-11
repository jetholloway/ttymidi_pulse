#ifndef PULSE_DBUS_HH
#define PULSE_DBUS_HH

#include <vector>
#include <map>
#include <gio/gio.h>		// for g_dbus_*

void throw_glib_errors( GError *e );

struct DBusPulseAudio
{
	void connect();

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

	void disconnect();

private:
	GDBusConnection *pulse_conn;
};

#endif  // PULSE_DBUS_HH
