#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <memory>
#include "MvCameraControl.h"

class HikCamera
{
public:
    HikCamera();
    ~HikCamera();

    // 禁用拷贝，允许移动（可选）
    HikCamera(const HikCamera &) = delete;
    HikCamera &operator=(const HikCamera &) = delete;

    // 枚举所有可用设备
    static std::vector<MV_CC_DEVICE_INFO> EnumerateDevices();

    // 打开指定索引的相机（默认第一个）
    bool open(int deviceIndex = 0);

    // 关闭相机
    void close();

    // 抓取一帧图像，返回 cv::Mat（空表示失败）
    cv::Mat grabFrame(int timeoutMs = 1000);

    // 是否已打开
    bool isOpen() const { return m_isOpened; }

    bool setCameraParameters(int width, int height, float frameRate,
                             float exposureTime, float gain);

private:
    void *m_handle = nullptr;
    bool m_isOpened = false;
    unsigned int m_payloadSize = 0;
    std::vector<unsigned char> m_buffer;

    bool setupCamera();
    cv::Mat convertToMat(const MV_FRAME_OUT_INFO_EX &frameInfo, unsigned char *pData);
    void RGB2BGR(unsigned char *data, unsigned int width, unsigned int height);
};