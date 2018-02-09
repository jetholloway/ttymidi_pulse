#!/usr/bin/python3

import os, dbus

session_bus = dbus.SessionBus()

def get_pulseaudio_bus():
	srv_addr = os.environ.get("PULSE_DBUS_SERVER")

	if not srv_addr:
		session_bus = dbus.SessionBus()

		srv_addr = session_bus.get_object("org.PulseAudio1", "/org/pulseaudio/server_lookup1")\
			.Get("org.PulseAudio.ServerLookup1", "Address", dbus_interface="org.freedesktop.DBus.Properties")

	return dbus.connection.Connection(srv_addr)

#-------------------------------------------------------------------------------
#   Different DBus objects use different interfaces, so these functions let you
# get the properties of different object types

def get_property_list( interface, obj ):
	# Dictionary of properties ( the dictionary is a mapping dbus.String -> dbus.Array(Byte) )
	# This is a bit of a filthy format, so we will change them to a different one
	properties_temp = obj.Get(interface, "PropertyList")

	# This new dictionary is of type ( string -> unicode string )
	properties = { str(p): bytearray(properties_temp[p]).decode("utf-8") for p in properties_temp }

	return properties

def get_client_property_list( client ):
	return get_property_list("org.PulseAudio.Core1.Client", client)

def get_device_property_list( device ):
	return get_property_list("org.PulseAudio.Core1.Device", device)

def get_stream_property_list( stream ):
	return get_property_list("org.PulseAudio.Core1.Stream", stream)

#-------------------------------------------------------------------------------

# - get_method_name is the name of the method that gets the things you want
# - interface is the interface that those things use
def get_things( bus, get_method_name ):
	# Remote object (this is a dbus.proxies.ProxyObject)
	things_ro    = bus.get_object(object_path="/org/pulseaudio/core1")
	# Interface:
	things_int   = dbus.Interface(things_ro, "org.freedesktop.DBus.Properties")
	# Array of paths:
	things_paths = things_int.Get("org.PulseAudio.Core1", get_method_name)

	things = [ dbus.Interface(bus.get_object(object_path=path),
		dbus_interface="org.freedesktop.DBus.Properties") for path in things_paths ]

	return things

def get_clients( bus ):
	return get_things(bus, "Clients")

def get_sinks( bus ):
	return get_things(bus, "Sinks")

def get_streams( bus ):
	return get_things(bus, "PlaybackStreams")

#===============================================================================
# Start main program here

bus = get_pulseaudio_bus()

clients = get_clients(bus)

#   mpd_stream_paths will contain all of the DBus object paths for streams
# that belong to any mpd client.
mpd_stream_paths = []

# List all of the pulseaudio clients
for c in clients:
	properties = get_client_property_list(c)

	# Print name of client
	if "application.name" in properties:
		print(properties["application.name"])
	else:
		print("Unknown application")

	# Get length of longest key (for justification of columns)
	l = 0
	for key in properties:
		l = max(l, len(key))

	for key,value in properties.items():
		# "application.process.machine_id" (30)1
		print("\t", key.ljust(l+1), value)
	print()
