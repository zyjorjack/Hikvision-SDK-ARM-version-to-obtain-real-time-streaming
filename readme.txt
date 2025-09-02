1.将lib下所有的库拷贝到 hiklib 下，包括 HCNetSDKCom 文件夹
2.在终端输入 make 命令即可生成 main，使用 ./main 即可执行

cameras_config.txt 文件用于存储多个海康威视摄像头的连接配置信息。两个程序都使用相同的格式读取这个文件。
每行代表一个摄像头的配置，包含以下四个字段，字段之间用空格分隔：
IP地址 用户名 密码 通道号

字段说明：
IP地址 - 摄像头的网络地址（例如：192.168.1.64）
用户名 - 登录摄像头所需的用户名（例如：admin）
密码 - 登录摄像头所需的密码（例如：12345）
通道号 - 摄像头通道号（通常是整数，如 1 表示主码流）

示例文件内容：
192.168.1.64 admin 12345 1
192.168.1.66 admin password 2

关于main.cpp和main1.cpp相同之处：
功能目标一致：都是读取配置文件（cameras_config.txt），登录多个摄像头，并实时显示视频流。
依赖库相同：都使用了 HCNetSDK.h（海康SDK）和 opencv2/opencv.hpp（图像处理）。
多线程架构：都使用 std::thread 为每个摄像头创建一个线程进行视频流捕获和显示。
基本流程一致：初始化 SDK → 读取配置 → 登录设备 → 开始预览 → 循环捕获图像 → 退出清理。

关于main.cpp和main1.cpp不同之处：
main.cpp 使用 NET_DVR_CaptureJPEGPicture_NEW 捕获单帧 JPEG 图像。
main1.cpp 使用 NET_DVR_RealPlay_V40 并设置回调函数 RealDataCallBack 接收实时流。

main.cpp 直接使用 std::string 赋值到 NET_DVR_USER_LOGIN_INFO 结构体。
main1.cpp 使用 strncpy 安全拷贝字符串到字符数组，避免内存越界。

main1.cpp 定义了 RealDataCallBack 回调函数用于接收实时流数据，但未实现图像显示。
main.cpp 没有使用回调，而是主动轮询抓图。
