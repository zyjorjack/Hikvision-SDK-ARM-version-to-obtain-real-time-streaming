CC       = gcc
CXX      = g++
CFLAGS   = -g
CXXFLAGS = $(CFLAGS) -std=c++11

# 添加OpenCV相关路径 - 需要根据你的实际安装路径修改
OPENCV_INCLUDE = /usr/include/opencv4  # 或其他OpenCV头文件路径
OPENCV_LIB = /usr/include/lib                  # 或其他OpenCV库路径

LIBPATH = ./hiklib
LIBS = -L$(LIBPATH) -Wl,-rpath=$(LIBPATH):./hiklib/HCNetSDKCom \
       -lhcnetsdk -lssl -lcrypto \
       -L/usr/lib/aarch64-linux-gnu \
       -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -pthread

SRC = ./main.cpp
TARGET = ./main

all: 
	$(CXX) $(CXXFLAGS) -I$(OPENCV_INCLUDE) $(SRC) -o $(TARGET) -L$(LIBPATH) $(LIBS)

.PHONY: clean
clean:
	rm -f $(TARGET)