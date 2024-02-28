#!/bin/python3

import socket
import time
import pathlib
import sys

def read_byte_size(sock) :
    text = ""
    while True :
        c = sock.recv(1)
        if c == 0 or len(c) == 0:
            return -1
        c = c.decode('ascii')
        if c[0] == ' ':
            print(f'Record size: {text}')
            return int(text)
        else :
            text += str(c)
    return -1

def listen_for_messages(mode, protocol, host, port):
    if mode == 'internet' :
        sock = socket.socket(socket.AF_INET, protocol)
        sock.bind((host, port))
        if protocol == socket.SOCK_STREAM:
            sock.listen(5)
            while True :
                clientsock, address = sock.accept()
                print(f"TCP connection from {address}")
                while True :
                    record_size = read_byte_size(clientsock)
                    if (record_size == -1) :
                        print('Connection closed')
                        break
                    else :
                        data = clientsock.recv(record_size)
                        data = data.decode('ascii')
                        print(f"message: {data}")
        else :
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind((host, port))
            while True :
                data,addr = sock.recvfrom(65535)
                data = data.decode('ascii')
                print(f"message: {data}")
    elif mode == 'unix':
        pathlib.Path.unlink(pathlib.PosixPath(host), missing_ok=True)
        sock = socket.socket(socket.AF_UNIX, protocol)
        sock.bind(host)
        while True :
            data,addr = sock.recvfrom(65535)
            data = data.decode('ascii')
            print(f"message: {data}")

def main() :
    host = "/tmp/slog"    
    mode = 'unix'
    port = 50514
    protocol = 'udp'
    if len(sys.argv) >= 2:
        host = sys.argv[1]
    colonIndex = host.find(':')
    if colonIndex == -1:
        mode = 'unix'
    else:
        mode = 'internet'
        port = int(host[colonIndex+1:])
        host = host[:colonIndex]
    if len(sys.argv) == 3 and sys.argv[2] == 'tcp':
        protocol = 'tcp'    
    if (mode == 'internet') :
        print(f'Bind using {mode} sockets to {host}:{port} with protocol {protocol}')
    else:
        print(f'Bind using {mode} sockets to {host} with protocol {protocol}')
    if protocol == 'udp':
        protocol = socket.SOCK_DGRAM
    else:
        protocol = socket.SOCK_STREAM
    listen_for_messages(mode, protocol, host, port)


if __name__ == '__main__':
    main()