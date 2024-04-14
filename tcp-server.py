#!/usr/bin/env python3
#-*- encoding: utf-8 -*-
import socket
import argparse

server_ip = "0.0.0.0"
if __name__=='__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='tcp port', default=8080)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	listen_num = 5
	tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	tcp_server.bind((server_ip, args.port))
	tcp_server.listen(listen_num)

	client,address = tcp_server.accept()
	print("[*] Connected!! [ Source : {}]".format(address))
	#client.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

	buffer_size = 1024
	while True:
		#client,address = tcp_server.accept()
		#print("[*] Connected!! [ Source : {}]".format(address))
		data = client.recv(buffer_size)
		if (type(data) is bytes):
			data = data.decode('utf-8')
		print("[*] Received Data : {}".format(data))

		#client.send(b"ACK!!")

	client.close()
