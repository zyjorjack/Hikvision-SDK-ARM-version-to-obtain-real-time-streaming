#include <iostream>
#include "HCNetSDK.h"  // ������������ SDK ��ͷ�ļ�
#include <opencv2/opencv.hpp>  // ����OpenCV��ͷ�ļ�
#include <fstream>  // �ļ���
#include <vector> 
#include <thread>  // �߳�
#include <sstream>

// ����ͷ���ýṹ��
struct CameraConfig {
	std::string index;  // �����ļ����
	std::string ip;
	std::string username;
	std::string password;
	int channel;
	LONG userID;
	LONG realPlayHandle;
};

// ���ͼ����ʾ����
void CaptureAndShowCameraStream(CameraConfig& cameraConfig) {
	

	// ��¼�豸
	NET_DVR_USER_LOGIN_INFO loginInfo = { 0 };
	NET_DVR_DEVICEINFO_V40 deviceInfo = { 0 };

	// �����豸��Ϣ
	strncpy(loginInfo.sDeviceAddress, cameraConfig.ip.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);  // ip
	loginInfo.wPort = 8000;  // �˿ں�Ĭ��8000
	strncpy(loginInfo.sUserName, cameraConfig.username.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);    // �û���
	strncpy(loginInfo.sPassword, cameraConfig.password.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);    // ����

	cameraConfig.userID = NET_DVR_Login_V40(&loginInfo, &deviceInfo);
	if (cameraConfig.userID < 0) {
		std::cout << "ip:" << cameraConfig.ip << " The device login failed, error code:" << NET_DVR_GetLastError() << std::endl;
		return;
	}

	// ����Ԥ����Ϣ
	NET_DVR_PREVIEWINFO previewInfo = { 0 };
	previewInfo.lChannel = cameraConfig.channel;
	previewInfo.dwStreamType = 0;  // ������
	previewInfo.dwLinkMode = 0;
	previewInfo.bBlocked = 0;

	// ����ʵʱԤ��
	cameraConfig.realPlayHandle = NET_DVR_RealPlay_V40(cameraConfig.userID, &previewInfo, NULL, NULL);
	if (cameraConfig.realPlayHandle < 0) {
		std::cout << "ip:" << cameraConfig.ip << " Device preview failed with error code:" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(cameraConfig.userID);
		return;
	}

	// ����OpenCV����
	std::string windowname = "Camera Stream - " + cameraConfig.ip + "-" +cameraConfig.index;
	cv::namedWindow(windowname, cv::WINDOW_NORMAL);
	std::cout << "ip:" << cameraConfig.ip << " Device preview successful!" << std::endl;

	// ����ץͼ
	NET_DVR_JPEGPARA* img_para = new NET_DVR_JPEGPARA;
	img_para->wPicQuality = 1;
	img_para->wPicSize = 9;  // ���÷ֱ���

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

		// תΪMat��ʽ
		cv::Mat grabImg = cv::imdecode(cv::Mat(1, picSize, CV_8UC1, jpegBuffer), cv::IMREAD_COLOR);

		// ��ʾͼ��
		// cv::imshow(windowname, grabImg);

		// ��ESC�˳�
		if (cv::waitKey(1) == 27) {
			break;
		}

		delete[] jpegBuffer;
	}

	// ������Դ
	NET_DVR_StopRealPlay(cameraConfig.realPlayHandle);
	NET_DVR_Logout(cameraConfig.userID);
	delete img_para;
}

// ��ȡ�������
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
	// ��ʼ����������SDK
	if (!NET_DVR_Init()) {
		std::cout << "SDK initialization failed!" << std::endl;
		return -1;
	}

	std::vector<CameraConfig> cameras;

	// ��ȡ��������ļ�
	int camera_num = ReadCameraConfig(cameras, "cameras_config.txt");
	std::cout << "Get " << camera_num << " Camera equipment!" << std::endl;

	// ����ÿ���������Ƶ��
	std::vector<std::thread> threads;
	for (auto& camera : cameras) {
		threads.push_back(std::thread(CaptureAndShowCameraStream, std::ref(camera)));
	}

	// �ȴ������߳����
	for (auto& t : threads) {
		t.join();
	}

	// ����SDK��Դ
	NET_DVR_Cleanup();

	return 0;
}