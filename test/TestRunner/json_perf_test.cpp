#include "framework/perf_framework.h"
#include "framework/test_macros.h"
#include <shine/json/json.hpp>
#include <yyjson.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// Test data
static const std::string simple_json = R"(
{
    "name": "John Doe",
    "age": 30,
    "city": "New York",
    "is_active": true,
    "scores": [85.5, 92.0, 78.5]
}
)";

static const std::string complex_json = R"(
{
    "users": [
        {
            "id": 1,
            "name": "Alice Johnson",
            "email": "alice@example.com",
            "profile": {
                "age": 28,
                "city": "San Francisco",
                "hobbies": ["reading", "coding", "hiking"],
                "active": true
            },
            "posts": [
                {"title": "Hello World", "likes": 42},
                {"title": "My Journey", "likes": 31}
            ]
        },
        {
            "id": 2,
            "name": "Bob Smith",
            "email": "bob@example.com",
            "profile": {
                "age": 35,
                "city": "Chicago",
                "hobbies": ["gaming", "photography"],
                "active": false
            },
            "posts": [
                {"title": "Tech News", "likes": 156},
                {"title": "Weekend Plans", "likes": 23}
            ]
        }
    ],
    "metadata": {
        "version": "1.0",
        "timestamp": 1640995200,
        "settings": {
            "theme": "dark",
            "notifications": true,
            "privacy": {
                "public_profile": false,
                "show_email": false
            }
        }
    }
}
)";

// Generate large JSON for performance testing
static std::string generate_large_json(size_t num_objects) {
    std::ostringstream oss;
    oss << "{\n\"data\": [";
    for (size_t i = 0; i < num_objects; ++i) {
        if (i > 0) oss << ",";
        oss << "\n  {\n"
            << "    \"id\": " << i << ",\n"
            << "    \"name\": \"User" << i << "\",\n"
            << "    \"email\": \"user" << i << "@example.com\",\n"
            << "    \"active\": " << (i % 2 == 0 ? "true" : "false") << ",\n"
            << "    \"score\": " << (85.0 + (i % 15)) << ",\n"
            << "    \"tags\": [\"tag" << (i % 5) << "\", \"tag" << (i % 3) << "\"]\n"
            << "  }";
    }
    oss << "\n]\n}";
    return oss.str();
}

static std::string medium_json = generate_large_json(100);

// ==========================================
// Shine JSON Library Benchmarks
// ==========================================

SHINE_PERF_TEST(ShineJSON, Parse_Simple) {
    while (state.keep_running()) {
        auto doc = shine::json::parse(simple_json);
        (void)doc; // Prevent optimization
    }
}

SHINE_PERF_TEST(ShineJSON, Parse_Complex) {
    while (state.keep_running()) {
        auto doc = shine::json::parse(complex_json);
        (void)doc;
    }
}

SHINE_PERF_TEST(ShineJSON, Parse_Medium) {
    while (state.keep_running()) {
        auto doc = shine::json::parse(medium_json);
        (void)doc;
    }
}

SHINE_PERF_TEST(ShineJSON, Serialize_Simple) {
    auto doc = shine::json::parse(simple_json);
    while (state.keep_running()) {
        auto str = doc.root().dump();
        (void)str;
    }
}

SHINE_PERF_TEST(ShineJSON, Serialize_Complex) {
    auto doc = shine::json::parse(complex_json);
    while (state.keep_running()) {
        auto str = doc.root().dump();
        (void)str;
    }
}

SHINE_PERF_TEST(ShineJSON, Serialize_Medium) {
    auto doc = shine::json::parse(medium_json);
    while (state.keep_running()) {
        auto str = doc.root().dump();
        (void)str;
    }
}

SHINE_PERF_TEST(ShineJSON, Access_Simple) {
    auto doc = shine::json::parse(simple_json);
    while (state.keep_running()) {
        auto name = doc.root()["name"].as_string();
        auto age = doc.root()["age"].as_integer();
        auto city = doc.root()["city"].as_string();
        auto active = doc.root()["is_active"].as_boolean();
        auto scores = doc.root()["scores"].as_array();
        (void)name; (void)age; (void)city; (void)active; (void)scores;
    }
}

SHINE_PERF_TEST(ShineJSON, Access_Complex) {
    auto doc = shine::json::parse(complex_json);
    while (state.keep_running()) {
        auto users = doc.root()["users"].as_array();
        for (const auto& user : users) {
            auto id = user["id"].as_integer();
            auto name = user["name"].as_string();
            auto email = user["email"].as_string();
            auto profile = user["profile"];
            auto age = profile["age"].as_integer();
            auto city = profile["city"].as_string();
            (void)id; (void)name; (void)email; (void)age; (void)city;
        }
    }
}

SHINE_PERF_TEST(ShineJSON, Iterate_Array) {
    auto doc = shine::json::parse(large_json);
    while (state.keep_running()) {
        size_t count = 0;
        for (const auto& item : doc.root()["data"].array_elements()) {
            count++;
        }
        (void)count;
    }
}

SHINE_PERF_TEST(ShineJSON, Iterate_Object) {
    auto doc = shine::json::parse(complex_json);
    while (state.keep_running()) {
        size_t count = 0;
        for (const auto& [key, value] : doc.root().object_members()) {
            count++;
        }
        (void)count;
    }
}

// ==========================================
// yyjson Library Benchmarks
// ==========================================

SHINE_PERF_TEST(yyjson, Parse_Simple) {
    while (state.keep_running()) {
        yyjson_doc* doc = yyjson_read(simple_json.c_str(), simple_json.size(), 0);
        yyjson_doc_free(doc);
    }
}

SHINE_PERF_TEST(yyjson, Parse_Complex) {
    while (state.keep_running()) {
        yyjson_doc* doc = yyjson_read(complex_json.c_str(), complex_json.size(), 0);
        yyjson_doc_free(doc);
    }
}

SHINE_PERF_TEST(yyjson, Parse_Medium) {
    while (state.keep_running()) {
        yyjson_doc* doc = yyjson_read(medium_json.c_str(), medium_json.size(), 0);
        yyjson_doc_free(doc);
    }
}

SHINE_PERF_TEST(yyjson, Serialize_Simple) {
    yyjson_doc* doc = yyjson_read(simple_json.c_str(), simple_json.size(), 0);
    while (state.keep_running()) {
        char* json_str = yyjson_write(doc, 0, nullptr);
        free(json_str);
    }
    yyjson_doc_free(doc);
}

SHINE_PERF_TEST(yyjson, Serialize_Complex) {
    yyjson_doc* doc = yyjson_read(complex_json.c_str(), complex_json.size(), 0);
    while (state.keep_running()) {
        char* json_str = yyjson_write(doc, 0, nullptr);
        free(json_str);
    }
    yyjson_doc_free(doc);
}

SHINE_PERF_TEST(yyjson, Serialize_Medium) {
    yyjson_doc* doc = yyjson_read(medium_json.c_str(), medium_json.size(), 0);
    while (state.keep_running()) {
        char* json_str = yyjson_write(doc, 0, nullptr);
        free(json_str);
    }
    yyjson_doc_free(doc);
}

SHINE_PERF_TEST(yyjson, Access_Simple) {
    yyjson_doc* doc = yyjson_read(simple_json.c_str(), simple_json.size(), 0);
    yyjson_val* root = yyjson_doc_get_root(doc);
    while (state.keep_running()) {
        yyjson_val* name = yyjson_obj_get(root, "name");
        yyjson_val* age = yyjson_obj_get(root, "age");
        yyjson_val* city = yyjson_obj_get(root, "city");
        yyjson_val* active = yyjson_obj_get(root, "is_active");
        yyjson_val* scores = yyjson_obj_get(root, "scores");

        const char* name_str = yyjson_get_str(name);
        int64_t age_val = yyjson_get_sint(age);
        const char* city_str = yyjson_get_str(city);
        bool active_val = yyjson_get_bool(active);
        size_t scores_size = yyjson_arr_size(scores);

        (void)name_str; (void)age_val; (void)city_str; (void)active_val; (void)scores_size;
    }
    yyjson_doc_free(doc);
}

SHINE_PERF_TEST(yyjson, Access_Complex) {
    yyjson_doc* doc = yyjson_read(complex_json.c_str(), complex_json.size(), 0);
    yyjson_val* root = yyjson_doc_get_root(doc);
    yyjson_val* users = yyjson_obj_get(root, "users");
    while (state.keep_running()) {
        size_t users_size = yyjson_arr_size(users);
        for (size_t i = 0; i < users_size; ++i) {
            yyjson_val* user = yyjson_arr_get(users, i);
            yyjson_val* id = yyjson_obj_get(user, "id");
            yyjson_val* name = yyjson_obj_get(user, "name");
            yyjson_val* email = yyjson_obj_get(user, "email");
            yyjson_val* profile = yyjson_obj_get(user, "profile");
            yyjson_val* age = yyjson_obj_get(profile, "age");
            yyjson_val* city = yyjson_obj_get(profile, "city");

            int64_t id_val = yyjson_get_sint(id);
            const char* name_str = yyjson_get_str(name);
            const char* email_str = yyjson_get_str(email);
            int64_t age_val = yyjson_get_sint(age);
            const char* city_str = yyjson_get_str(city);

            (void)id_val; (void)name_str; (void)email_str; (void)age_val; (void)city_str;
        }
    }
    yyjson_doc_free(doc);
}

SHINE_PERF_TEST(yyjson, Iterate_Array) {
    yyjson_doc* doc = yyjson_read(medium_json.c_str(), medium_json.size(), 0);
    yyjson_val* root = yyjson_doc_get_root(doc);
    yyjson_val* data = yyjson_obj_get(root, "data");
    while (state.keep_running()) {
        size_t count = 0;
        yyjson_arr_iter iter;
        yyjson_arr_iter_init(data, &iter);
        yyjson_val* val;
        while ((val = yyjson_arr_iter_next(&iter))) {
            count++;
        }
        (void)count;
    }
    yyjson_doc_free(doc);
}

SHINE_PERF_TEST(yyjson, Iterate_Object) {
    yyjson_doc* doc = yyjson_read(complex_json.c_str(), complex_json.size(), 0);
    yyjson_val* root = yyjson_doc_get_root(doc);
    while (state.keep_running()) {
        size_t count = 0;
        yyjson_obj_iter iter;
        yyjson_obj_iter_init(root, &iter);
        yyjson_val *key, *val;
        while ((key = yyjson_obj_iter_next(&iter))) {
            val = yyjson_obj_iter_get_val(key);
            count++;
        }
        (void)count;
    }
    yyjson_doc_free(doc);
}

// ==========================================
// Unit Tests for Correctness
// ==========================================

SHINE_TEST(JSON_Correctness, Shine_Parse_Simple) {
    auto doc = shine::json::parse(simple_json);
    const auto& root = doc.root();

    EXPECT_EQ(root["name"].as_string(), "John Doe");
    EXPECT_EQ(root["age"].as_integer(), 30);
    EXPECT_EQ(root["city"].as_string(), "New York");
    EXPECT_EQ(root["is_active"].as_boolean(), true);

    const auto& scores = root["scores"].as_array();
    EXPECT_EQ(scores.size(), 3);
    EXPECT_EQ(scores[0].as_number(), 85.5);
    EXPECT_EQ(scores[1].as_number(), 92.0);
    EXPECT_EQ(scores[2].as_number(), 78.5);
}

SHINE_TEST(JSON_Correctness, yyjson_Parse_Simple) {
    yyjson_doc* doc = yyjson_read(simple_json.c_str(), simple_json.size(), 0);
    EXPECT_TRUE(doc != nullptr);

    yyjson_val* root = yyjson_doc_get_root(doc);
    EXPECT_TRUE(root != nullptr);

    yyjson_val* name = yyjson_obj_get(root, "name");
    EXPECT_TRUE(name != nullptr);
    EXPECT_EQ(std::string(yyjson_get_str(name)), "John Doe");

    yyjson_val* age = yyjson_obj_get(root, "age");
    EXPECT_TRUE(age != nullptr);
    EXPECT_EQ(yyjson_get_sint(age), 30);

    yyjson_doc_free(doc);
}

SHINE_TEST(JSON_Correctness, Roundtrip_Test) {
    // Parse with Shine JSON, serialize, then parse with yyjson
    auto shine_doc = shine::json::parse(simple_json);
    std::string serialized = shine_doc.root().dump();

    yyjson_doc* yyjson_doc = yyjson_read(serialized.c_str(), serialized.size(), 0);
    EXPECT_TRUE(yyjson_doc != nullptr);

    yyjson_val* root = yyjson_doc_get_root(yyjson_doc);
    yyjson_val* name = yyjson_obj_get(root, "name");
    EXPECT_EQ(std::string(yyjson_get_str(name)), "John Doe");

    yyjson_doc_free(yyjson_doc);
}

SHINE_TEST(JSON_Correctness, Complex_Data_Access) {
    auto doc = shine::json::parse(complex_json);
    const auto& root = doc.root();

    const auto& users = root["users"].as_array();
    EXPECT_EQ(users.size(), 2);

    // First user
    const auto& user1 = users[0];
    EXPECT_EQ(user1["id"].as_integer(), 1);
    EXPECT_EQ(user1["name"].as_string(), "Alice Johnson");
    EXPECT_EQ(user1["profile"]["age"].as_integer(), 28);
    EXPECT_EQ(user1["profile"]["city"].as_string(), "San Francisco");

    // Second user
    const auto& user2 = users[1];
    EXPECT_EQ(user2["id"].as_integer(), 2);
    EXPECT_EQ(user2["name"].as_string(), "Bob Smith");
    EXPECT_EQ(user2["profile"]["age"].as_integer(), 35);
    EXPECT_EQ(user2["profile"]["active"].as_boolean(), false);
}
