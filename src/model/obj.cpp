#include "obj.h"

#include <string>
#include <expected>
#include <string_view>



#include "fmt/format.h"
#include "util/file_util.h"
#include "util/timer/function_timer.h"

namespace shine::model
{

    std::expected<ObjMesh*,std::string> obj::parseObjFile(std::string_view filePath)
    {

        util::FunctionTimer ___timer___("解析Obj的时间");

        // 打开文件映射
        auto openResult = util::open_file_from_mapping(filePath.data());
        if (!openResult.has_value())
        {
            return std::unexpected(openResult.error());
        }

        // 获取Map和文件大小
        auto file_mapping = std::move(openResult.value());
        auto file_size    = util::get_file_size(file_mapping);

        fmt::println("file_size:{}",file_size.value());

        if (!file_size.has_value())
        {
            return std::unexpected("get file size error");
        }

        // 直接读取文件数据
        auto file_data = util::read_data_from_mapping(file_mapping,file_size.value(),0);
        if (!file_data.has_value())
        {
            return std::unexpected(file_data.error());
        }

        auto file_data_view = std::move(file_data.value());
        std::string_view data_view(
            reinterpret_cast<const char*>(file_data_view.content.data()),
            file_data_view.content.size()
        );

        mesh = new ObjMesh();
        mesh->positions.emplace_back(0.0f);
        mesh->positions.emplace_back(0.0f);
        mesh->positions.emplace_back(0.0f);

        mesh->texcoords.emplace_back(0.0f);
        mesh->texcoords.emplace_back(0.0f);

        mesh->normals.emplace_back(0.0f);
        mesh->normals.emplace_back(0.0f);
        mesh->normals.emplace_back(1.0f);

        mesh->textures.emplace_back(objTexture());

        data.mesh     = mesh;
        data.object   = {};
        data.group    = {};
        data.material = 0;
        data.line     = 1;
        data.base     = {};

        // 判断 \ , /  情况
        {
            size_t pos = filePath.find_last_of("\\");
            if(pos == std::string::npos)
            {
                pos = filePath.find_last_of("/");
                if(pos != std::string::npos)
                {
                    data.base = filePath.substr(0,pos+1);
                }

            }else{
                data.base = filePath.substr(0,pos+1);
            }
        }

        while(!data_view.empty())
        {
            size_t line_end = data_view.find('\n');
            const bool has_line_break = (line_end != std::string_view::npos);

            // 处理当前行（包括没有换行符的最后一行）
            std::string_view line = has_line_break
            ? data_view.substr(0, line_end)
            : data_view;

            if(line.empty())
            {
                data_view = data_view.substr(line_end+1);
                continue;
            }

            if(line.back() == '\r')
            {
                line.remove_suffix(1);
            }

            parseLine(line);

            if(has_line_break)
            {
                data_view = data_view.substr(line_end+1);
            }

        }

       // fmt::println("{}",std::to_string(std::stacktrace::current()));

        return mesh;
    }

    bool obj::isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\r';
    }

    bool obj::isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    void obj::parseLine(std::string_view line)
    {

        size_t pos = 0;

        if(isWhitespace(line[0]))
        {
            return;
        }

        switch(line[0])
        {
            case '#':
                fmt::println("{}",line.substr(2));
                return;
            case 'm':
                if(line.substr(0,7) == "mtllib ")
                {
                    parseMtlFile(line.substr(7));
                }
                return;
        }

    }

    void obj::parseMtlFile(std::string_view line)
    {

        std::string_view mtl_file = data.base.append(line);

        // 打开文件映射
        auto openResult = util::open_file_from_mapping(mtl_file.data());
        if (!openResult.has_value())
        {
            return;
        }

        // 获取Map和文件大小
        auto file_mapping = std::move(openResult.value());
        auto file_size    = util::get_file_size(file_mapping);

        fmt::println("mtl file_size:{}",file_size.value());

        if (!file_size.has_value())
        {
            return;
        }

        // 直接读取文件数据
        auto file_data = util::read_data_from_mapping(file_mapping,file_size.value(),0);
        if (!file_data.has_value())
        {
            return;
        }

        auto file_data_view = std::move(file_data.value());
        std::string_view data_view(
            reinterpret_cast<const char*>(file_data_view.content.data()),
            file_data_view.content.size()
        );

        while(!data_view.empty())
        {
            size_t line_end = data_view.find('\n');
            const bool has_line_break = (line_end != std::string_view::npos);

            std::string_view line = has_line_break
            ? data_view.substr(0, line_end)
            : data_view;

            if(isWhitespace(line[0]))
            {
                continue;
            }

            switch(line[0])
            {
                case '#':
                    line = line.substr(2);
                    fmt::println("mtl: {}",line);
                    break;

                case 'n':
                    if(line.substr(0,7) == "newmtl ")
                    {
                        line = line.substr(7);
                        fmt::println("mtl: {}",line);
                    }
                    break;

            }

            if(has_line_break)
            {
                data_view = data_view.substr(line_end+1);
            }
        }
    }
}


