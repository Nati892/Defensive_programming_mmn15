from enum import Enum
import struct

class MessageCodes(Enum):
    HELLO = 1
    BYE = 2

class RequestMessage:
    ClientId:int
    def PackClass(self):
        return struct.pack("i",self.ClientId)

    def UnpackMessage(self,ByteStream):
        self.ClientId=struct.unpack("i",ByteStream)


class ResponseMessage:
    ClientId:int
    def PackClass(self):
        return struct.pack("i",self.ClientId)

    def UnpackMessage(self,ByteStream):
        self.ClientId=struct.unpack("i",ByteStream)