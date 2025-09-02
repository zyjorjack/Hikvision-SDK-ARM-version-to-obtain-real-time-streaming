#include <iostream>
#include "HCNetSDK.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <thread>
#include <sstream>
#include <cstring>  // 添加头文件

struct CameraConfig {
    std::string index;
    std::string ip;
    std::string username;
    std::string password;
    int channel;
    LONG userID;
    LONG realPlayHandle;
};

// 回调函数声明
void CALLBACK RealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser);

void CaptureAndShowCameraStream(CameraConfig& cameraConfig) {
    // 初始化SDK
    if (!NET_DVR_Init()) {
        std::cerr << "NET_DVR_Init failed: " << NET_DVR_GetLastError() << std::endl;
        return;
    }

    // 启用SDK日志(重要调试手段)
    NET_DVR_SetLogToFile(3, const_cast<char*>("./sdk_log/"));

    // 准备登录参数（复制字符串避免const问题）
    char ip[64] = {0};
    char username[64] = {0};
    char password[64] = {0};
    strncpy(ip, cameraConfig.ip.c_str(), sizeof(ip)-1);
    strncpy(username, cameraConfig.username.c_str(), sizeof(username)-1);
    strncpy(password, cameraConfig.password.c_str(), sizeof(password)-1);

    // 登录设备 - 使用V30接口
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
        
        // 获取详细错误描述
        char* errorMsg = NET_DVR_GetErrorMsg(&lastError);
        if (errorMsg) {
            std::cerr << "Error message: " << errorMsg << std::endl;
            // 根据SDK文档决定如何释放errorMsg
            // 如果SDK要求释放，通常使用 NET_DVR_FreeBuffer 或 delete[]
            delete[] errorMsg;  // 或 NET_DVR_FreeBuffer(errorMsg);
        }
        
        NET_DVR_Cleanup();
        return;
    }

    // 设置预览参数
    NET_DVR_PREVIEWINFO previewInfo = {0};
    previewInfo.lChannel = cameraConfig.channel;
    previewInfo.dwStreamType = 0;
    previewInfo.dwLinkMode = 0;
    previewInfo.bBlocked = 1;

    // 启动实时预览
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

    // 主循环
    while (true) {
        if (cv::waitKey(100) == 27) {
            break;
        }
    }

    // 停止预览并登出
    NET_DVR_StopRealPlay(cameraConfig.realPlayHandle);
    NET_DVR_Logout(cameraConfig.userID);
    NET_DVR_Cleanup();
}

// 实时流回调函数
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
    // 初始化SDK
    if (!NET_DVR_Init()) {
        std::cerr << "SDK initialization failed!" << std::endl;
        return -1;
    }

    // 设置SDK日志
    NET_DVR_SetLogToFile(3, const_cast<char*>("./sdk_log/"));

    std::vector<CameraConfig> cameras;
    int camera_num = ReadCameraConfig(cameras, "cameras_config.txt");
    if (camera_num == 0) {
        std::cerr << "No camera config found!" << std::endl;
        NET_DVR_Cleanup();
        return -1;
    }

    std::cout << "Found " << camera_num << " cameras" << std::endl;

    // 启动线程
    std::vector<std::thread> threads;
    for (auto& camera : cameras) {
        threads.emplace_back(CaptureAndShowCameraStream, std::ref(camera));
    }

    // 等待线程结束
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    NET_DVR_Cleanup();
    return 0;
}