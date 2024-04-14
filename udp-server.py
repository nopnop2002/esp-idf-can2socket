#!/usr/bin/env python3
#-*- encoding: utf-8 -*-
import sys
import select, socket
import time
import signal
import argparse

def handler(signal, frame):
	print('handler')
	sys.exit()

if __name__=='__main__':
	signal.signal(signal.SIGINT, handler)
	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='udp port', default=8080)
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	#s.bind(('<broadcast>', args.port)) # Limited broadcast address
	s.bind(('0.0.0.0', args.port)) # Any address
	s.setblocking(0)

	while True:
		result = select.select([s],[],[])
		msg = result[0][0].recv(1024)
		if (type(msg) is bytes):
			msg=msg.decode('utf-8')
		print(msg)
