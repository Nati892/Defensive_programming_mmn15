
import sqlite3
import threading
import sqlite3
import threading
import uuid
from datetime import datetime

class ThreadSafeSQLite:
    _instance = None
    _lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        with cls._lock:
            if not cls._instance:
                cls._instance = super().__new__(cls)
                cls._instance._connection = None
                cls._instance._cursor = None
                cls._instance._connect()
                cls._instance.ensure_tables_created()
            return cls._instance

    def _connect(self):
        self._connection = sqlite3.connect("example.db")
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
                    ID TEXT(36) PRIMARY KEY,
                    Name TEXT(255),
                    PublicKey BLOB(160),
                    LastSeen DATETIME,
                    AESKey BLOB(128)
                )
            """)

        if not files_table_exists:
            self._cursor.execute("""
                CREATE TABLE files (
                    ID TEXT(36) PRIMARY KEY,
                    FileName TEXT(255),
                    PathName TEXT(255),
                    Verified BOOLEAN
                )
            """)
        self._connection.commit()
    

    def user_exists(self, name, user_id):
        count=-1
        self._connect()
        try:
            self._cursor.execute("SELECT COUNT(*) FROM clients WHERE Name=? AND ID=?", (name,user_id))
            count = self._cursor.fetchone()[0]
        finally:
            self._disconnect()
            return count 

    def create_user(self, name):
        user_id=None
        with self._lock:
            self._connect()
            try:
                user_id = str(uuid.uuid4())
                self._cursor.execute("INSERT INTO clients (ID, Name) VALUES (?, ?)", (user_id, name))
                self._connection.commit()
            #make sure its created
                if self.user_exists(name,user_id) >0:
                    return user_id
            except Exception as e:
                return None
            finally:
                self._disconnect()
                return user_id

    def user_name_exists(self, name):
        with self._lock:
            self._connect()
            try:
                self._cursor.execute("SELECT COUNT(*) FROM clients WHERE Name=?", (name,))
                count = self._cursor.fetchone()[0]
                return count > 0
            finally:
                self._disconnect()

    def update_user_last_seen(self, name):
        with self._lock:
            self._connect()
            try:
                now=datetime.now()
                self._cursor.execute("UPDATE clients SET LastSeen=? WHERE Name=?", (now, name))
                self._connection.commit()
            finally:
                self._disconnect()