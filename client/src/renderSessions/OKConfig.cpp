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

    char* buffer = (char*)malloc(file_size + 1);
    size_t bytes_read = fread(buffer, 1, file_size, file);

    if (bytes_read != file_size)
    {
        free(buffer);
        fclose(file);
        return false;
    }

    buffer[bytes_read] = 0;
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
        //IGLLog(IGLLogLevel::LOG_INFO, "OKConfig::load() - No config file found, using default...\n");
        return false;
    }

    //IGLLog(IGLLogLevel::LOG_INFO, "OKConfig::load() - Found JSON config file:\n\n%s", ok_config_json.c_str());

    JSONCPP_STRING err;
    Json::Value root;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    size_t str_size = ok_config_json.size();

    if (!reader->parse(ok_config_json.c_str(), ok_config_json.c_str() + str_size, &root, &err))
    {
        //IGLLog(IGLLogLevel::LOG_ERROR, "OKConfig::load() - Error parsing config file: %s\n", err.c_str());
        return false;
    }

    if (root.isMember("server_ip_address"))
    {
        const Json::Value server_ip_address_value = root["server_ip_address"];

        if (server_ip_address_value.isString())
        {
            server_ip_address_ = server_ip_address_value.asString();
        }
        else
        {
            return false;
        }

    }
    else
    {
        return false;
    }

    //IGLLog(IGLLogLevel::LOG_ERROR, "OKConfig::load() - Parsed successfully, server IP = %s\n", server_ip_address_.c_str());

    // All other parameters are optional.

    if (root.isMember("enable_auto_connect"))
    {
        const Json::Value value = root["enable_auto_connect"];
        enable_auto_connect_ = (bool)value.asUInt();
    }

    // Res has to be modulo 32 pixels for optimal image quality / scaling

    if (root.isMember("per_eye_width"))
    {
        const Json::Value value = root["per_eye_width"];

        if (value.isUInt())
        {
            per_eye_width_ = value.asUInt();
            per_eye_width_ += per_eye_width_ % 32;
        }
    }

    if (root.isMember("per_eye_height"))
    {
        const Json::Value value = root["per_eye_height"];

        if (value.isUInt())
        {
            per_eye_height_ = value.asUInt();
            per_eye_height_ += per_eye_height_ % 32;
        }
    }

    if (root.isMember("desired_refresh_rate"))
    {
        const Json::Value value = root["desired_refresh_rate"];

        if (value.isUInt())
        {
            desired_refresh_rate_ = value.asUInt();
        }
    }

    if (root.isMember("polling_rate_mult"))
    {
        const Json::Value value = root["polling_rate_mult"];

        if (value.isUInt())
        {
            polling_rate_mult_ = value.asUInt();
        }
    }

    if (root.isMember("foveation"))
    {
        const Json::Value value = root["foveation"];

        if (value.isUInt())
        {
            foveation_ = value.asUInt();
            foveation_ = clamp<uint32_t>(foveation_, 0, 100);
        }
    }

    if (root.isMember("enable_sharpening"))
    {
        const Json::Value value = root["enable_sharpening"];

        if (value.isUInt())
        {
            //enable_sharpening_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("max_res_factor"))
    {
        const Json::Value value = root["max_res_factor"];

        if (value.isDouble())
        {
            max_res_factor_ = value.asFloat();
        }
    }

    if (root.isMember("max_bitrate_kbps"))
    {
        const Json::Value value = root["max_bitrate_kbps"];

        if (value.isUInt())
        {
            //max_bitrate_kbps_ = value.asUInt();
        }
    }

    if (root.isMember("prediction_offset_ns"))
    {
        const Json::Value value = root["prediction_offset_ns"];

        if (value.isDouble())
        {
            prediction_offset_ns_ = value.asFloat();
        }
    }

    if (root.isMember("pose_time_offset_s"))
    {
        const Json::Value value = root["pose_time_offset_s"];

        if (value.isDouble())
        {
            pose_time_offset_s_ = value.asFloat();
        }
    }

    if (root.isMember("latch_timeout_ms"))
    {
        const Json::Value value = root["latch_timeout_ms"];

        if (value.isUInt())
        {
            latch_timeout_ms_ = value.asUInt();
        }
    }

    if (root.isMember("enable_audio_playback"))
    {
        const Json::Value value = root["enable_audio_playback"];

        if (value.isUInt())
        {
            //enable_audio_playback_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_audio_recording"))
    {
        const Json::Value value = root["enable_audio_recording"];

        if (value.isUInt())
        {
            //enable_audio_recording_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_eye_tracking"))
    {
        const Json::Value value = root["enable_eye_tracking"];

        if (value.isUInt())
        {
            enable_eye_tracking_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_face_tracking"))
    {
        const Json::Value value = root["enable_face_tracking"];

        if (value.isUInt())
        {
            enable_face_tracking_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_hand_tracking"))
    {
        const Json::Value value = root["enable_hand_tracking"];

        if (value.isUInt())
        {
            enable_hand_tracking_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_body_tracking"))
    {
        const Json::Value value = root["enable_body_tracking"];

        if (value.isUInt())
        {
            enable_body_tracking_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_waist_loco"))
    {
        const Json::Value value = root["enable_waist_loco"];

        if (value.isUInt())
        {
            enable_waist_loco_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_swap_thumbsticks"))
    {
        const Json::Value value = root["enable_swap_thumbsticks"];

        if (value.isUInt())
        {
            enable_swap_thumbsticks_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("enable_remote_controller_offset"))
    {
        const Json::Value value = root["enable_remote_controller_offset"];

        if (value.isUInt())
        {
            enable_remote_controller_offset_ = (bool)value.asUInt();
        }
    }

    if (root.isMember("remote_controller_offset"))
    {
        const Json::Value value = root["remote_controller_offset"];
        // Not implemented yet, just use the default hardcoded values for now...
    }

    return true;
}

bool OKConfig::save()
{
	return true;
}


} // namespace BVR

