#holds context for conversation with client
import socket

class ClientContext:
        soc:socket
        Authenticated:bool = False
        BufferedData = b""
        ID:bytes
        PubKey:bytes
        AESKey:bytes
        def __init__(self,soc:socket):
            self.soc=soc
            
            
            
            
        CLIENT_MESSAGE_HEADER_SIZE =23  
        SEND_PUB_KEY_PAYLOAD_SIZE =415
        NAME_SIZE=255
