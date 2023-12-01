// 通用平台的任务处理代码

#include "filesystem"
#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"

// 编码转换
#include <codecvt>

std::wstring_convert<std::codecvt_utf8<wchar_t>> conv_Ustr_Wstr; // string utf-8 与 wstring utf-16 的双向转换器

namespace PaddleOCR {
    // ==================== 工具函数 ====================

    // 专门用于消息的wstring转string，转换失败时返回默认提示文字
    std::string msg_wstr_2_ustr(std::wstring &msg)
    {
        try
        {
            std::string msgU8 = conv_Ustr_Wstr.to_bytes(msg); // 转回u8
            return msgU8;
        }
        catch (...)
        {
            return "wstring failed to convert to utf-8 string";
        }
    }

    // 检查路径pathW是否为文件，是返回true
    bool is_exists_wstr(std::wstring pathW)
    {
        return std::filesystem::exists(pathW);
    }
    // ==================== 类的实现 ====================

    // 代替cv imread，接收utf-8字符串传入，返回Mat。
    cv::Mat Task::imread_u8(std::string pathU8, int flag) {
        // todo 若为剪贴板任务
        // string u8 转 wchar_t
        std::wstring  wpath;
        try {
            wpath = conv_Ustr_Wstr.from_bytes(pathU8);
        } catch (...) {
            set_state(CODE_ERR_PATH_CONV, MSG_ERR_PATH_CONV(pathU8));
            return {};
        }
        return imread_wstr(wpath, nullptr, 0);
    }

}
