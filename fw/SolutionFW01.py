import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# Let the source address be 192.168.1.21:1234
s.bind(("0.0.0.0", 443))
s.connect(("fw.ctf.hf", 8888))
s.sendall('Hello, world')
data = s.recv(1024)
print data

