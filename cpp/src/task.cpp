
#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"
#include "include/nlohmann/json.hpp" // json库

namespace PaddleOCR
{
    // ==================== 工具 ====================

    // json对象转字符串
    std::string json_dump(nlohmann::json j)
    {
        try
        {
            std::string str = j.dump(-1, ' ', FLAGS_ensure_ascii);
            return str;
        }
        catch (...)
        {
            nlohmann::json j2;
            j2["code"] = CODE_ERR_JSON_DUMP;
            j2["data"] = MSG_ERR_JSON_DUMP;
            std::string str = j2.dump(-1, ' ', FLAGS_ensure_ascii);
            return str;
        }
    }

    // 设置状态
    void Task::set_state(int code, std::string msg)
    {
        t_code = code;
        t_msg = msg;
    }

    // 获取状态json字符串
    std::string Task::get_state_json(int code, std::string msg)
    {
        nlohmann::json j;
        if (code == CODE_INIT && msg.empty())
        { // 留空，填充当前状态
            code = t_code;
            msg = t_msg;
        }
        j["code"] = code;
        j["data"] = msg;
        return json_dump(j);
    }
    
    // 将OCR结果转换为json字符串  
    std::string Task::get_ocr_result_json(const std::vector<OCRPredictResult> &ocr_result)
    {
        nlohmann::json outJ;
        outJ["code"] = 100;
        outJ["data"] = nlohmann::json::array();
        bool isEmpty = true;
        for (int i = 0; i < ocr_result.size(); i++)
        {
            nlohmann::json j;
            j["text"] = ocr_result[i].text;
            j["score"] = ocr_result[i].score;
            std::vector<std::vector<int>> b = ocr_result[i].box;
            // 无包围盒
            if (b.empty())
            {
                if (FLAGS_det) // 开了det仍无包围盒，跳过本组 
                    continue;
                else // 未开det，填充空包围盒 
                    for (int bi = 0; bi < 4; bi++)
                        b.push_back(std::vector<int>{-1, -1});
            }
            // 启用了rec仍没有文字，跳过本组 
            if (FLAGS_rec && (j["score"] <= 0 || j["text"] == ""))
            {
                continue;
            }
            else
            {
                j["box"] = {{b[0][0], b[0][1]}, {b[1][0], b[1][1]}, {b[2][0], b[2][1]}, {b[3][0], b[3][1]}};
            }
            outJ["data"].push_back(j);
            isEmpty = false;
        }
        // 结果1：识别成功，无文字（rec未检出） 
        if (isEmpty)
        {
            return "";
        }
        // 结果2：识别成功，有文字 
        return json_dump(outJ);
    }

    // 输入json字符串，解析并读取Mat 
    cv::Mat Task::imread_json(std::string& str_in) {
        cv::Mat img;
        bool is_image = false; // 当前是否已找到图片 
        std::string logstr = "";
        try {
            auto j = nlohmann::json::parse(str_in); // 转json对象 
            for (auto& el : j.items()) { // 遍历键值 
                std::string value = to_string(el.value());
                int vallen = value.length();
                if (vallen > 2 && value[0] == '\"' && value[vallen - 1] == '\"') {
                    value = value.substr(1, vallen - 2); // 删去nlohmann字符串的两端引号
                }
                // 提取图片 
                if (!is_image) {
                    if (el.key() == "image_dir" || el.key() == "image_path") { // 图片路径
                        img = imread_u8(value); // 读取图片 
                        is_image = true;
                    }
                    // TODO base64
                }
                //else {} // TODO: 其它参数热更新
            }
        }
        catch (...) {
            set_state(CODE_ERR_JSON_PARSE, MSG_ERR_JSON_PARSE); // 报告状态：解析失败 
            return cv::Mat();
        }
        if (!is_image) {
            set_state(CODE_ERR_NO_TASK, MSG_ERR_NO_TASK); // 报告状态：未发现有效任务 
        }
        return img;
    }

    // ==================== 任务流程 ====================

    // 入口
    int Task::ocr()
    {
        // 初始化引擎
        ppocr = new PPOCR(); // 创建引擎对象
        std::cout << "OCR init completed." << std::endl;

        // 单张图片识别模式
        if (!FLAGS_image_path.empty())
        {
            return single_image_mode();
        }
        //// 套接字服务器模式
        // else if (FLAGS_port != -1) {
        //	return socket_mode();
        // }
        // 匿名管道模式
        else {
            return anonymous_pipe_mode();
        }

        return 0;
    }

    // 单张图片识别模式 
    int Task::single_image_mode()
    {
        set_state();
        cv::Mat img = imread_u8(FLAGS_image_path);
        if (img.empty())
        { // 读图失败 
            std::cout << get_state_json() << std::endl;
            return 0;
        }
        // 执行OCR 
        std::vector<OCRPredictResult> res_ocr = ppocr->ocr(img, FLAGS_det, FLAGS_rec, FLAGS_cls);
        // 获取结果 
        std::string res_json = get_ocr_result_json(res_ocr);
        // 结果1：识别成功，无文字（rec未检出）
        if (res_json.empty()) {
            std::cout << get_state_json(CODE_OK_NONE, MSG_OK_NONE(FLAGS_image_path)) << std::endl;
        }
        // 结果2：识别成功，有文字 
        else {
            std::cout << res_json << std::endl;
        }
        return 0;
    }

    // 匿名管道模式 
    int Task::anonymous_pipe_mode() {
        while (1) {
            set_state();
            // 读取一行输入 
            std::string str_in;
            getline(std::cin, str_in);
            cv::Mat img = imread_json(str_in);
            if (img.empty())
            { // 读图失败 
                std::cout << get_state_json() << std::endl;
                continue;
            }
            // 执行OCR 
            std::vector<OCRPredictResult> res_ocr = ppocr->ocr(img, FLAGS_det, FLAGS_rec, FLAGS_cls);
            // 获取结果 
            std::string res_json = get_ocr_result_json(res_ocr);
            // 结果1：识别成功，无文字（rec未检出）
            if (res_json.empty()) {
                std::cout << get_state_json(CODE_OK_NONE, MSG_OK_NONE(FLAGS_image_path)) << std::endl;
            }
            // 结果2：识别成功，有文字 
            else {
                std::cout << res_json << std::endl;
            }
        }
        return 0;
    }
}

// ./PaddleOCR-json.exe -config_path="models/zh_CN.txt" -image_path="D:/Test/t2.png" -ensure_ascii=0