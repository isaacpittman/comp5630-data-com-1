import selectors
import socket
import sys
import os
import re


def print_usage():
    print("Usage: " + sys.argv[0] + " port")
    print("Example: " + sys.argv[0] + " 8989")


# Parse and validate command line args
if len(sys.argv) != 2:
    print("Expected 1 argument but found {}".format(len(sys.argv) - 1))
    print_usage()
    exit()

try:
    port = int(sys.argv[1])
except ValueError:
    print("Expected an integer for port, but found {}".format(sys.argv[1]))
    print_usage()
    exit()

# This selector will be used to detect when the listening socket has a connection ready to accept, and when an accepted
#   socket has data to read.
# Use DefaultSelector so Python can choose the most efficient selector for the current operating system.
sel = selectors.DefaultSelector()

# Key is an established socket with a client. Value is a string buffer containing all the data that has been read from
#   the client during this session. Each time read() is called, the data read from that connection is appended here. See
#   the read() function for more info.
data_read = {}

# Key is an established socket with a client. Value is a file descriptor that is waiting to be written to the client.
#   This is set by read() when a client makes a GET request, and is written by write() when the socket is ready for
#   writing. See read() and write() for more info.
data_write = {}


# When the listening socket receive a connection request, the registered selector will call this method, which will
#   accept the connection then register another listener to handle incoming data once the connection is established
def accept(sock, mask):
    conn, addr = sock.accept()
    # Set the connection to non-blocking mode so it can be used with select
    # (see https://docs.python.org/3.6/library/socket.html#notes-on-socket-timeouts)
    conn.setblocking(False)
    # When this new connection has data ready to read or write, the selector will call the handle_connection() method
    sel.register(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, handle_connection)


# Parse the selector mask to see whether the socket is ready to read or to write and call the appropriate method
def handle_connection(conn, mask):
    if mask & selectors.EVENT_READ:
        read(conn)
    elif mask & selectors.EVENT_WRITE:
        write(conn)
    else:
        raise Exception("Shouldn't be able to get here")


# When an established connection is ready to be written to (that is, when the socket is in a writable state),
#   handle_connection will call this method, which writes to the connection the file data that we've previously queued
#   for writing. It assumes the the value of data_write[conn] is a file descriptor opened for reading in binary mode.
def write(conn):
    # If we have data to be written, write it and close the connection
    if conn in data_write:
        # Read from the file and write it to the connection
        conn.sendall(data_write[conn].read())
        # Done writing, so close the file, delete the entry so we don't try to write it again, and close the connection
        # TODO: Should probably encapsulate this cleanup code into a method, rather than copy pasting it everywhere
        data_write[conn].close()
        del data_write[conn]
        del data_read[conn]
        sel.unregister(conn)
        conn.close()


# When an established connection has data that's ready to be read, the selector will call this method, which reads
#   the data and acts on it.
# Specifically:
#   For GET, if the server receives the "GET index.html HTTP/1.0" request, it sends out "200 OK" to the client,
#   followed by the file index.html. If the requested file doesn't exist, the server sends out "404 Not Found"
#   response to the client.
#
#   For PUT, if the server receives the "PUT test.txt" request, it will save the file as test.txt. If the received file
#   from client is successfully created, the server sends back a "200 OK File Created" response to the client.
def read(conn):
    # Read up to the first 4096 bytes from the socket.
    # TODO: We should probably handle endianess here
    read_buffer = conn.recv(4096)

    # Get one buffer's worth of data from the socket and append it to the data_read buffer for this connection.
    #   read() will be called again by select, if there's data remaining to be read from the socket. Each call to
    #   read() will append more data to the data_read buffer, until we've received it all.
    if read_buffer:
        # read_buffer didn't return the empty string, which indicates that the client connection is still open.

        if conn in data_read:
            # This is not the first data we've read from the socket for this connection, so append it to what we've
            #   already read.
            data_read[conn] += read_buffer
        else:
            # Otherwise, create a new data_read buffer for this connection containing the data we just read
            data_read[conn] = read_buffer

        # The client sends a blank line to indicate the end of headers and the beginning of data. Let's parse it now.
        data_read_parsed = data_read[conn].split('\r\n\r\n'.encode('UTF-8'))

        if data_read_parsed != -1:
            # We the newline in the data, so we must have received all the headers.

            # Extract the headers from the parsed data.
            headers = data_read_parsed[0].decode('UTF-8')

            # This will match a GET request for HTTP 1.0 or 1.1 and capture the requested file path in a capture group
            get_request_regex = re.compile(r'GET /(.+) HTTP/1\.[01]')
            get_request_match = get_request_regex.match(headers)

            # This will match a PUT request for HTTP 1.0 or 1.1 and capture the requested file path in a capture group
            put_request_regex = re.compile(r'PUT /(.+) HTTP/1\.[01]')
            put_request_match = put_request_regex.match(headers)

            if get_request_match:
                # Handle the get request
                try:
                    # This will thrown FileNotFoundError if the file doesn't exist
                    # TODO: Assume the file specified is a valid filename. This could be improved.
                    os.path.getsize(get_request_match.group(1))
                    conn.sendall('HTTP/1.0 200 OK\r\n\r\n'.encode('UTF-8'))
                    # Open the file for read only in binary mode
                    # TODO: We should probably use htons here, or read the file as text and encode it
                    data_write[conn] = open(get_request_match.group(1), 'rb')
                    # Don't close the connection. write() will close it after sending the file.
                except FileNotFoundError:
                    conn.sendall('HTTP/1.0 404 Not Found\r\n\r\n'.encode('UTF-8'))
                    # In HTTP 1.0, the server is responsible for closing the connection after sending the response
                    del data_read[conn]
                    sel.unregister(conn)
                    conn.close()
            elif put_request_match:
                # TODO: We probably don't need to parse this every time. We could just parse it once then skip until
                # TODO:     the data_read buffer has reached the appropriate number of bytes.
                content_length_regex = re.compile(r'Content-Length: (\d+)', re.MULTILINE)
                content_length_match = content_length_regex.search(headers)
                if content_length_match:
                    # We found a Content-Length header, so grab the number of bytes
                    content_length = content_length_match.group(1)

                    # We've already received the content length header, and we're waiting for all the data from
                    #   the client to be read.
                    if len(data_read_parsed) > 1:
                        # We have received all the headers and some data
                        request_data = data_read_parsed[1]
                        if len(request_data) == int(content_length):
                            # We've  received all the data from the client. Go ahead and write it locally
                            #   and send our response.
                            # TODO: Assume the file path specified is valid for writing. Could be improved.
                            with open(put_request_match.group(1), 'wb') as f:
                                f.write(request_data)
                            # TODO: Send correct header
                            conn.sendall('HTTP/1.0 200 OK File Created\r\n\r\n'.encode('UTF-8'))
                            # We're done handling the PUT. Close the connection and cleanup.
                            del data_read[conn]
                            sel.unregister(conn)
                            conn.close()
    else:
        # If select notified us, but recv is empty, then the client must have closed the connection
        print('Warning: Client closed connection', conn)
        # Delete the server's buffer for this client
        if conn in data_read:
            del data_read[conn]
        sel.unregister(conn)
        conn.close()


try:
    with socket.socket() as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('localhost', port))
        s.listen()
        s.setblocking(False)

        # When the socket receives a connection request from the client, call the accept() method
        sel.register(s, selectors.EVENT_READ, accept)

        try:
            while True:
                events = sel.select()
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj, mask)
        except KeyboardInterrupt:
            print("Received KeyboardInterrupt. Exiting.")
            for conn in data_read.keys():
                conn.close()
        finally:
            sel.close()
except OSError as e:
    print("Error configuring the socket.")
    print(e)