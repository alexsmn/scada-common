set PATH=%PATH%;d:\vcpkg\installed\x86-windows\tools\protobuf
rem protoc --cpp_out=. --python_out=. scada.proto
protoc --python_out=. scada.proto