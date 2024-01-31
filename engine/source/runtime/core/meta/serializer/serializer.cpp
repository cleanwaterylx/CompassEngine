#include "serializer.h"
#include <assert.h>
namespace Compass
{

    template<>
    PJson PSerializer::write(const char& instance)
    {
        return PJson(instance);
    }
    template<>
    char& PSerializer::read(const PJson& json_context, char& instance)
    {
        assert(json_context.is_number());
        return instance = json_context.number_value();
    }

    template<>
    PJson PSerializer::write(const int& instance)
    {
        return PJson(instance);
    }
    template<>
    int& PSerializer::read(const PJson& json_context, int& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<int>(json_context.number_value());
    }

    template<>
    PJson PSerializer::write(const unsigned int& instance)
    {
        return PJson(static_cast<int>(instance));
    }
    template<>
    unsigned int& PSerializer::read(const PJson& json_context, unsigned int& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<unsigned int>(json_context.number_value());
    }

    template<>
    PJson PSerializer::write(const float& instance)
    {
        return PJson(instance);
    }
    template<>
    float& PSerializer::read(const PJson& json_context, float& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<float>(json_context.number_value());
    }

    template<>
    PJson PSerializer::write(const double& instance)
    {
        return PJson(instance);
    }
    template<>
    double& PSerializer::read(const PJson& json_context, double& instance)
    {
        assert(json_context.is_number());
        return instance = static_cast<float>(json_context.number_value());
    }

    template<>
    PJson PSerializer::write(const bool& instance)
    {
        return PJson(instance);
    }
    template<>
    bool& PSerializer::read(const PJson& json_context, bool& instance)
    {
        assert(json_context.is_bool());
        return instance = json_context.bool_value();
    }

    template<>
    PJson PSerializer::write(const std::string& instance)
    {
        return PJson(instance);
    }
    template<>
    std::string& PSerializer::read(const PJson& json_context, std::string& instance)
    {
        assert(json_context.is_string());
        return instance = json_context.string_value();
    }

} // namespace Compass