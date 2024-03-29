#!/usr/bin/env python3

# Copyright (c) 2017 Joseph Bisch
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import os
import random
import socket
import string
import sys
import time
from _thread import *

MY_NICK = 'maxxe'
SERVER_IP = '127.0.0.1'
channels = ['#libera', '#linux', '#swirc', '#test']

def get_random_ascii(length):
    return ''.join(random.choices(string.ascii_uppercase + string.digits,
        k=length))

HOST = ''
PORT = 6667

log = open('ircfuzz.log', 'wb')

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.setblocking(0)
s.settimeout(2.0)
print('Socket created')

try:
    s.bind((HOST, PORT))
except socket.error as msg:
    print('Bind failed. Error: ' + str(msg))
    sys.exit()

print('Socket bind complete')

s.listen(10)
print('Socket now listening')

param_cnt = {
    '001': 0, '002': 0, '003': 0, '004': 0, '005': 0,
    '301': 1, '302': 0, '303': 0, '305': 0, '306': 0,
    '311': 0, '312': 0,
    '328': random.choice([2, 3]),
    '332': random.choice([2, 3]),
    '333': random.choice([3, 4]),
    '353': random.choice([4, 5, 6, 7, 8, 9, 10]),
    '366': 3,

    '042': random.choice([1, 2, 3]),
    '252': random.choice([1, 2, 3]),
    '253': random.choice([1, 2, 3]),
    '254': random.choice([1, 2, 3]),
    '396': random.choice([1, 2, 3]),

    '221': random.choice([1, 2]),
    '265': random.choice([1, 2, 3, 4]),
    '266': random.choice([1, 2, 3, 4]),
    '324': random.choice([2, 3, 4]),
    '329': random.choice([2, 3]),
    '433': random.choice([2, 3]),
    '470': random.choice([2, 3, 4]),

    'ACCOUNT': 1,
    'AUTHENTICATE': 1,
    'AWAY': random.choice([0, 1]),
    'CAP': 3,
    'INVITE': 2,
    'JOIN': random.choice([1, 2]),
    'KICK': random.choice([2, 3]),
    'MODE': random.choice([2, 3]),
    'NICK': random.choice([1, 2]),
    'NOTICE': 2,
    'PART': random.choice([1, 2]),
    'PING': random.choice([1, 2]),
    'PONG': random.choice([1, 2]),
    'PRIVMSG': 2,
    'QUIT': random.choice([0, 1]),
    'TOPIC': random.choice([1, 2]),
    'WALLOPS': 1,
    'XXX': 2
}

def choose_nick():
    return choose_params('WALLOPS')[0]

def choose_command():
    bytes_array = [bytes(i).zfill(3) for i in range(1, 395)]
    irc_cmds = [
        '001', '002', '003', '004', '005',
        '301', '302', '303', '305', '306',
        '311', '312',
        '328',
        '332',
        '333',
        '353',
        '366',

        # events/misc.cpp
        # event_allaround_extract_remove_colon()
        '042',
        '252',
        '253',
        '254',
        '396',

        # events/misc.cpp
        '221',
        '265',
        '266',
        '324',
        '329',
        '433',
        '470',

        # events/whois.cpp
        # TODO: Add param count?
        '275', '671',
        '276',
        '307',
        '313',
        '317',
        '319',
        '330',
        '335',
        '338', '616',
        '378',
        '379', '310', '615',

        'ACCOUNT',
        'AUTHENTICATE',
        'AWAY',
        'CAP',
        'INVITE',
        'JOIN',
        'KICK',
        'MODE',
        'NICK',
        'NOTICE',
        'PART',
        'PING',
        'PONG',
        'PRIVMSG',
        'QUIT',
        'TOPIC',
        'WALLOPS',
        'XXX'
    ]
    return random.choice(bytes_array + irc_cmds * 100)

def choose_submsg():
    submsgs = [
        'CDCC LIST',
        'CDCC SEND',
        'CDCC XMIT',
        'CLIENTINFO',
        'DCC CHAT',
        'DCC OFFER',
        'DCC SEND',
        'DCC XMIT',
        'FINGER',
        'PING',
        'TIME',
        'USERINFO',
        'VERSION',
        'XDCC LIST',
        'XDCC SEND'
    ]
    return random.choice(submsgs)

def choose_params(cmd):
    params = []
    if cmd not in param_cnt:
        param_cnt[cmd] = 0
    for _ in range(random.randint(param_cnt[cmd], param_cnt[cmd] +
            random.randint(0, 4)) + 1):
        elements = [
            '' + get_random_ascii(random.randint(0, 2)),
            '' + get_random_ascii(random.randint(0, 2)),
            '#a',
            '',
            '#c',
            '#d',
            MY_NICK,
            'p' * (random.randint(0, 2)),
            '' + get_random_ascii(random.randint(0, 5)),
            '' + get_random_ascii(random.randint(0, 5)),
            '' + get_random_ascii(random.randint(0, 3)),
            '' + get_random_ascii(random.randint(0, 3)),
            '' + get_random_ascii(random.randint(0, 1)) *
            (2 ** random.randint(0, 6) + random.randint(-1000, 1000)),
            'http://' + get_random_ascii(random.randint(0, 2)) + '.com/' +
            get_random_ascii(random.randint(0, 2))
        ]
        params.append(random.choice(elements))
    if params and random.choice([True, False]):
        params[-1] = ':' + params[-1]
    for i, param in enumerate(params):
        if random.randint(0, 20) > 16:
            if len(param) > 10:
                pos = random.randint(len(param) - 9, len(param) - 1)
                params[i] = params[i][:pos] + '\t' + params[i][pos + 1:]
    return params

def fuzz():
    prefix = ''
    if random.choice([True, True, True, False]):
        prefix = ':%s ' % choose_nick()
    # prefix = ':127.0.0.1 '
    command = choose_command()
    params = ' '.join(choose_params(command))
    for _ in range(random.randint(0, 5)):
        if channels:
            channel = random.choice(channels)
            channels.remove(channel)
            sendall(':%s PART %s\r\n' % (MY_NICK, channel))
    if random.choice([0, 1, 2, 3, 4, 5, 6]) >= 4:
        return ('%s%s %s :\x01%s %s%s%s' %
            (prefix,
             'PRIVMSG',
             choose_nick(),
             choose_submsg(),
             params[:100] if len(params) > 100 else params,
             random.choice(['\x01', '']),
             random.choice(['\r\n', '\n', ''])))
    return ('%s%s %s %s%s' %
        (prefix,
         command,
         choose_nick(),
         params[:100] if len(params) > 100 else params,
         random.choice(['\r\n', '\n', ''])))

def send(to_send):
    log.write(to_send.encode('utf-8'))
    conn.send(to_send.encode('utf-8'))

def sendall(to_send):
    log.write(to_send.encode('utf-8'))
    conn.sendall(to_send.encode('utf-8'))

def clientthread(conn):
    conn.recv(1024)

    send(':%s 001 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 002 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 003 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 004 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 005 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 251 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 252 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 253 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 254 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 255 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 375 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 372 %s :a\r\n' % (SERVER_IP, MY_NICK))
    send(':%s 376 %s :a\r\n' % (SERVER_IP, MY_NICK))

    while True:
        try:
            time.sleep(0.02)
            sendall(fuzz())
        except socket.error as e:
            break
        except Exception as e:
            print('Exception: ' + str(e))

    conn.close()
    print('Socket broken')

while True:
    try:
        conn, addr = s.accept()
    except socket.timeout:
        continue

    start_new_thread(clientthread, (conn,))

s.close()
