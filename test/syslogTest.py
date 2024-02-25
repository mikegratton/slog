#!/bin/python3

import socket
import time
import pathlib

UNIX_PATH = "/tmp/slog"
IP = "localhost"
PORT = 50514
mode = 'unix'

pathlib.Path.unlink(pathlib.PosixPath(UNIX_PATH), missing_ok=True)

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

if mode == 'tcp' :
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((IP, PORT))
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
elif mode == 'unix':
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(UNIX_PATH)
    while True :
        data,addr = sock.recvfrom(65535)
        data = data.decode('ascii')
        print(f"message: {data}")
else :
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((IP, PORT))
    while True :
        data,addr = sock.recvfrom(65535)
        data = data.decode('ascii')
        print(f"message: {data}")