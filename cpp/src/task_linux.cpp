// Lindows 平台的任务处理代码

#ifdef __linux__

#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"

namespace PaddleOCR {
    // ==================== 类的实现 ====================

    // 代替 cv::imread ，从路径pathW读入一张图片。pathW必须为unicode的wstring
    cv::Mat Task::imread_wstr(std::wstring pathW, std::string *pathU8p, int flag) {
        std::string pathU8;
        if (pathU8p == nullptr) {
            pathU8 = msg_wstr_2_ustr(pathW); // 再转回utf-8，以备输出错误。
            // ↑ 由于这个函数要被剪贴板CF_UNICODETEXT等复用，可能调用方只能提供wstring，所以多此一举转换一次。
        } else {
            pathU8 = *pathU8p;
        }
        if (!is_exists_wstr(pathW)) {
            // 路径不存在
            set_state(CODE_ERR_PATH_EXIST, MSG_ERR_PATH_EXIST(pathU8)); // 报告状态：路径不存在且无法输出
            return {};
        }
        FILE *fp = fopen(pathU8.c_str(), "rb");
        if (!fp) {
            // 打开失败
            set_state(CODE_ERR_PATH_READ, MSG_ERR_PATH_READ(pathU8)); // 报告状态：无法读取
            return {};
        }
        // 将文件读取到内存
        fseek(fp, 0, SEEK_END); // 设置流 fp 的文件位置为 SEEK_END 文件的末尾
        long sz = ftell(fp); // 获取流 fp 的当前文件位置，即总大小（字节）
        char *buf = new char[sz]; // 存放文件字节内容
        fseek(fp, 0, SEEK_SET); // 设置流 fp 的文件位置为 SEEK_SET 文件的开头
        long n = fread(buf, 1, sz, fp); // 从给定流 fp 读取数据到 buf 所指向的数组中，返回成功读取的元素总数
        cv::_InputArray arr(buf, sz); // 转换为OpenCV数组
        cv::Mat img = cv::imdecode(arr, flag); // 解码内存数据，变成cv::Mat数据
        delete[] buf; // 释放buf空间
        fclose(fp); // 关闭文件
        if (img.empty()) {
            set_state(CODE_ERR_PATH_DECODE, MSG_ERR_PATH_DECODE(pathU8)); // 报告状态：解码失败
        }
        return img;
    }

    // 从剪贴板读入一张图片，返回Mat。注意flag对剪贴板内存图片无效，仅对剪贴板路径图片有效。
    cv::Mat Task::imread_clipboard(int flag) {
        return {};
    }

    int Task::socket_mode() {
        return 0;
    }
}

#endif