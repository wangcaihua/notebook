import socket


def get_random_ports(num: int):
  socks, ports = [], []
  for _ in range(num):
    sock = socket.socket()
    sock.bind(('', 0))
    port = sock.getsockname()[1]
    ports.append(port)
    socks.append(sock)
  for sock in socks:
    sock.close()

  return ports
