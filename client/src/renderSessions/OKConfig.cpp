//--------------------------------------------------------------------------------------
// Copyright (c) 2024 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "defines.h"

#include "OKConfig.h"

#include "jsoncpp/json/json.h"
#include "jsoncpp/json_value.cpp"
#include "jsoncpp/json_reader.cpp"
#include "jsoncpp/json_writer.cpp"

namespace BVR 
{

bool read_file(const std::string& filename, std::string& file_contents)
{
    FILE* file = fopen(filename.c_str(), "r");

    if (file == NULL)
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    if ((int)file_size < 1)
    {
        return false;
    }

    char* buffer = (char*)malloc(file_size);
    size_t bytes_read = fread(buffer, 1, file_size, file);

    if (bytes_read != file_size)
    {
        free(buffer);
        fclose(file);
        return false;
    }

    file_contents = buffer;

    free(buffer);
    fclose(file);
    return true;
}

static const OKConfig default_config;

OKConfig::OKConfig()
{
}

void OKConfig::reset()
{
    *this = default_config;
}

bool OKConfig::load()
{
    std::string ok_config_json;
    const bool read_ok = read_file(json_fullpath_, ok_config_json);

    if (!read_ok || ok_config_json.empty())
    {
        return false;
    }

    IGLLog(IGLLogLevel::LOG_INFO, "OKConfig::load() - Found JSON config file:\n\n%s", ok_config_json.c_str());

    JSONCPP_STRING err;
    Json::Value root;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    size_t str_size = ok_config_json.size();

    if (!reader->parse(ok_config_json.c_str(), ok_config_json.c_str() + str_size, &root, &err))
    {
        return false;
    }

    return true;
}

bool OKConfig::save()
{
	return true;
}


} // namespace BVR

