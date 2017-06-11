#include "pulse_dbus.hh"

#include <iostream>

using namespace std;

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
vector<string> gv_to_vs( GVariant *gv );

vector<uint32_t> gv_to_vuint32( GVariant *gv );

// Note: this function creates a GVariant, that must be freed later
GVariant *vuint32_to_gv( const vector<uint32_t> & vuint32 );

//==============================================================================

// Prints any Glib errors
void throw_glib_errors( GError *e )
{
	if ( e != NULL )
	{
		cerr << "Glib error has occurred: " << e->message << endl;

		throw e;
	}
	else
		return;
}

// Connects to PulseAudio via DBus
bool DBusPulseAudio::connect()
{
	GError *error = NULL;
	string pulse_server_string;

	if ( this->conn_open == true )
	{
		cerr << "DBusPulseAudio::connect(): Connection already open" << endl;
		return true;
	}

	// Try environment variable
	if ( getenv("PULSE_DBUS_SERVER") != NULL )
		pulse_server_string = getenv("PULSE_DBUS_SERVER");

	if ( pulse_server_string == "" )
	{
		// Otherwise, try using DBus
		GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
		throw_glib_errors(error);

		GDBusProxy *proxy = g_dbus_proxy_new_sync( connection,
		                                           G_DBUS_PROXY_FLAGS_NONE,
		                                           NULL,  // DBus interface
		                                           "org.PulseAudio1",   // Name
		                                           "/org/pulseaudio/server_lookup1",  // path
		                                           "org.PulseAudio1.ServerLookup1",   // interface
		                                           NULL,
		                                           &error );
		throw_glib_errors(error);

		GVariant* gvp = g_dbus_proxy_get_cached_property( proxy, "Address" );

		if ( gvp != NULL )
		{
			pulse_server_string = g_variant_get_string(gvp, NULL);

			g_variant_unref(gvp);
		}

		g_object_unref(proxy);
		g_object_unref(connection);
	}

	if ( pulse_server_string == "" )
	{
		cerr << "Unable to find PulseAudio bus name" << endl;
		return false;
	}

	cout << "Connecting to PulseAudio bus: " << pulse_server_string << endl;

	// Connect to the bus
	this->pulse_conn = g_dbus_connection_new_for_address_sync(
	                       pulse_server_string.c_str(),  // Address
	                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
	                       NULL,  // GDBusAuthObserver
	                       NULL,  // GCancellable
	                       &error );
	throw_glib_errors(error);

	this->conn_open = true;
	return true;
}

//   Gets general things from PulseAudio.  This could include: clients, sinks,
// etc.  It returns them in a GVariant.  This function is meant to be wrapped by
// another function which returns the thigns in a nicer data structure
GVariant* get_things_gv( GDBusConnection *conn, const char *get_method_name, const char * interface, const char * path )
{
	GVariant *temp_tva, *temp_va, *temp_a;
	GError *error = NULL;

	temp_tva = g_dbus_connection_call_sync(
		conn,
		NULL,                              // Bus name
		path,                              // Path of object
		"org.freedesktop.DBus.Properties", // Interface name
		"Get",                             // Method name
		g_variant_new("(ss)",interface,get_method_name), // Params
		NULL,                              // reply type
		G_DBUS_CALL_FLAGS_NONE,
		-1,                                // Timeout
		NULL,                              // Cancellable
		&error
	);
	throw_glib_errors(error);

	// extract the array out of the tuple of variant
	temp_va = g_variant_get_child_value(temp_tva,0);
	temp_a = g_variant_get_variant(temp_va);

	// Clean up
	g_variant_unref(temp_va);
	g_variant_unref(temp_tva);

	return temp_a;
}

void set_things_gv( GDBusConnection *conn, const char *get_method_name, const char * interface, const char * path, GVariant* input )
{
	GVariant *temp_tva;
	GError *error = NULL;

	temp_tva = g_dbus_connection_call_sync(
		conn,
		NULL,                              // Bus name
		path,                              // Path of object
		"org.freedesktop.DBus.Properties", // Interface name
		"Set",                             // Method name
		g_variant_new("(ssv)",interface,get_method_name,input), // Params
		NULL,                              // reply type
		G_DBUS_CALL_FLAGS_NONE,
		-1,                                // Timeout
		NULL,                              // Cancellable
		&error
	);
	throw_glib_errors(error);

	// Clean up
	g_variant_unref(temp_tva);
}

// Note: this function deletes the GVariant input
vector<string> gv_to_vs( GVariant *gv )
{
	vector<string> answer;

	// Convert the Gvariant array-of-strings into a vector<string>
	answer.resize(g_variant_n_children(gv));
	for ( size_t i = 0; i < g_variant_n_children(gv); i++ )
	{
		GVariant *g_s = g_variant_get_child_value(gv,i);
		answer[i] = g_variant_get_string(g_s, NULL);
		g_variant_unref(g_s);
	}
	g_variant_unref(gv);

	return answer;
}

// Note: this function deletes the GVariant input
vector<uint32_t> gv_to_vuint32( GVariant *gv )
{
	vector<uint32_t> answer;

	// Convert the Gvariant array-of-strings into a vector<string>
	answer.resize(g_variant_n_children(gv));
	for ( size_t i = 0; i < g_variant_n_children(gv); i++ )
	{
		GVariant *g_s = g_variant_get_child_value(gv,i);
		answer[i] = g_variant_get_uint32(g_s);
		g_variant_unref(g_s);
	}
	g_variant_unref(gv);

	return answer;
}

// Note: this function creates a GVariant, that must be freed later
GVariant *vuint32_to_gv( const vector<uint32_t> & vuint32 )
{
	GVariantBuilder *builder;
	GVariant *answer;

	// Create a 'builder' object
	builder = g_variant_builder_new(G_VARIANT_TYPE("au"));

	// Add all of the values to the builder
	for ( uint32_t u : vuint32 )
		g_variant_builder_add(builder, "u", u);

	// Convert the builder into an actual GVariant
	answer = g_variant_new("au", builder);

	// Clean up
	g_variant_builder_unref(builder);

	return answer;
}

// Gets all of the clients, and returns their DBus paths
vector<string> DBusPulseAudio::get_clients( )
{
	GVariant * gv = get_things_gv( this->pulse_conn, "Clients", "org.PulseAudio.Core1", "/org/pulseaudio/core1" );
	return gv_to_vs(gv);
}

// Gets all of the sinks, and returns their DBus paths
vector<string> DBusPulseAudio::get_sinks( )
{
	GVariant * gv = get_things_gv( this->pulse_conn, "Sinks", "org.PulseAudio.Core1", "/org/pulseaudio/core1" );
	return gv_to_vs(gv);
}

// Gets all of a clients playback streams, and returns their DBus paths
vector<string> DBusPulseAudio::get_playback_streams( const char * path )
{
	GVariant * gv = get_things_gv( this->pulse_conn, "PlaybackStreams", "org.PulseAudio.Core1.Client", path );
	return gv_to_vs(gv);
}

// Gets a stream's volume
vector<uint32_t> DBusPulseAudio::get_volume( const char * path )
{
	GVariant * gv = get_things_gv( this->pulse_conn, "Volume", "org.PulseAudio.Core1.Stream", path );
	return gv_to_vuint32(gv);
}

// Sets a stream's volume
void DBusPulseAudio::set_volume( const char * path, const vector<uint32_t> & vols )
{
	GVariant * gv = vuint32_to_gv(vols);
	set_things_gv( this->pulse_conn, "Volume", "org.PulseAudio.Core1.Stream", path, gv );
	// Apparently you don't have to free 'gv', as it is already freed or some shit
}

// Get's a PulseAudio object's properties
map<string,string> DBusPulseAudio::get_property_list( const char *interface, const char *path )
{
	GVariant *gv_adsab;
	map<string,string> answer;

	// Make the DBus call to get the data
	gv_adsab = get_things_gv(this->pulse_conn, "PropertyList", interface, path);

	for ( size_t i = 0; i < g_variant_n_children(gv_adsab); i++ )
	{
		GVariant *property, *key_gv, *data_gv;

		property = g_variant_get_child_value(gv_adsab,i);

		// Unpack the shitty dictionary entry
		string key, data;

		if ( g_variant_n_children(property) != 2 )
		{
			cerr << "Property is a dictionary entry which does not have 2 children" << endl;
			exit(1);
		}

		key_gv  = g_variant_get_child_value(property,0);
		data_gv = g_variant_get_child_value(property,1);

		// data_gv is an array of bytes, not a fucking string, so decode that
		// But do not include the trailing '\0'
		for ( size_t j = 0; j < g_variant_n_children(data_gv) - 1; j++ )
		{
			GVariant *byte_gv = g_variant_get_child_value(data_gv, j);
			data.append(1, g_variant_get_byte(byte_gv));
			g_variant_unref(byte_gv);
		}

		key  = g_variant_get_string(key_gv, NULL);

		answer[key] = data;

		// Clean up
		g_variant_unref(key_gv);
		g_variant_unref(data_gv);
		g_variant_unref(property);
	}

	// Clean up
	g_variant_unref(gv_adsab);

	return answer;
}

//   This may fail, if there is no connection to pulseaudio, but it will not
// crash the prgoram.
void DBusPulseAudio::set_client_volume( unsigned int vol_in, const char *prop_name, const char *prop_val )
{
	vector<string> clients, mpd_stream_paths;

	if ( this->conn_open == false )
	{
		// Attempt to re-open the connection
		if ( !this->connect() )
		{
			cerr << "DBusPulseAudio::set_client_volume(): the connection is closed" << endl;
			return;
		}
	}

	try
	{
		// Get the pulse clients
		clients = this->get_clients();

		// Go through each client
		for ( const string & c : clients )
		{
			map<string,string> properties = this->get_property_list("org.PulseAudio.Core1.Client", c.c_str() );

			if ( properties.find(prop_name) != properties.end() and properties[prop_name] == (string)prop_val )
				for ( const string & path : this->get_playback_streams( c.c_str() ) )
					mpd_stream_paths.push_back(path);
		}

		// Go through each stream and set the volume
		for ( const string & stream_path : mpd_stream_paths )
		{
			// Note that the maximum volume is supposedly 65535
			vector<uint32_t> old_vols, new_vols;

			old_vols = this->get_volume( stream_path.c_str() );

			new_vols = old_vols;
			for ( uint32_t & v : new_vols )
				v = vol_in;

			this->set_volume( stream_path.c_str(), new_vols );
		}
	}
	catch ( GError * e )
	{
		if ( e->domain == g_dbus_error_quark() and
		     e->code == G_DBUS_ERROR_UNKNOWN_METHOD )
		// "GDBus.Error:org.freedesktop.DBus.Error.UnknownMethod"
		// This is the error that occurs when a pulseaudio client
		// disappears half way through set_pulse_client_volume()
		{
			//   Silently ignore this, because it's not really an error.  It's
			// just the same outcome as if zero instances of the client were
			// running in the first place.
			g_error_free(e);
		}
		else if ( e->domain == g_io_error_quark() and
		          e->code == G_IO_ERROR_CLOSED )
		// "The connection is closed"
		// This happens when we kill pulseaudio while tty_pulse is running
		{
			cerr << "Pulseaudio connection has closed" << endl;
			g_error_free(e);
			this->conn_open = false;
		}
		else
			throw e;
	}
}

void DBusPulseAudio::disconnect()
{
	if ( this->conn_open == true )
	{
		GError *error = nullptr;

		g_dbus_connection_close_sync(this->pulse_conn, nullptr, &error );
		g_object_unref(this->pulse_conn);
		throw_glib_errors(error);

		this->conn_open = false;
	}
}
