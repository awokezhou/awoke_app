import os
import socket     
import time     
import sys
import struct 
import threading

server_socket_path = "/home/share/awoke_app/application/unix_sock_with_python/unix_sock_with_python"

def client_run(msgdata):
    #msgdata = "hypnos stop"
    #msgdata = "hypnos wakeup"
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)     
    sock.connect(server_socket_path)     
    #time.sleep(0.1)     
    sock.send(msgdata)     
    #print sock.recv(1024)     
    sock.close()  

unix_sock = {
    "hypnosStart":"hypnos start",
    "tboxsendFinish":"tboxsend finish",
}

def create_unix_socket_server(sock_addr):
    sock = None
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        if os.path.exists(sock_addr):
            os.unlink(sock_addr)
        sock.bind(sock_addr)
        sock.listen(5)
    except Exception as e:
        print(e)
    return sock

def server_run():
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(server_socket_path)
    sock.listen(5)
    while True:
        connection,address = sock.accept()
        msg_recv = connection.recv(1024)
        for i in msg_recv:
            print('%#x'%ord(i))

def hypnos_wakeup_process(connection, address):
    print("hypnos_wakeup_process")
    time.sleep(5)
    print("send data ok")
    connection.send("send data ok")
    
class UnixSockServer(threading.Thread):
    def __init__(self, addr):
        threading.Thread.__init__(self)
        self.stop_event = threading.Event()
        self.sock = create_unix_socket_server(addr)                
        self.buff_max = 1024

    def run(self):
        while not self.stop_event.is_set():
            try:
                print("run once")
                self.run_once()
            except Exception as e:
                print("Exception {}".format(e))
            finally:
                time.sleep(5)
    
    def run_once(self):
        connection, address = self.sock.accept()
        raw_message = connection.recv(self.buff_max).split("\0")[0]
        if raw_message == "hypnos wakeup":
            hypnos_wakeup_process(connection, address)

def main_loop():
    time.sleep(60)


if __name__ == '__main__':
    
    server = UnixSockServer(server_socket_path)
    #client_run(sys.argv[1])  
    server.start()
    #main_loop()

