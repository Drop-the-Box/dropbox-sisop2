import socket

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 6999  # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    while True:
        message = input("Send something to the server: ")
        s.sendall(message.encode('utf-8'))
