
import os
import sqlite3
import threading
import sqlite3
import threading
import uuid
from datetime import datetime

from Client import ClientContext

class ThreadSafeSQLite:
    _instance = None
    _lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        with cls._lock:
            NoExist=os.path.isfile("defensive.db")
            if not cls._instance or NoExist:
                cls._instance = super().__new__(cls)
                cls._instance._connection = None
                cls._instance._cursor = None
                cls._instance._connect()
                cls._instance.ensure_tables_created()
            return cls._instance

    def _connect(self):
        self._connection = sqlite3.connect("defensive.db")
        self._cursor = self._connection.cursor()

    def _disconnect(self):
        if self._connection:
            self._connection.close()
            self._connection = None
            self._cursor = None

    def execute_query(self, query):
        with self._lock:
            try:
                if not self._connection:
                    self._connect()
                self._cursor.execute(query)
                result = self._cursor.fetchall()
                return result
            finally:
                self._disconnect()

    def close_connection(self):
        with self._lock:
            self._disconnect()

    def ensure_tables_created(self):
        if not self._connection:
            self._connect()

        # Check if the tables exist
        self._cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='clients'")
        clients_table_exists = bool(self._cursor.fetchone())

        self._cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='files'")
        files_table_exists = bool(self._cursor.fetchone())

        # Create the tables if they don't exist
        if not clients_table_exists:
            self._cursor.execute("""
                CREATE TABLE clients (
                    ID TEXT(16) PRIMARY KEY,
                    Name TEXT(255),
                    PublicKey BLOB(160),
                    LastSeen DATETIME,
                    AESKey BLOB(16)
                )
            """)

        if not files_table_exists:
            self._cursor.execute("""
                CREATE TABLE files (
                    ID TEXT(16),
                    FileName TEXT(255),
                    PathName TEXT(255),
                    Verified BOOLEAN
                )
            """)
        self._connection.commit()
    

    def user_exists(self, name, user_id:uuid):
        count=-1
        self._connect()
        try:
# Convert bytes to string using ASCII encoding
            string_data = name.decode('ascii', errors='ignore')

# Cut the string at the position of the null byte
            string_cut_at_null = string_data.split('\x00')[0]
            user_id.bytes
            self._cursor.execute("SELECT COUNT(*) FROM clients WHERE Name=? AND ID=?", (string_cut_at_null,user_id.bytes))
            count = self._cursor.fetchone()[0]
        except Exception as e:
            pass
        finally:
            self._disconnect()
            return count 

    def create_user(self, name):
        user_id=None
        with self._lock:
            self._connect()
            try:
                uidHex=uuid.uuid4()
                user_id = uidHex.bytes
                # Convert bytes to string using ASCII encoding
                string_data = name.decode('ascii', errors='ignore')

                # Cut the string at the position of the null byte
                string_cut_at_null = string_data.split('\x00')[0]
            
                self._cursor.execute("INSERT INTO clients (ID, Name) VALUES (?, ?)", (user_id, string_cut_at_null))
                self._connection.commit()
            #make sure its created
                if self.user_exists(name,uidHex) >0:
                    return user_id
            except Exception as e:
                return None
            finally:
                self._disconnect()
                return user_id

    def create_userfile(self,context:ClientContext, FileName):
        user_id=context.ID.hex()
        FilePath:str=user_id+"/"+FileName
        with self._lock:
            self._connect()
            try:
                self._cursor.execute("INSERT INTO files (ID, FileName,PathName,Verified) VALUES (?, ?, ?, ?)", (context.ID, FileName,FilePath,True))
                self._connection.commit()
            #make sure its created
            except Exception as e:
                self._disconnect()
                return False
            finally:
                self._disconnect()
        return self.file_exists(context.ID)>0
           
            
            
    def user_name_exists(self, name):
        with self._lock:
            string_data = name.decode('ascii', errors='ignore')
            # Cut the string at the position of the null byte
            string_cut_at_null = string_data.split('\x00')[0]
            self._connect()
            try:
                self._cursor.execute("SELECT COUNT(*) FROM clients WHERE Name=?", (string_cut_at_null,))
                count = self._cursor.fetchone()[0]
                return count > 0
            except Exception as e:
                pass
            finally:
                self._disconnect()

    def file_exists(self,ID:bytes):
        with self._lock:
            self._connect()
            try:
                self._cursor.execute("SELECT COUNT(*) FROM files WHERE ID=?", (ID,))
                count = self._cursor.fetchone()[0]
                return count > 0
            except Exception as e:
                pass
            finally:
                self._disconnect()
                
    def rsa_for_user_exists(self, name:bytes,cid:uuid):
        with self._lock:
            self._connect()
            try:
                string_data = name.decode('ascii', errors='ignore')

                # Cut the string at the position of the null byte
                string_cut_at_null = string_data.split('\x00')[0]
                self._cursor.execute("SELECT COUNT(*) FROM clients WHERE Name=? AND ID IS NOT NULL AND ID=? AND publickey IS NOT NULL AND PublicKey <> ''", (string_cut_at_null,cid.bytes))
                count = self._cursor.fetchone()[0]
                return count > 0
            except Exception as e:
                pass
            finally:
                self._disconnect()
                
    def update_user_last_seen(self, Context:ClientContext):
        with self._lock:
            self._connect()
            try:
                now=datetime.now()
                self._cursor.execute("UPDATE clients SET LastSeen=? WHERE ID=?", (now, Context.ID))
                self._connection.commit()
            except Exception as e:
                pass
            finally:
                self._disconnect()
                
    def set_user_pub_key(self, ID:uuid, PublicKey):
        success:bool =True
        with self._lock:
            self._connect()
            try:
                now=datetime.now()
                self._cursor.execute("UPDATE clients SET PublicKey=? WHERE ID=?", (PublicKey, ID.bytes))
                self._connection.commit()
            except Exception as e:
                print("error setting pubkey to client")
                success=False
            finally:
                self._disconnect()                
        return success
    
    
    def get_user_pub_key(self, context):
        with self._lock:
            self._connect()
            try:
                # Assuming PublicKey is the column name in your table
                self._cursor.execute("SELECT PublicKey FROM clients WHERE ID=?", (context.ID,))
                result = self._cursor.fetchone()
                if result:
                    pub_key = result[0]
                    return pub_key
                else:
                    # Return some default value or raise an exception
                    return None
            finally:
                self._disconnect()
        
    def set_user_aes_key(self, ID:uuid, AESKey):
        success:bool =True
        with self._lock:
            self._connect()
            try:
                now=datetime.now()
                self._cursor.execute("UPDATE clients SET AESKey=? WHERE ID=?", (AESKey, ID.bytes))
                self._connection.commit()
            except Exception as e:
                print("error setting AES Key to client")
                success=False
            finally:
                self._disconnect()                
        return success
    
    