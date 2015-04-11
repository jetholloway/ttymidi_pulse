#ifndef PULSE_DBUS_HH
#define PULSE_DBUS_HH

#include <vector>
#include <map>
#include <gio/gio.h>		// for g_dbus_*

bool print_errors( GError *e );

GDBusConnection* get_pulseaudio_bus();

GVariant* get_things_gv(
	GDBusConnection *conn,
	const char *get_method_name,
	const char * interface,
	const char * path );

void set_things_gv(
	GDBusConnection *conn,
	const char *get_method_name,
	const char * interface,
	const char * path, GVariant* input );

// Note: these functions delete the GVariant input
std::vector<std::string> gv_to_vs( GVariant *gv );

std::vector<uint32_t> gv_to_vuint32( GVariant *gv );

// Note: this function creates a GVariant, that must be freed later
GVariant *vuint32_to_gv( const std::vector<uint32_t> & vuint32 );

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
