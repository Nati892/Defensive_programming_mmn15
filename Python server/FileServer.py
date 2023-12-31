import os
import socket
import threading
from Client import ClientContext
from Messages import *
import Utils
import DBWrapper
import CryptoWrapper
import cksum
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

    def ClientHandler(soc:socket):
        with soc:
            Server.StartConversation(soc)

    def StartConversation(client_socket):
        print("handler started")
        Context = ClientContext(client_socket)
        Context.soc=client_socket
        with Context.soc:
            IsClientRegistered=HandleClientRegistration(Context)
            if not IsClientRegistered:
                print("client registration failed")
                return;
            print("register/reconnect success")
            print("receiving file")
            handleReceiveFile(Context)
        print("Done with client object")
                    

#this func makes sure the client is registered either first time registration or reconnection
def HandleClientRegistration(context:ClientContext):
    registered:bool=False
    Messsage=ReceiveMesssage(context)
    if Messsage is None:
         return False
    if Messsage.code == ClientMessageType.register_request:#if client want to register
        if CanRegisterClient(Messsage.Payload):
            ClientId=RegisterClient(Messsage.Payload)
            if(ClientId==None):
                SendMessage(context,ServerMessageType.general_error,0,None)
            else:#successful registration
                context.ID=ClientId
                payload:bytes= ClientId
                SendMessage(context,ServerMessageType.register_success_response,len(payload),payload)
                
                Messsage =ReceiveMesssage(context)
                if Messsage==None:
                    return False
                if Messsage.code!=ClientMessageType.send_public_key_request:
                    SendMessage(context,ServerMessageType.general_error,0,None)
                    return
                if HandlePubKey(context,Messsage)==False:# try to handle pub-key
                    return False;
                
                AESKey = CryptoWrapper.random_aes_key()#create aes key 
                context.AESKey=AESKey#save in context
                db_instance =DBWrapper.ThreadSafeSQLite()
                db_instance.set_user_aes_key(Messsage.ID, AESKey)
                #save in db the aes key
                #encrypt key and send it
                encryptedAESKey=CryptoWrapper.rsa_encryption( context.PubKey,AESKey)
                payload=ClientId+encryptedAESKey
                SendMessage(context,ServerMessageType.pub_rsa_received_sending_encrypted_aes,len(payload),payload)
                return True
        else:
            SendMessage(context,ServerMessageType.register_failure_response,0,None) 
    elif Messsage.code == ClientMessageType.reconnect_request:#if client wants to reconnect
        context.ID=Messsage.ID.bytes
        if CanReconnectClient(Messsage.Payload,Messsage.ID):
            context.PubKey= GetPubKey(context)
            AESKey = CryptoWrapper.random_aes_key()#create aes key 
            context.AESKey=AESKey#save in context
            #save in db the aes key
            db_instance =DBWrapper.ThreadSafeSQLite()
            db_instance.set_user_aes_key(Messsage.ID, AESKey)
            #encrypt key and send it
            encryptedAESKey=CryptoWrapper.rsa_encryption( context.PubKey,AESKey)
            payload=context.ID+encryptedAESKey
            SendMessage(context,ServerMessageType.reconnect_allowed_sending_aes_key,len(payload),payload)
            registered= True
        else:
            SendMessage(context,ServerMessageType.reconnect_denied,len(context.ID),context.ID)
            registered= False
    else:
        SendMessage(context,ServerMessageType.general_error,0,None)
    
    return registered

#registraition
def CanRegisterClient(ClientNameString:bytes):
    CanConnect:bool = False
    db_instance =DBWrapper.ThreadSafeSQLite()
    CanConnect=not db_instance.user_name_exists(ClientNameString)
    return CanConnect

def CanReconnectClient(ClientNameString:bytes,cid:uuid):
    CanConnect:bool = False
    db_instance =DBWrapper.ThreadSafeSQLite()
    CanConnect= db_instance.rsa_for_user_exists(ClientNameString,cid)
    return CanConnect

def RegisterClient(ClientNameString:bytes):
    db_instance =DBWrapper.ThreadSafeSQLite()
    c_uuid=db_instance.create_user(ClientNameString)
    
    return c_uuid

#pubkey  handling
def HandlePubKey(context:ClientContext,msg:ClientMessage):
    res:bool=False
    if(msg.PayloadSize>=ClientContext.SEND_PUB_KEY_PAYLOAD_SIZE):
        db_instance =DBWrapper.ThreadSafeSQLite()
        is_there_user=db_instance.user_exists(msg.Payload[:ClientContext.NAME_SIZE],msg.ID)
        if(is_there_user==False):
            return False
        ClientContext.PubKey=msg.Payload[ClientContext.NAME_SIZE:]
        db_instance.set_user_pub_key(msg.ID, context.PubKey)
        return True
    else:
        res=False
    return res

def GetPubKey(context:ClientContext):
    db_instance =DBWrapper.ThreadSafeSQLite()
    return db_instance.get_user_pub_key(context)


def handleReceiveFile(Context:ClientContext):
    FileSendDone=False
    while not FileSendDone:
        RecFileName, RecFileData, RecCRC = GetClientFile(Context)    
        ClientMsg=ReceiveMesssage(Context)
        if ClientMessage is None:
            print("error with client socket")
            return False;
        if ClientMsg.code== ClientMessageType.correct_crc_code_response:
            print("correct crc response from client")
            FileSendDone=SaveFile(Context,RecFileName, RecFileData)
            if(not FileSendDone):
                SendGeneralError(Context)
            else:
                SendOkMessage(Context);
            FileSendDone=True
        elif ClientMsg.code == ClientMessageType.terminate_connect_invalid_crc_code_response:
            FileSendDone=True
            SendOkMessage(Context);
    pass
        
def GetClientFile(Context:ClientContext):
    #get client message file, and check crc stuff loop 
    msg =ReceiveMesssage(Context)# get the file msg
    if msg ==None:
        return False
    if not msg.code==ClientMessageType.send_file_request:
        SendGeneralError(Context);
    try:
        byte_array = msg.Payload[:4]
        msg.Payload=msg.Payload[4:]
        integer_value = int.from_bytes(byte_array, byteorder='little')
        
        FileNameArr=msg.Payload[:255]
        msg.Payload=msg.Payload[255:]
        
        string_data = FileNameArr.decode('ascii', errors='ignore')

                    # Cut the string at the position of the null byte
        FileName = string_data.split('\x00')[0]
        filedata =  CryptoWrapper.aes_decryption(Context.AESKey,msg.Payload)
        FileCRC =cksum.memcrc(filedata)
        SendCRC(Context,len(msg.Payload),FileName,FileCRC)
    except Exception as e:
        SendGeneralError(Context);
    return FileName,filedata,FileCRC
        
def SaveFile(Context:ClientContext,RecFileName:str, RecFileData:bytes):
    success:bool =False
    #db save
    db_instance =DBWrapper.ThreadSafeSQLite()
    if not db_instance.create_userfile(Context,RecFileName):
        #clean file and send general error
        return False
    else:
        Created=Utils.CreateUserFile(Context,RecFileName,RecFileData)
        if(Created):
            if not Created:
                Utils.DeleteUserFile(Context,RecFileName)            
            else:
                success=True;
    return success
    #file save

def SendCRC(context:ClientContext,ContentSize:int,FileName:bytes,crc:int ):
    DataToSend:bytes = bytes(context.ID)
    
    ContentSize_bytes = struct.pack('<I', ContentSize)
    DataToSend=DataToSend+ContentSize_bytes
    
    padded_name_bytes =str.encode( FileName.ljust(255, "\0"))
    DataToSend=DataToSend+padded_name_bytes
    
    crc_bytes = struct.pack('<I', crc)
    DataToSend=DataToSend+crc_bytes
    SendMessage(context,ServerMessageType.file_received_sending_crc_checksum,len(DataToSend),DataToSend)
    
    # recieve message in context
def ReceiveMesssage(context:ClientContext):
    
    #recieve 23 first bytes to build message header
    while len(context.BufferedData)<ClientContext.CLIENT_MESSAGE_HEADER_SIZE:
        (data_rec, data_rec_len) = ReceiveData(context.soc)
        if data_rec_len==0:
            return None
            
        context.BufferedData = context.BufferedData + data_rec
    
    #build message header
    Request:ClientMessage=ClientMessage(context.BufferedData[:ClientContext.CLIENT_MESSAGE_HEADER_SIZE])
    context.BufferedData=context.BufferedData[ClientContext.CLIENT_MESSAGE_HEADER_SIZE:]
    
    #handle payload
    while Request.PayloadSize>len(context.BufferedData):
        (data_rec, data_rec_len) = ReceiveData(context.soc)
        if data_rec_len==0:
            return None
        
        context.BufferedData = context.BufferedData + data_rec

    # Extract the payload
    Request.Payload = context.BufferedData[:Request.PayloadSize]
    # Remove the extracted bytes from the buffer
    context.BufferedData = context.BufferedData[Request.PayloadSize:]
    
    db_instance =DBWrapper.ThreadSafeSQLite()
    db_instance.update_user_last_seen(context)
    return Request

def SendOkMessage(Context):
    DataToSend:bytes = bytes(Context.ID)
    SendMessage(Context,ServerMessageType.message_received,len(DataToSend),DataToSend)
    
#send message to client via context
def SendMessage(context:ClientContext,MessageCode,bufferSize,buffer):
    Message:ServerMessage=ServerMessage(MessageCode,bufferSize,buffer)
    buffer:bytes= Message.ToBuffer()
    SendData(context.soc,buffer)
    pass
    
# data receive
def ReceiveData(soc:socket):
    try:
        data = soc.recv(1024)
        if not data:
            print("Connection closed by the client. Goodbye!")  # debug
            return b"", 0
        else:
            if len(data) > 0:
                #Utils.print_dataArr(data)  # debug
                return data, len(data)
    except Exception as e:
        print(f"Error occurred while handling client: {e}")
        return b"", 0
    
# data send
def SendData(soc:socket, data:bytes):
    try:
        if not data or len(data) == 0:
            return False

        if not isinstance(data, bytes):
            encoded_data = data.encode()
        else:
            encoded_data = data

        try:
            soc.send(encoded_data)
        except:
            print("failed to send message to socket, socket closed")
            return False
        return True

    except Exception as e:
        print(f"Error occurred while handling client: {e}")
        return False

def SendGeneralError(context:ClientContext):
    print("sending error message to client")
    SendMessage(context,ServerMessageType.general_error,0,None)