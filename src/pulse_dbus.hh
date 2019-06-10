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
