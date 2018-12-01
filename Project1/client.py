import socket
import sys
import os

def print_usage():
    print("Usage: " + sys.argv[0] + " host port GET/PUT file")
    print("Example: " + sys.argv[0] + " www.google.com 80 GET index.html")


# Parse and validate command line args
if len(sys.argv) != 5:
    print("Expected 4 arguments but found {}".format(len(sys.argv) - 1))
    print_usage()
    exit()

host = sys.argv[1]
port = sys.argv[2]
method = sys.argv[3]
if method not in ['GET', 'PUT']:
    print("Invalid method " + method)
    print_usage()
    exit()

filename = sys.argv[4]

# The data received from the server. The data from each call to `recv` will be stored in a separate list item
data = []

# Use create_connection to create and connect the socket at once.
# `create_connection` uses AF_INET (IPV4) or AF_INET6 (IPV6) as needed, based on address.
# It creates a SOCK_STREAM (TCP) socket by default, per https://github.com/python/cpython/blob/master/Modules/socketmodule.c
# It seems to automatically convert host to network byte order, although I haven't confirmed in source code and couldn't
# find any references to automatic conversion in the docs. The examples from python.org (and elsewhere) don't mention
# using `htons`.
# I use `with` so that socket will automatically be closed.
with socket.create_connection((host, port)) as s:
    # Use UTF-8 when encoding, which includes a Byte Order Mark (BOM), so we don't have to worry about endianess
    s.sendall('{method} /{filename} HTTP/1.0\r\n'.format(method=method, filename=filename).encode('UTF-8'))
    if method == 'PUT':
        # Send a Content-Length header to indicate the size of the file we plan to send
        s.sendall('Content-Length: {} \r\n'.format(os.path.getsize(filename)).encode('UTF-8'))
    # Must send a blank line `\r\n\` to signal end of headers
    s.sendall('\r\n'.encode('UTF-8'))
    if method == 'PUT':
        # Assume filename points to a valid file
        # Use `rb` to open the file in readonly mode with binary encoding
        with open(filename, 'rb') as f:
            s.sendfile(f)
    # In Python 3.5 and later, this call will be retried automatically upon being interrupted by a signal
    while True:
        buffer = s.recv(1024)
        if not buffer:
            break
        data.append(buffer)

# Use `map` to call `repr` on each item in data, to convert them to strings, then `join `them together into one string
print('Received', ''.join(map(repr, data)))
