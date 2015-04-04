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
	if ( e != NULL )
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
	GError *error = NULL;
	const char* pulse_server_string = NULL;
	GDBusConnection *answer;

	// Try environment variable
	pulse_server_string = getenv("PULSE_DBUS_SERVER");

	if ( pulse_server_string == NULL )
	{
		// Otherwise, try using DBus
		GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
		print_errors(error);

		GDBusProxy *proxy = g_dbus_proxy_new_sync( connection,
		                                           G_DBUS_PROXY_FLAGS_NONE,
		                                           NULL,  // DBus interface
		                                           "org.PulseAudio1",   // Name
		                                           "/org/pulseaudio/server_lookup1",  // path
		                                           "org.PulseAudio1.ServerLookup1",   // interface
		                                           NULL,
		                                           &error );
		print_errors(error);

		GVariant* gvp = g_dbus_proxy_get_cached_property( proxy, "Address" );
		pulse_server_string = g_variant_get_string(gvp, NULL);

		g_dbus_connection_close_sync(connection, NULL, &error );
		print_errors(error);
	}

	if ( pulse_server_string == NULL )
	{
		g_printerr("Unable to find PulseAudio bus name\n");
		exit(1);
	}

	printf("PulseAudio bus: %s\n", pulse_server_string);

	// Connect to the bus
	answer = g_dbus_connection_new_for_address_sync( pulse_server_string,  // Address
	                                                 G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
	                                                 NULL,  // GDBusAuthObserver
	                                                 NULL,  // GCancellable
	                                                 &error );
	print_errors(error);

	return answer;
}

GVariant* get_things_gv( GDBusConnection *conn, const char *get_method_name, const char * interface, const char * path )
{
	GVariant *temp_tva, *temp_va, *temp_a;
	GDBusProxy *proxy;
	GError *error = NULL;

	// Interface Proxy
	//   Note: we are not connected to a bus, but a direct peer-to-peer
	// connection, so the bus name is NULL
	proxy = g_dbus_proxy_new_sync(
	        conn,
	        G_DBUS_PROXY_FLAGS_NONE,
	        NULL,                              // Interface info struct (opt.)
	        NULL,                              // Bus Name
	        path,           // Path of object
	        "org.freedesktop.DBus.Properties", // Interface
	        NULL,                              // GCancellable
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
	          NULL,                   // Cancellable
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
	GError *error = NULL;

	// Interface Proxy
	//   Note: we are not connected to a bus, but a direct peer-to-peer
	// connection, so the bus name is NULL
	proxy = g_dbus_proxy_new_sync(
	        conn,
	        G_DBUS_PROXY_FLAGS_NONE,
	        NULL,                              // Interface info struct (opt.)
	        NULL,                              // Bus Name
	        path,           // Path of object
	        "org.freedesktop.DBus.Properties", // Interface
	        NULL,                              // GCancellable
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
	          NULL,                   // Cancellable
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

int main()
{
	GDBusConnection *connection;
	GError *error = NULL;
	GDBusProxy *proxy;

	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if ( print_errors(error) )
		exit(1);

	// Create a proxy object for the "bus driver" (name "org.freedesktop.DBus")
	proxy = g_dbus_proxy_new_sync( connection,
	                               G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                               NULL,                     // DBus interface
	                               "org.freedesktop.DBus",   // Name
	                               "/org/freedesktop/DBus",  // path
	                               "org.freedesktop.DBus",   // interface
	                               NULL,                     // GCancellable
	                               &error );
	print_errors(error);

	// Call ListNames method, wait for reply
	GVariant* gvp = g_dbus_proxy_call_sync( proxy,
	                                        "ListNames",            // method name
	                                        g_variant_new("()"),    // parameters to method
	                                        G_DBUS_CALL_FLAGS_NONE, // flags
	                                        -1,                     // timeout
	                                        NULL,                   // cancellable
	                                        &error );
	if ( print_errors(error) )
		exit(1);

	// Print the results
	GVariant * array_of_strings = g_variant_get_child_value(gvp, 0);

	g_print("Names on the message bus:\n");

	for (size_t count = 0; count < g_variant_n_children(array_of_strings); ++count)
	{
		GVariant *child = g_variant_get_child_value(array_of_strings, count);
		g_print("  %s\n", g_variant_get_string(child, NULL));
		g_variant_unref(child);
	}

	g_variant_unref(array_of_strings);
	g_variant_unref(gvp);
	g_object_unref(proxy);
	g_dbus_connection_close_sync(connection, NULL, &error );
	print_errors(error);

	return 0;
}
