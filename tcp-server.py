#!/usr/bin/env python3
#-*- encoding: utf-8 -*-
import argparse
import socket
import select
import signal

def handler(signal, frame):
	global running
	print('handler')
	running = False

server_ip = "0.0.0.0"
if __name__=='__main__':
	signal.signal(signal.SIGINT, handler)
	running = True

	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='tcp port', default=8080)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	listen_num = 5
	tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	tcp_server.bind((server_ip, args.port))
	tcp_server.listen(listen_num)

	read_list = [tcp_server]
	while running:
		readable, writable, errored = select.select(read_list, [], [], 1)
		#print("readable={}".format(readable))
		for sock in readable:
			if sock is tcp_server:
				print("start accept")
				tcp_client,address = tcp_server.accept()
				print("Connected!! [ Source : {}]".format(address))
				read_list.append(tcp_client)
			elif sock is tcp_client:
				msg = tcp_client.recv(1024)
				if (type(msg) is bytes):
					msg = msg.decode('utf-8')
				print("{}".format(msg))

	tcp_server.close()
