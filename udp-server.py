#!/usr/bin/env python3
#-*- encoding: utf-8 -*-
import argparse
import select, socket
import signal

def handler(signal, frame):
	global running
	print('handler')
	running = False

if __name__=='__main__':
	signal.signal(signal.SIGINT, handler)
	running = True

	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='udp port', default=8080)
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	udp_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	#udp_server.bind(('<broadcast>', args.port)) # Limited broadcast address
	udp_server.bind(('0.0.0.0', args.port)) # Any address
	udp_server.setblocking(0)

	while running:
		result = select.select([udp_server],[],[])
		msg = result[0][0].recv(1024)
		if (type(msg) is bytes):
			msg=msg.decode('utf-8')
		print("{}".format(msg))

	#print("socket close")
	udp_server.close()
