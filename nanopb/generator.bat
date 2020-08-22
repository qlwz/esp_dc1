cd /d %~dp0

D:\Workspaces_Smarthome\esp\nanopb\generator-bin\protoc.exe --nanopb_out=. DC1Config.proto
copy DC1Config.pb.h ..\include /y
copy DC1Config.pb.c ..\src /y
del DC1Config.pb.h
del DC1Config.pb.c
