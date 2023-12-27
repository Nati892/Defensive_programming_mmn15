import cksum as pycksum
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import unpad, pad
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
import zlib


def random_aes_key():
    aes_random = get_random_bytes(16)
    return aes_random


def rsa_encryption(public_key, data):
    cipher = PKCS1_OAEP.new(RSA.importKey(public_key))
    return cipher.encrypt(data)


def aes_decryption(aes_key, data):
    iv = bytearray([0] * AES.block_size)
    cipher = AES.new(aes_key, AES.MODE_CBC, iv)
    return unpad(cipher.decrypt(data), AES.block_size)


def calculate_crc(filename):
    checkSum = pycksum.cksum(open(filename))
    return checkSum


def get_crc_sum(string: bytes):
    # string = string.encode()
    return zlib.crc32(string) & 0xffffffff