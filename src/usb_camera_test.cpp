#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 尝试打开摄像头 (通常索引是 0)
    cv::VideoCapture cap(0);

    // 检查摄像头是否成功打开
    if (!cap.isOpened()) {
        std::cerr << "错误：无法打开摄像头!" << std::endl;
        return -1;
    }

    std::cout << "摄像头已打开。按 'q' 键退出。" << std::endl;

    cv::Mat frame;
    while (true) {
        // 从摄像头捕获一帧
        cap >> frame;

        // 检查帧是否为空
        if (frame.empty()) {
            std::cerr << "错误：无法获取帧!" << std::endl;
            break; // 或者 continue 尝试下一次
        }

        // 在窗口中显示帧
        cv::imshow("摄像头测试", frame);

        // 等待按键，延迟 1ms (使 imshow 生效)
        // 如果按下 'q' 键，则退出循环
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    // 释放摄像头和关闭所有 OpenCV 窗口
    cap.release();
    cv::destroyAllWindows();

    std::cout << "测试完成。" << std::endl;
    return 0;
}



