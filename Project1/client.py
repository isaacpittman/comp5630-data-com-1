import socket
import sys

# Validate command line args
if len(sys.argv) != 5:
    print("Usage: " + sys.argv[0] + " host port GET/PUT file")
    print("Example: " + sys.argv[0] + " www.google.com 80 GET index.html")
    exit(0)

# Parse the command line arguments
host = sys.argv[1]
port = sys.argv[2]
method = sys.argv[3]
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
with socket.create_connection((host, port)) as conn:
    # Must send a blank line `\r\n\` to signal end of request
    # Use `ascii` encoding when converting string to bytes, because HTTP headers only support ASCII, per
    #  https://stackoverflow.com/questions/4400678/what-character-encoding-should-i-use-for-a-http-header
    conn.sendall('{method} /{filename} HTTP/1.0\r\n\r\n'.format(method=method, filename=filename).encode('ascii'))
    # In Python 3.5 and later, this call will be retried automatically upon being interrupted by a signal
    while True:
        buffer = conn.recv(1024)
        if not buffer:
            break
        data.append(buffer)

# Use `map` to call `repr` on each item in data, to convert them to strings, then `join `them together into one string
print('Received', ''.join(map(repr, data)))
