
all:
    cd ../Server
    go build -o ../HTTPServer/GoServer
    cd ../
    msbuild URemoteControl.sln /p:Configuration=Release /p:Platform=x64



