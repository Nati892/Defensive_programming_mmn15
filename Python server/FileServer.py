import os
import socket
import threading
from Messages import *
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

#main server class, holds the main server logic
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
            Server.StartConversation(socket)

    def StartConversation(client_socket):
        print("handler started")
        Context = ClientContext(client_socket)
        Context.soc=client_socket
        IsClientRegistered=HandleClientRegistration(Context)

#holds context for conversation with client
class ClientContext:
        soc:socket
        Authenticated:bool = False
        BufferedData = b""

        def __init__(self,soc:socket):
            self.soc=soc

def HandleClientRegistration(context:ClientContext):
    registered:bool=False
    Messsage=RecieveMesssage(context)

    return registered

# recieve message in context
def RecieveMesssage(context:ClientContext):
    
    #recieve 23 first bytes to build message header
    while len(context.BufferedData)<CLIENT_MESSAGE_HEADER_SIZE:
        (data_rec, data_rec_len) = ReceiveData(context.soc)
        if data_rec_len==0:
            return None
            
        context.BufferedData = context.BufferedData + data_rec
    
    #build message header
    Request:ClientMessage=ClientMessage(context.BufferedData[:CLIENT_MESSAGE_HEADER_SIZE])
    context.BufferedData=context.BufferedData[CLIENT_MESSAGE_HEADER_SIZE:]
    
    #handle payload
    while Request.PayloadSize>len(context.BufferedData):
        (data_rec, data_rec_len) = ReceiveData(context.soc)
        if data_rec_len==0:
            return None
        
        context.BufferedData = context.BufferedData + data_rec

    ClientMessage.Payload=context.BufferedData[ClientMessage.PayloadSize:]
    context.BufferedData[:ClientMessage.Payload]
        

#data recieve and send
def ReceiveData(soc):
    with soc:
        try:
            data = soc.recv(1024)
            if not data:
                print("Connection closed by the client. Goodbye!")#debug
                return (b"",0)
            else:
                if len(data)>0:
                    return (data, len(data))
                    print("debug: got data:"+str(len(data)))#debug
                    Utils.print_dataArr(data)#debug
        except Exception as e:
            print(f"Error occurred while handling client: {e}")
            return (b"",0)


def SendData(soc,data):
    with soc:
        try:
            if not data or len(data)==0:
                return False                    
            if not isinstance(data,bytes):
                EncodedData= data.encode()
            else:
                EncodedData=data
            soc.send(EncodedData)
            return True
        except Exception as e:
            print(f"Error occurred while handling client: {e}")
        finally:
            print(f"Connection closed. Stay cool!")
            return False