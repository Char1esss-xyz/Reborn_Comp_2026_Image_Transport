#include "hik_camera.hpp"
#include <iostream>
#include <stdexcept>

HikCamera::HikCamera() = default;

HikCamera::~HikCamera()
{
    close();
}

std::vector<MV_CC_DEVICE_INFO> HikCamera::EnumerateDevices()
{
    MV_CC_DEVICE_INFO_LIST stDevList;
    memset(&stDevList, 0, sizeof(stDevList));

    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDevList);
    if (MV_OK != nRet)
    {
        throw std::runtime_error("Failed to enumerate devices");
    }

    std::vector<MV_CC_DEVICE_INFO> devices;
    for (unsigned int i = 0; i < stDevList.nDeviceNum; ++i)
    {
        if (stDevList.pDeviceInfo[i])
        {
            devices.push_back(*stDevList.pDeviceInfo[i]);
        }
    }
    return devices;
}

bool HikCamera::setCameraParameters(int width, int height, float frameRate,
                                    float exposureTime, float gain)
{
    if (!m_isOpened || !m_handle)
    {
        std::cerr << "Error: Cannot set parameters - camera not opened" << std::endl;
        return false;
    }

    int nRet = MV_OK;

    // First, check if this is a color camera (monochrome cameras don't have white balance)
    bool isColorCamera = false;
    MVCC_ENUMVALUE stColorMode = {0};
    if (MV_OK == MV_CC_GetEnumValue(m_handle, "PixelColorFilter", &stColorMode))
    {
        isColorCamera = (stColorMode.nCurValue > 0); // 0 typically means mono
    }

    // 1. Set resolution (if specified)
    if (width > 0 && height > 0)
    {
        // First check if the camera supports this resolution
        MVCC_INTVALUE stMaxWidth = {0};
        MVCC_INTVALUE stMaxHeight = {0};
        MV_CC_GetIntValue(m_handle, "WidthMax", &stMaxWidth);
        MV_CC_GetIntValue(m_handle, "HeightMax", &stMaxHeight);

        if (width <= static_cast<int>(stMaxWidth.nCurValue) &&
            height <= static_cast<int>(stMaxHeight.nCurValue))
        {

            nRet = MV_CC_SetIntValue(m_handle, "Width", width);
            if (MV_OK != nRet)
            {
                std::cerr << "Set Width failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
                return false;
            }

            nRet = MV_CC_SetIntValue(m_handle, "Height", height);
            if (MV_OK != nRet)
            {
                std::cerr << "Set Height failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
                return false;
            }
        }
        else
        {
            std::cerr << "Requested resolution " << width << "x" << height
                      << " exceeds maximum supported resolution" << std::endl;
            return false;
        }
    }

    // 2. Set frame rate
    nRet = MV_CC_SetEnumValue(m_handle, "AcquisitionFrameRateMode", 1); // 1 = Custom, 0 = Default
    if (MV_OK == nRet)
    {
        nRet = MV_CC_SetFloatValue(m_handle, "AcquisitionFrameRate", frameRate);
        if (MV_OK != nRet)
        {
            std::cerr << "Set frame rate failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
            // Continue anyway - not critical
        }
    }

    // 3. Set exposure time (if specified, -1 means don't change)
    if (exposureTime > 0)
    {
        // First check if exposure time is adjustable
        MVCC_ENUMVALUE stExposureAuto = {0};
        MV_CC_GetEnumValue(m_handle, "ExposureAuto", &stExposureAuto);

        if (stExposureAuto.nCurValue > 0)
        {                                                           // If auto is enabled (1=Once, 2=Continuous)
            nRet = MV_CC_SetEnumValue(m_handle, "ExposureAuto", 0); // 0 = Off
        }

        // Set manual exposure time (in microseconds)
        nRet = MV_CC_SetFloatValue(m_handle, "ExposureTime", exposureTime);
        if (MV_OK != nRet)
        {
            std::cerr << "Set exposure time failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
        }
    }

    // 4. Set gain (if specified, -1 means don't change)
    if (gain > 0)
    {
        // First check if gain auto is enabled
        MVCC_ENUMVALUE stGainAuto = {0};
        MV_CC_GetEnumValue(m_handle, "GainAuto", &stGainAuto);

        if (stGainAuto.nCurValue > 0)
        {                                                       // If auto is enabled
            nRet = MV_CC_SetEnumValue(m_handle, "GainAuto", 0); // 0 = Off
        }

        nRet = MV_CC_SetFloatValue(m_handle, "Gain", gain);
        if (MV_OK != nRet)
        {
            std::cerr << "Set gain failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
        }
    }

    std::cout << "Camera parameters configured successfully" << std::endl;
    return true;
}
bool HikCamera::open(int deviceIndex)
{
    if (m_isOpened)
    {
        close();
    }

    auto devices = EnumerateDevices();
    if (deviceIndex < 0 || static_cast<size_t>(deviceIndex) >= devices.size())
    {
        std::cerr << "Invalid device index: " << deviceIndex << std::endl;
        return false;
    }

    const auto &devInfo = devices[deviceIndex];

    // 检查设备是否可访问
    // Cast away const — safe here because devInfo is a copy or mutable buffer
    if (!MV_CC_IsDeviceAccessible(
            const_cast<MV_CC_DEVICE_INFO *>(&devInfo),
            MV_ACCESS_Exclusive))
    {
        std::cerr << "Device is not accessible." << std::endl;
        return false;
    }

    // 创建句柄
    int nRet = MV_CC_CreateHandle(&m_handle, &devInfo);
    if (MV_OK != nRet)
    {
        std::cerr << "Create handle failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
        return false;
    }

    // 打开设备
    nRet = MV_CC_OpenDevice(m_handle);
    if (MV_OK != nRet)
    {
        std::cerr << "Open device failed. Error: 0x" << std::hex << nRet << std::dec << std::endl;
        MV_CC_DestroyHandle(m_handle);
        m_handle = nullptr;
        return false;
    }

    m_isOpened = true;

    setCameraParameters(1280, 720, 30.0f, 5000.0f, 16.0f);

    // 设置最佳包大小（仅 GigE）
    if (devInfo.nTLayerType == MV_GIGE_DEVICE)
    {
        int nPacketSize = MV_CC_GetOptimalPacketSize(m_handle);
        if (nPacketSize > 0)
        {
            MV_CC_SetIntValue(m_handle, "GevSCPSPacketSize", nPacketSize);
        }
    }

    // 关闭触发模式
    MV_CC_SetEnumValue(m_handle, "TriggerMode", 0);

    // 获取 PayloadSize
    MVCC_INTVALUE stPayloadSize = {0};
    nRet = MV_CC_GetIntValue(m_handle, "PayloadSize", &stPayloadSize);
    if (MV_OK != nRet)
    {
        std::cerr << "Get PayloadSize failed." << std::endl;
        close();
        return false;
    }
    m_payloadSize = static_cast<unsigned int>(stPayloadSize.nCurValue);
    m_buffer.resize(m_payloadSize);

    // 开始取流
    nRet = MV_CC_StartGrabbing(m_handle);
    if (MV_OK != nRet)
    {
        std::cerr << "Start grabbing failed." << std::endl;
        close();
        return false;
    }

    return true;
}

void HikCamera::close()
{
    if (!m_isOpened)
        return;

    MV_CC_StopGrabbing(m_handle);
    MV_CC_CloseDevice(m_handle);
    MV_CC_DestroyHandle(m_handle);

    m_handle = nullptr;
    m_isOpened = false;
    m_buffer.clear();
    m_payloadSize = 0;
}

cv::Mat HikCamera::grabFrame(int timeoutMs)
{
    if (!m_isOpened || !m_handle)
    {
        return cv::Mat();
    }

    MV_FRAME_OUT_INFO_EX frameInfo = {0};
    int nRet = MV_CC_GetOneFrameTimeout(
        m_handle,
        m_buffer.data(),
        m_payloadSize,
        &frameInfo,
        timeoutMs);

    if (MV_OK != nRet)
    {
        std::cerr << "Grab frame timeout or error: 0x" << std::hex << nRet << std::dec << std::endl;
        return cv::Mat();
    }

    return convertToMat(frameInfo, m_buffer.data());
}

cv::Mat HikCamera::convertToMat(const MV_FRAME_OUT_INFO_EX &frameInfo, unsigned char *pData)
{
    if (!pData)
        return cv::Mat();

    if (frameInfo.enPixelType == 17301513) // 或者使用 PixelType_Gvsp_BayerRG8
    {
        // Bayer RG8 转 BGR
        cv::Mat bayer(frameInfo.nHeight, frameInfo.nWidth, CV_8UC1, pData);
        cv::Mat bgr;
        cv::cvtColor(bayer, bgr, cv::COLOR_BayerRG2BGR);
        return bgr.clone(); // 使用 clone() 确保数据独立
    }
    else
    {
        std::cout << "Unsupported pixel format: " << frameInfo.enPixelType
                  << ". Only BayerRG8 (17301513) is supported." << std::endl;
        return cv::Mat();
    }
}