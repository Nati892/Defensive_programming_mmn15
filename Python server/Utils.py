import os
from Client import ClientContext
USERS_DIR="UserFiles"
def CreateUserFile(Context:ClientContext, fileName, FileData: bytes)->bool:
    # Combine folder name and file name to get the complete file path
    fPath= Context.ID.hex()
    FolderPath=os.path.join(USERS_DIR,fPath)
    file_path = os.path.join(FolderPath, fileName)
    
    try:
        # Ensure the directory structure exists
        os.makedirs(FolderPath, exist_ok=True)

        # Write the bytes content to the file
        with open(file_path, 'wb') as file:
            file.write(FileData)
        return True;
    except Exception as e:
        return False
        
        
def DeleteUserFile(Context:ClientContext, name):
    # Combine folder path and file name to get the complete file path
    fPath= Context.ID.hex()
    file_path = os.path.join(fPath, name)
    try:
        # Attempt to remove the file
        os.remove(fPath)
        print(f"File '{name}' has been deleted.")
    except Exception as e:
        print(f"Error deleting file '{name}': {str(e)}")
        
def print_dataArr(arr):
    index= 0
    for i in arr:
        index+=1
        print(f"{index} : {i}\n")