# xrnNetwork
[![Continous Integration](https://github.com/DiantArts/xrnNetwork/actions/workflows/continuous-integration.yml/badge.svg)](https://github.com/DiantArts/xrnNetwork/actions/workflows/continuous-integration.yml)

Linux only (because of CMake) TCP and UDP client/server.

# How to run
Run the following commands to install the dependencies (ubuntu-22.04 example):
```
apt-get install -y python3-pip cppcheck clang-tidy ccache moreutils cmake gcovr llvm
```
```
pip install conan ninja
```

Launch the server first (compiling the client first prevent the compilation of the server - sorry):
```
./.toolchain/compile --server
```
Launch the client
```
./.toolchain/compile
```

Use `-d` to enable debug.

`\q` in the running console to close an instance (client or server)
