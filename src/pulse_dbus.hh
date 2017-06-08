#ifndef PULSE_DBUS_HH
#define PULSE_DBUS_HH

#include <vector>
#include <map>
#include <gio/gio.h>		// for g_dbus_*

void throw_glib_errors( GError *e );

GDBusConnection* get_pulseaudio_bus();

std::vector<std::string> get_clients( GDBusConnection *conn );

std::vector<std::string> get_sinks( GDBusConnection *conn );

std::vector<std::string> get_playback_streams(
	GDBusConnection *conn,
	const char * path );

std::vector<uint32_t> get_volume( GDBusConnection *conn, const char * path );

void set_volume(
	GDBusConnection *conn,
	const char * path,
	const std::vector<uint32_t> & vols );

std::map<std::string,std::string> get_property_list(
	GDBusConnection* conn,
	const char *interface,
	const char *path );

#endif  // PULSE_DBUS_HH
