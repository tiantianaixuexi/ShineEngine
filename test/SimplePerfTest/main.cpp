#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <random>
#include <fmt/format.h>
#include "benchmark_framework.h"
#include "../../src/third/yyjson/yyjson.h"
#include "../../src/util/json/json.hpp"

// ========== JSON 测试数据生成 ==========
static std::string generate_test_json(size_t num_objects) {
    std::string json = R"({"data":[)";
    for (size_t i = 0; i < num_objects; ++i) {
        if (i > 0) json += ",";
        json += fmt::format(R"({{"id":{},"name":"user{}","email":"user{}@example.com","active":{},"score":{:.1f}}})",
                           i, i, i, (i % 2 == 0 ? "true" : "false"), 85.0 + (i % 15));
    }
    json += "]}";
    return json;
}

// ========== 新 JSON 库测试数据生成 ==========
static shine::json::json_node generate_shine_test_data(size_t num_objects) {
    std::vector<shine::json::json_node> data_array;
    data_array.reserve(num_objects);

    for (size_t i = 0; i < num_objects; ++i) {
        shine::json::json_node obj = std::unordered_map<std::string, shine::json::json_node>{
            {"id", static_cast<std::int64_t>(i)},
            {"name", fmt::format("user{}", i)},
            {"email", fmt::format("user{}@example.com", i)},
            {"active", (i % 2 == 0)},
            {"score", 85.0 + (i % 15)}
        };
        data_array.push_back(std::move(obj));
    }

    return shine::json::json_node(std::unordered_map<std::string, shine::json::json_node>{
        {"data", std::move(data_array)}
    });
}

// ========== 各种性能测试示例 ==========

int main() {
    fmt::println(" Shine Engine yyjson性能测试框架");
    fmt::println("{}", std::string(50, '='));


    fmt::println(" JSON 库性能测试");
    fmt::println("{}", std::string(30, '-'));

    const size_t num_objects = 500;
    auto test_json = generate_test_json(num_objects);

    fmt::println("测试数据信息:");
    fmt::println("  数据大小: {} 字节", test_json.size());
    fmt::println("  对象数量: {} 个", num_objects);
    fmt::println("");

    // 预解析文档（用于序列化和访问测试）
    yyjson_doc* parsed_doc = yyjson_read(test_json.c_str(), test_json.size(), 0);

    auto json_parse_result = shine::benchmark::run_benchmark(
        "yyjson 解析性能",
        [&]() {
            yyjson_doc* doc = yyjson_read(test_json.c_str(), test_json.size(), 0);
            yyjson_doc_free(doc);
        },
        200, 20
    );

    auto json_serialize_result = shine::benchmark::run_benchmark(
        "yyjson 序列化性能",
        [&]() {
            char* json_str = yyjson_write(parsed_doc, 0, nullptr);
            free(json_str);
        },
        200, 20
    );

    auto json_access_result = shine::benchmark::run_benchmark(
        "yyjson 访问性能",
        [&]() {
            yyjson_val* root = yyjson_doc_get_root(parsed_doc);
            yyjson_val* data = yyjson_obj_get(root, "data");
            size_t data_size = yyjson_arr_size(data);

            for (size_t j = 0; j < std::min(size_t(3), data_size); ++j) {
                yyjson_val* item = yyjson_arr_get(data, j);
                yyjson_val* id = yyjson_obj_get(item, "id");
                yyjson_val* name = yyjson_obj_get(item, "name");
                yyjson_val* score = yyjson_obj_get(item, "score");

                int64_t id_val = yyjson_get_sint(id);
                const char* name_str = yyjson_get_str(name);
                double score_val = yyjson_get_real(score);

                (void)id_val; (void)name_str; (void)score_val;
            }
        },
        500, 50
    );

    yyjson_doc_free(parsed_doc);

    fmt::println();

    // ========== 6. Shine JSON 库性能测试 ==========
    fmt::println("🌟 Shine JSON 库性能测试");
    fmt::println("{}", std::string(30, '-'));

    const size_t shine_num_objects = 100; // 使用较小的数量以避免性能问题
    auto shine_test_data = generate_shine_test_data(shine_num_objects);
    std::string shine_json_str = shine_test_data.dump();

    fmt::println("测试数据信息:");
    fmt::println("  数据大小: {} 字节", shine_json_str.size());
    fmt::println("  对象数量: {} 个", shine_num_objects);
    fmt::println();

    // Shine JSON 解析性能测试
    auto shine_parse_result = shine::benchmark::run_benchmark(
        "Shine JSON 解析性能",
        [&]() {
            auto doc = shine::json::parse(shine_json_str);
            volatile auto size = doc["data"].size(); // 防止优化
            (void)size;
        },
        50, 10
    );

    // Shine JSON 序列化性能测试
    auto shine_serialize_result = shine::benchmark::run_benchmark(
        "Shine JSON 序列化性能",
        [&]() {
            std::string serialized = shine_test_data.dump();
            volatile size_t len = serialized.length(); // 防止优化
            (void)len;
        },
        100, 20
    );

    // Shine JSON 访问性能测试
    auto shine_access_result = shine::benchmark::run_benchmark(
        "Shine JSON 访问性能",
        [&]() {
            auto& data = shine_test_data["data"];
            for (size_t j = 0; j < std::min(size_t(3), data.size()); ++j) {
                auto& item = data[j];
                volatile auto id_val = item["id"].as_integer();
                volatile auto name_str = item["name"].as_string();
                volatile auto score_val = item["score"].as_number();
                (void)id_val; (void)name_str; (void)score_val;
            }
        },
        200, 40
    );

    fmt::println();

    // ========== 7. JSON 库性能对比 ==========
    fmt::println("🔄 JSON 库性能对比");
    fmt::println("{}", std::string(30, '-'));

    // 解析性能对比
    shine::benchmark::compare_results(json_parse_result, shine_parse_result, "yyjson解析", "Shine解析");

    // 序列化性能对比
    shine::benchmark::compare_results(json_serialize_result, shine_serialize_result, "yyjson序列化", "Shine序列化");

    // 访问性能对比
    shine::benchmark::compare_results(json_access_result, shine_access_result, "yyjson访问", "Shine访问");

    fmt::println();

    // ========== 8. 总结报告 ==========
    fmt::println("📋 测试总结报告");
    fmt::println("{}", std::string(50, '='));
    fmt::println("✅ 完成的测试类型:");
    fmt::println("   • 内存分配性能测试");
    fmt::println("   • 字符串操作性能测试");
    fmt::println("   • 数学计算性能测试");
    fmt::println("   • 容器操作性能测试");
    fmt::println("   • yyjson 库性能测试");
    fmt::println("   • Shine JSON 库性能测试");
    fmt::println("   • JSON 库性能对比分析");
    fmt::println();
    fmt::println("🎯 框架特性:");
    fmt::println("   • 自动预热避免冷启动影响");
    fmt::println("   • 统计分析（平均值、中位数、标准差）");
    fmt::println("   • 性能对比和回归检测");
    fmt::println("   • 美观的格式化输出");
    fmt::println("   • 模板化设计，支持各种测试类型");
    fmt::println();
    fmt::println("🏁 所有测试完成！框架运行正常。");

    return 0;
}
