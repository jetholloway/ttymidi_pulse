#include <cstdlib>          // for exit()
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <gio/gio.h>		// for g_dbus_*
#include <assert.h>

using namespace std;

bool print_errors( GError *e )
{
	if ( e != nullptr )
	{
		printf("Error: %s\n",e->message);
		g_error_free(e);
		return 1;
	}
	else
		return 0;
}

GDBusConnection* get_pulseaudio_bus()
{
	GError *error = nullptr;
	const char* pulse_server_string = nullptr;
	GDBusConnection *answer;

	// Try environment variable
	pulse_server_string = getenv("PULSE_DBUS_SERVER");

	if ( pulse_server_string == nullptr )
	{
		// Otherwise, try using DBus
		GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
		print_errors(error);

		GDBusProxy *proxy = g_dbus_proxy_new_sync( connection,
		                                           G_DBUS_PROXY_FLAGS_NONE,
		                                           nullptr,  // DBus interface
		                                           "org.PulseAudio1",   // Name
		                                           "/org/pulseaudio/server_lookup1",  // path
		                                           "org.PulseAudio1.ServerLookup1",   // interface
		                                           nullptr,
		                                           &error );
		print_errors(error);

		GVariant* gvp = g_dbus_proxy_get_cached_property( proxy, "Address" );
		pulse_server_string = g_variant_get_string(gvp, nullptr);

		g_dbus_connection_close_sync(connection, nullptr, &error );
		print_errors(error);
	}

	if ( pulse_server_string == nullptr )
	{
		g_printerr("Unable to find PulseAudio bus name\n");
		exit(1);
	}

	printf("PulseAudio bus: %s\n", pulse_server_string);

	// Connect to the bus
	answer = g_dbus_connection_new_for_address_sync( pulse_server_string,  // Address
	                                                 G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
	                                                 nullptr,  // GDBusAuthObserver
	                                                 nullptr,  // GCancellable
	                                                 &error );
	print_errors(error);

	return answer;
}

GVariant* get_things_gv( GDBusConnection *conn, const char *get_method_name, const char * interface, const char * path )
{
	GVariant *temp_tva, *temp_va, *temp_a;
	GDBusProxy *proxy;
	GError *error = nullptr;

	// Interface Proxy
	//   Note: we are not connected to a bus, but a direct peer-to-peer
	// connection, so the bus name is NULL
	proxy = g_dbus_proxy_new_sync(
	        conn,
	        G_DBUS_PROXY_FLAGS_NONE,
	        nullptr,                              // Interface info struct (opt.)
	        nullptr,                              // Bus Name
	        path,           // Path of object
	        "org.freedesktop.DBus.Properties", // Interface
	        nullptr,                              // GCancellable
	        &error );
	print_errors(error);

	// Array of paths
	// temp_gv is a tuple, of a single variant, of an array, of object paths
	//   Note that for some filthy reason, you are able to use a floating
	// GVariant for the parameters.
	temp_tva = g_dbus_proxy_call_sync(
	          proxy,
	          "Get",                  // Method name
	          g_variant_new("(ss)",interface,get_method_name), // Params
	          G_DBUS_CALL_FLAGS_NONE,
	          -1,                     // Timeout
	          nullptr,                   // Cancellable
	          &error );
	print_errors(error);
	g_object_unref(proxy);

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
	GDBusProxy *proxy;
	GError *error = nullptr;

	// Interface Proxy
	//   Note: we are not connected to a bus, but a direct peer-to-peer
	// connection, so the bus name is NULL
	proxy = g_dbus_proxy_new_sync(
	        conn,
	        G_DBUS_PROXY_FLAGS_NONE,
	        nullptr,                              // Interface info struct (opt.)
	        nullptr,                              // Bus Name
	        path,           // Path of object
	        "org.freedesktop.DBus.Properties", // Interface
	        nullptr,                              // GCancellable
	        &error );
	print_errors(error);

	// Array of paths
	// temp_gv is a tuple, of a single variant, of an array, of object paths
	//   Note that for some filthy reason, you are able to use a floating
	// GVariant for the parameters.
	temp_tva = g_dbus_proxy_call_sync(
	          proxy,
	          "Set",                  // Method name
	          g_variant_new("(ssv)",interface,get_method_name,input), // Params
	          G_DBUS_CALL_FLAGS_NONE,
	          -1,                     // Timeout
	          nullptr,                   // Cancellable
	          &error );
	print_errors(error);
	g_object_unref(proxy);

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
		answer[i] = g_variant_get_string(g_s, nullptr);
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
		g_variant_builder_add (builder, "u", u);

	// Convert the builder into an actual GVariant
	answer = g_variant_new ("au", builder);

	// Clean up
	g_variant_builder_unref (builder);

	return answer;
}

vector<string> get_clients( GDBusConnection *conn )
{
	GVariant * gv = get_things_gv( conn, "Clients", "org.PulseAudio.Core1", "/org/pulseaudio/core1" );
	return gv_to_vs(gv);
}

vector<string> get_sinks( GDBusConnection *conn )
{
	GVariant * gv = get_things_gv( conn, "Sinks", "org.PulseAudio.Core1", "/org/pulseaudio/core1" );
	return gv_to_vs(gv);
}

vector<string> get_playback_streams( GDBusConnection *conn, const char * path )
{
	GVariant * gv = get_things_gv( conn, "PlaybackStreams", "org.PulseAudio.Core1.Client", path );
	return gv_to_vs(gv);
}

vector<uint32_t> get_volume( GDBusConnection *conn, const char * path )
{
	GVariant * gv = get_things_gv( conn, "Volume", "org.PulseAudio.Core1.Stream", path );
	return gv_to_vuint32(gv);
}

void set_volume( GDBusConnection *conn, const char * path, const vector<uint32_t> & vols )
{
	GVariant * gv = vuint32_to_gv(vols);
	set_things_gv( conn, "Volume", "org.PulseAudio.Core1.Stream", path, gv );
	// Apparently you don't have to free 'gv', as it is already freed or some shit
}

map<string,string> get_property_list( GDBusConnection* conn, const char *interface, const char *path )
{
	GVariant *gv_adsab;
	map<string,string> answer;

	// Make the DBus call to get the data
	gv_adsab = get_things_gv(conn, "PropertyList", interface, path);

	for ( size_t j = 0; j < g_variant_n_children(gv_adsab); j++ )
	{
		GVariant *property, *key_gv, *data_gv;

		property = g_variant_get_child_value(gv_adsab,j);

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

		key  = g_variant_get_string(key_gv, nullptr);

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
