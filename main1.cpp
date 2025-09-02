#include <iostream>
#include "HCNetSDK.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <thread>
#include <sstream>
#include <cstring>  // ���ͷ�ļ�

struct CameraConfig {
    std::string index;
    std::string ip;
    std::string username;
    std::string password;
    int channel;
    LONG userID;
    LONG realPlayHandle;
};

// �ص���������
void CALLBACK RealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser);

void CaptureAndShowCameraStream(CameraConfig& cameraConfig) {
    // ��ʼ��SDK
    if (!NET_DVR_Init()) {
        std::cerr << "NET_DVR_Init failed: " << NET_DVR_GetLastError() << std::endl;
        return;
    }

    // ����SDK��־(��Ҫ�����ֶ�)
    NET_DVR_SetLogToFile(3, const_cast<char*>("./sdk_log/"));

    // ׼����¼�����������ַ�������const���⣩
    char ip[64] = {0};
    char username[64] = {0};
    char password[64] = {0};
    strncpy(ip, cameraConfig.ip.c_str(), sizeof(ip)-1);
    strncpy(username, cameraConfig.username.c_str(), sizeof(username)-1);
    strncpy(password, cameraConfig.password.c_str(), sizeof(password)-1);

    // ��¼�豸 - ʹ��V30�ӿ�
    NET_DVR_DEVICEINFO_V30 deviceInfo = {0};
    cameraConfig.userID = NET_DVR_Login_V30(
        ip,
        8000,
        username,
        password,
        &deviceInfo);

    if (cameraConfig.userID < 0) {
        LONG lastError = NET_DVR_GetLastError();
        std::cerr << "Login failed, error code: " << lastError << std::endl;
        
        // ��ȡ��ϸ��������
        char* errorMsg = NET_DVR_GetErrorMsg(&lastError);
        if (errorMsg) {
            std::cerr << "Error message: " << errorMsg << std::endl;
            // ����SDK�ĵ���������ͷ�errorMsg
            // ���SDKҪ���ͷţ�ͨ��ʹ�� NET_DVR_FreeBuffer �� delete[]
            delete[] errorMsg;  // �� NET_DVR_FreeBuffer(errorMsg);
        }
        
        NET_DVR_Cleanup();
        return;
    }

    // ����Ԥ������
    NET_DVR_PREVIEWINFO previewInfo = {0};
    previewInfo.lChannel = cameraConfig.channel;
    previewInfo.dwStreamType = 0;
    previewInfo.dwLinkMode = 0;
    previewInfo.bBlocked = 1;

    // ����ʵʱԤ��
    cameraConfig.realPlayHandle = NET_DVR_RealPlay_V40(
        cameraConfig.userID, 
        &previewInfo, 
        RealDataCallBack, 
        NULL);

    if (cameraConfig.realPlayHandle < 0) {
        std::cerr << "RealPlay failed, error: " << NET_DVR_GetLastError() << std::endl;
        NET_DVR_Logout(cameraConfig.userID);
        NET_DVR_Cleanup();
        return;
    }

    std::cout << "Start preview success! IP: " << cameraConfig.ip << std::endl;

    // ��ѭ��
    while (true) {
        if (cv::waitKey(100) == 27) {
            break;
        }
    }

    // ֹͣԤ�����ǳ�
    NET_DVR_StopRealPlay(cameraConfig.realPlayHandle);
    NET_DVR_Logout(cameraConfig.userID);
    NET_DVR_Cleanup();
}

// ʵʱ���ص�����
void CALLBACK RealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser) {
    static int frameCount = 0;
    if (++frameCount % 100 == 0) {
        std::cout << "Received frame, size: " << dwBufSize << std::endl;
    }
}

int ReadCameraConfig(std::vector<CameraConfig>& cameras, const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << configFile << std::endl;
        return 0;
    }

    std::string line;
    int index = 1;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        CameraConfig config;
        if (iss >> config.ip >> config.username >> config.password >> config.channel) {
            config.index = std::to_string(index++);
            cameras.push_back(config);
        }
    }
    return cameras.size();
}

int main() {
    // ��ʼ��SDK
    if (!NET_DVR_Init()) {
        std::cerr << "SDK initialization failed!" << std::endl;
        return -1;
    }

    // ����SDK��־
    NET_DVR_SetLogToFile(3, const_cast<char*>("./sdk_log/"));

    std::vector<CameraConfig> cameras;
    int camera_num = ReadCameraConfig(cameras, "cameras_config.txt");
    if (camera_num == 0) {
        std::cerr << "No camera config found!" << std::endl;
        NET_DVR_Cleanup();
        return -1;
    }

    std::cout << "Found " << camera_num << " cameras" << std::endl;

    // �����߳�
    std::vector<std::thread> threads;
    for (auto& camera : cameras) {
        threads.emplace_back(CaptureAndShowCameraStream, std::ref(camera));
    }

    // �ȴ��߳̽���
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    NET_DVR_Cleanup();
    return 0;
}