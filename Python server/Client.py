import FileServer

#Init config

def FetchConfig():
    print("Fetching config from local Folder")
    conf = FileServer.Config()
    conf.FetchLocalConfig()
    return conf
    
if __name__ == "__main__":
    ServerConfig=FetchConfig()
    FileServer=FileServer.Server(ServerConfig)
    FileServer.Run()