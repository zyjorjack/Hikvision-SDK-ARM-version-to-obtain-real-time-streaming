#include <iostream>
#include "HCNetSDK.h"  // 包含海康威视 SDK 的头文件
#include <opencv2/opencv.hpp>  // 包含OpenCV的头文件
#include <fstream>  // 文件流
#include <vector> 
#include <thread>  // 线程
#include <sstream>

// 摄像头配置结构体
struct CameraConfig {
	std::string index;  // 配置文件序号
	std::string ip;
	std::string username;
	std::string password;
	int channel;
	LONG userID;
	LONG realPlayHandle;
};

// 相机图像显示函数
void CaptureAndShowCameraStream(CameraConfig& cameraConfig) {
	

	// 登录设备
	NET_DVR_USER_LOGIN_INFO loginInfo = { 0 };
	NET_DVR_DEVICEINFO_V40 deviceInfo = { 0 };

	// 设置设备信息
	strncpy(loginInfo.sDeviceAddress, cameraConfig.ip.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);  // ip
	loginInfo.wPort = 8000;  // 端口号默认8000
	strncpy(loginInfo.sUserName, cameraConfig.username.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);    // 用户名
	strncpy(loginInfo.sPassword, cameraConfig.password.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);    // 密码

	cameraConfig.userID = NET_DVR_Login_V40(&loginInfo, &deviceInfo);
	if (cameraConfig.userID < 0) {
		std::cout << "ip:" << cameraConfig.ip << " The device login failed, error code:" << NET_DVR_GetLastError() << std::endl;
		return;
	}

	// 设置预览信息
	NET_DVR_PREVIEWINFO previewInfo = { 0 };
	previewInfo.lChannel = cameraConfig.channel;
	previewInfo.dwStreamType = 0;  // 主码流
	previewInfo.dwLinkMode = 0;
	previewInfo.bBlocked = 0;

	// 启动实时预览
	cameraConfig.realPlayHandle = NET_DVR_RealPlay_V40(cameraConfig.userID, &previewInfo, NULL, NULL);
	if (cameraConfig.realPlayHandle < 0) {
		std::cout << "ip:" << cameraConfig.ip << " Device preview failed with error code:" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(cameraConfig.userID);
		return;
	}

	// 创建OpenCV窗口
	std::string windowname = "Camera Stream - " + cameraConfig.ip + "-" +cameraConfig.index;
	cv::namedWindow(windowname, cv::WINDOW_NORMAL);
	std::cout << "ip:" << cameraConfig.ip << " Device preview successful!" << std::endl;

	// 进行抓图
	NET_DVR_JPEGPARA* img_para = new NET_DVR_JPEGPARA;
	img_para->wPicQuality = 1;
	img_para->wPicSize = 9;  // 设置分辨率

	while (true) {
		DWORD picSize = 1920 * 1080;
		char* jpegBuffer = new char[picSize];
		LPDWORD sizeReturned = 0;

		BOOL lRet = NET_DVR_CaptureJPEGPicture_NEW(cameraConfig.userID, cameraConfig.channel, img_para, jpegBuffer, picSize, sizeReturned);
		if (lRet == FALSE) {
			std::cout << "ip:" << cameraConfig.ip << "Failed to retrieve image from device!" << std::endl;
			delete[] jpegBuffer;
			break;
		}

		// 转为Mat格式
		cv::Mat grabImg = cv::imdecode(cv::Mat(1, picSize, CV_8UC1, jpegBuffer), cv::IMREAD_COLOR);

		// 显示图像
		cv::imshow(windowname, grabImg);

		// 按ESC退出
		if (cv::waitKey(1) == 27) {
			break;
		}

		delete[] jpegBuffer;
	}

	// 清理资源
	NET_DVR_StopRealPlay(cameraConfig.realPlayHandle);
	NET_DVR_Logout(cameraConfig.userID);
	delete img_para;
}

// 读取相机配置
int ReadCameraConfig(std::vector<CameraConfig>& cameras, const std::string& configFile) {
	std::ifstream file(configFile);
	std::string line;
	int index = 1;

	while (std::getline(file, line)) {
		std::istringstream iss(line);
		CameraConfig cameraconfig;
		iss >> cameraconfig.ip >> cameraconfig.username >> cameraconfig.password >> cameraconfig.channel;
		cameraconfig.index = std::to_string(index++);
		cameras.push_back(cameraconfig);
	}

	return cameras.size();
}


int main() {
	// 初始化海康威视SDK
	if (!NET_DVR_Init()) {
		std::cout << "SDK initialization failed!" << std::endl;
		return -1;
	}

	std::vector<CameraConfig> cameras;

	// 读取相机配置文件
	int camera_num = ReadCameraConfig(cameras, "cameras_config.txt");
	std::cout << "Get " << camera_num << " Camera equipment!" << std::endl;

	// 启动每个相机的视频流
	std::vector<std::thread> threads;
	for (auto& camera : cameras) {
		threads.push_back(std::thread(CaptureAndShowCameraStream, std::ref(camera)));
	}

	// 等待所有线程完成
	for (auto& t : threads) {
		t.join();
	}

	// 清理SDK资源
	NET_DVR_Cleanup();

	return 0;

}
