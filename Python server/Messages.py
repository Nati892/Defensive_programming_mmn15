import struct
import uuid
from enum import IntEnum

CLIENT_MESSAGE_HEADER_SIZE =23
class ClientMessage:
    ID:uuid
    Version:int=0
    code:int=0
    PayloadSize:int=0
    Payload:bytes=0

    def __init__(self,binary_data):
        # Ensure the length of the binary data is 23 bytes
        if len(binary_data) != 23:
            raise ValueError("Invalid length of binary data")

        # Unpack the binary data using struct format
        unpacked_data = struct.unpack("<16s B H I", binary_data)

        # Extract individual fields
        id_bytes, version, code, size = unpacked_data

        # Convert the ID field to a UUID
        parsed_id = uuid.UUID(bytes=id_bytes)

        self.ID=parsed_id
        self.Version=version
        self.code=code
        self.Payload=size

class ServerMessageType(IntEnum):
    register_success_response = 2100
    register_failure_response = 2101
    pub_rsa_received_sending_encrypted_aes = 2102
    file_received_sending_crc_checksum = 2103
    message_received = 2104
    reconnect_allowed_sending_aes_key = 2105
    reconnect_denied = 2106
    general_error = 2107

class ClientMessageType(IntEnum):
    register_request = 1025
    send_public_key_request = 1026
    reconnect_request = 1027
    send_file_request = 1028
    correct_crc_code_response = 1029
    invalid_crc_code_response = 1030
    terminate_connect_invalid_crc_code_response = 1031