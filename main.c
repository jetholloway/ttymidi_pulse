#include <stdlib.h>          // for exit()
#include <stdio.h>
#include <string.h>
#include <gio/gio.h>		// for g_dbus_*

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

	printf("Pulseaudio bus: %s\n", pulse_server_string);

	// Connect to the bus
	answer = g_dbus_connection_new_for_address_sync( pulse_server_string,  // Address
	                                                 G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
	                                                 NULL,  // GDBusAuthObserver
	                                                 NULL,  // GCancellable
	                                                 &error );
	print_errors(error);


	return answer;
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
		g_print("  %s\n", g_variant_get_string(g_variant_get_child_value(array_of_strings, count), NULL) );
	}

	g_variant_unref(array_of_strings);
	g_variant_unref(gvp);
	g_object_unref(proxy);
	g_dbus_connection_close_sync(connection, NULL, &error );
	print_errors(error);

	return 0;
}
