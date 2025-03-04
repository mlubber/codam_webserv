import socket

# Server settings (change if needed)
HOST = "localhost"
PORT = 8080

# Constructing a valid HTTP/1.1 chunked request
request = (
    "POST /chunkmit HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Transfer-Encoding: chunked\r\n"
	# "Content-Length: 46\r\n"
    "Content-Type: text/plain\r\n"
	"\r\n"
    "6\r\nHello \r\n"
    "E\r\nGreat Chunked \r\n"
    "6\r\nworld!\r\n"
    "0\r\n\r\n"
)

# Create a socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Connect to the server
    sock.connect((HOST, PORT))
    print(f"Connected to {HOST}:{PORT}")

    # Send the request
    sock.sendall(request.encode())
    print("Chunked request sent.")

    # Receive the response
    response = sock.recv(4096)
    print("\n--- Server Response ---")
    print(response.decode())

finally:
    sock.close()
    print("Connection closed.")