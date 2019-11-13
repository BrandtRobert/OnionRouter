import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('localhost', 8888))
s.listen(5)

while True:
    client, _ = s.accept()
    msg = client.recv(256).decode('utf-8')
    print(msg)
    client.close()
