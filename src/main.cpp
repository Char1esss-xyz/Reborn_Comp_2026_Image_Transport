#include <httplib.h>
#include "hik_camera.hpp" // 确保路径正确
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>
#include <sstream>

const std::string BOUNDARY = "frameboundary";
std::vector<unsigned char> latest_frame_data;
std::mutex frame_mutex;
std::atomic<bool> keep_running{true};

void capture_video(HikCamera &cam)
{
    std::cout << "[Capture Thread] 尝试打开海康相机..." << std::endl;
    if (!cam.open(0))
    {
        std::cerr << "[Capture Thread] 错误：无法打开海康相机！" << std::endl;
        exit(1);
    }
    std::cout << "[Capture Thread] 海康相机已成功打开。" << std::endl;

    cv::Mat frame;
    cv::Mat rgbFrame; // For storing the RGB version
    std::vector<unsigned char> buffer;
    int frame_count = 0;

    while (keep_running.load())
    {
        frame = cam.grabFrame(1000); // 1秒超时
        if (frame.empty())
        {
            std::cerr << "[Capture Thread] 警告：抓取帧为空，跳过..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        frame_count++;
        if (frame_count % 30 == 0)
        {
            std::cout << "[Capture Thread] 已捕获帧数: " << frame_count << std::endl;
        }

        // ✅ CRITICAL: Convert from BGR (OpenCV) to RGB (for browser)
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
        buffer.clear();
        if (!cv::imencode(".jpg", rgbFrame, buffer, params))
        {
            std::cerr << "[Capture Thread] 错误：JPEG 编码失败！" << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame_data = buffer;
        }

        // 控制帧率 (~30 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    cam.close();
    std::cout << "[Capture Thread] 视频捕获线程已停止。" << std::endl;
}

int main() {
    HikCamera cam;

    std::cout << "[Main] 启动视频捕获线程..." << std::endl;
    std::thread capture_thread(capture_video, std::ref(cam));

    httplib::Server svr;

    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[HTTP] 接收到根路径 '/' 请求，来自 " << req.remote_addr << std::endl;
        std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Hikvision Camera Stream</title>
</head>
<body>
    <h1>Hikvision Camera Live Stream</h1>
    <img src="/video_feed" width="640" height="480" />
</body>
</html>
)";
        res.set_content(html, "text/html");
    });

    svr.Get("/video_feed", [](const httplib::Request &req, httplib::Response &res)
    {
    std::cout << "[HTTP] 接收到视频流请求 '/video_feed'，来自 " << req.remote_addr << std::endl;
    
    // ✅ SET CONTENT-TYPE CORRECTLY (ONLY ONCE!)
    res.set_header("Content-Type", "multipart/x-mixed-replace; boundary=" + BOUNDARY);
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "close");
    res.set_header("Access-Control-Allow-Origin", "*");

    res.set_chunked_content_provider(
        "", 
        [](size_t /*offset*/, httplib::DataSink& sink) -> bool {
            while (keep_running.load()) {
                std::vector<unsigned char> frame;
                {
                    std::lock_guard<std::mutex> lock(frame_mutex);
                    if (latest_frame_data.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                    frame = latest_frame_data;
                }

                // ✅ CORRECT MULTIPART FORMAT (NO Content-Length!)
                std::ostringstream header;
                header << "--" << BOUNDARY << "\r\n"
                       << "Content-Type: image/jpeg\r\n"
                       << "\r\n"; // Critical: double CRLF to end headers

                if (!sink.write(header.str().c_str(), header.str().size()) ||
                    !sink.write(reinterpret_cast<const char*>(frame.data()), frame.size())) {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(33));
            }
            return false;
        },
        [](bool success) {
            std::cout << "[Stream] 视频流结束。成功: " << (success ? "是" : "否") << std::endl;
        }
    ); });

    std::cout << "[Main] 服务器启动在 http://0.0.0.0:8080" << std::endl;
    std::cout << "[Main] 请在浏览器中访问 http://localhost:8080 查看视频流" << std::endl;
    std::cout << "[Main] 按 Enter 键停止服务器..." << std::endl;

    std::thread server_thread([&svr]() {
        svr.listen("0.0.0.0", 8080);
    });

    // std::cin.get(); // 等待用户按回车

    // std::cout << "[Main] 正在停止..." << std::endl;
    // keep_running.store(false);
    // svr.stop();

    if (capture_thread.joinable()) capture_thread.join();
    if (server_thread.joinable()) server_thread.join();

    std::cout << "[Main] 程序已退出。" << std::endl;
    return 0;
}