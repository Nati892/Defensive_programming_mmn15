import os
import socket
import threading
import Globals
import Utils
#Global constants
LOCAL_CONFIG_PATH = "port.info"
DEFAULT_PORT = "1357"
HOST="127.0.0.1"

#Config files
def CreateDefaultConfigFile():
    print("Creating new config file")
    if os.path.exists(LOCAL_CONFIG_PATH):
        os.remove(LOCAL_CONFIG_PATH)

    file_handler=open(LOCAL_CONFIG_PATH,"w")
    file_handler.write(DEFAULT_PORT+"\n")
    file_handler.close()

#This codes doesnt really need to look this way, but its strict and scalable
class Config:
    #privates
    ServerPort:int=0


    def FetchLocalConfig(self):
        ConfFile=None
        try:
            ConfFile=open(LOCAL_CONFIG_PATH,"r")
            self.ServerPort=int(ConfFile.readline())
        except:#rewriting config file with defaults
            if ConfFile is not None and ConfFile.closed==False: 
                ConfFile.close()
            CreateDefaultConfigFile()
            ConfFile=open(LOCAL_CONFIG_PATH,"r")
            self.ServerPort=int(ConfFile.readline())

class Server:
    port:int
    def __init__(self,config:Config):
        self.port=config.ServerPort
    def Run(self):
        print("Server starting at port: "+ str(self.port))
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind((HOST, self.port))
        server_socket.listen()
        try:
            while True:
                print("debug: accept\n")
                clientSoc,addr= server_socket.accept()
                #offload the client to different threads, freeing the main thread to keep accepting new conections
                clientThread = threading.Thread(target=Server.ClientHandler,args=(clientSoc,));
                clientThread.start()
        except Exception as e:
            print("\nServer shutting down. Goodbye! " + e.args)
        finally:
            server_socket.close()

#client handling
    def ClientHandler(socket):
        with socket:
            Server.HandleClient(socket)

    def HandleClient(client_socket):
        print("handler started")
        with client_socket:
            try:
                while True:
                    data = client_socket.recv(2048)
                    if not data:
                        print("Connection closed by the client. Goodbye!")#debug
                        break
                    else:
                        print("debug: got data:"+str(len(data)))#debug
                        Utils.print_dataArr(data)
              ##          client_socket.send(("got"+str(len(data))).encode())
            except Exception as e:
                print(f"Error occurred while handling client: {e}")
            finally:
                print(f"Connection closed. Stay cool!")
