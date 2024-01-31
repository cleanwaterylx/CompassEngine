#pragma once
#include "runtime/core/meta/json.h"
#include "runtime/core/meta/reflection/reflection.h"

#include <cassert>

namespace Compass
{
    template<typename...>
    inline constexpr bool always_false = false;

    class PSerializer
    {
    public:
        template<typename T>
        static PJson writePointer(T* instance)
        {
            return PJson::object {{"$typeName", PJson {"*"}}, {"$context", PSerializer::write(*instance)}};
        }

        template<typename T>
        static T*& readPointer(const PJson& json_context, T*& instance)
        {
            assert(instance == nullptr);
            std::string type_name = json_context["$typeName"].string_value();
            assert(!type_name.empty());
            if ('*' == type_name[0])
            {
                instance = new T;
                read(json_context["$context"], *instance);
            }
            else
            {
                instance = static_cast<T*>(
                    Reflection::TypeMeta::newFromNameAndPJson(type_name, json_context["$context"]).m_instance);
            }
            return instance;
        }

        template<typename T>
        static PJson write(const Reflection::ReflectionPtr<T>& instance)
        {
            T*          instance_ptr = static_cast<T*>(instance.operator->());
            std::string type_name    = instance.getTypeName();
            return PJson::object {{"$typeName", PJson(type_name)},
                                  {"$context", Reflection::TypeMeta::writeByName(type_name, instance_ptr)}};
        }

        template<typename T>
        static T*& read(const PJson& json_context, Reflection::ReflectionPtr<T>& instance)
        {
            std::string type_name = json_context["$typeName"].string_value();
            instance.setTypeName(type_name);
            return readPointer(json_context, instance.getPtrReference());
        }

        template<typename T>
        static PJson write(const T& instance)
        {

            if constexpr (std::is_pointer<T>::value)
            {
                return writePointer((T)instance);
            }
            else
            {
                static_assert(always_false<T>, "PSerializer::write<T> has not been implemented yet!");
                return PJson();
            }
        }

        template<typename T>
        static T& read(const PJson& json_context, T& instance)
        {
            if constexpr (std::is_pointer<T>::value)
            {
                return readPointer(json_context, instance);
            }
            else
            {
                static_assert(always_false<T>, "PSerializer::read<T> has not been implemented yet!");
                return instance;
            }
        }
    };

    // implementation of base types
    template<>
    PJson PSerializer::write(const char& instance);
    template<>
    char& PSerializer::read(const PJson& json_context, char& instance);

    template<>
    PJson PSerializer::write(const int& instance);
    template<>
    int& PSerializer::read(const PJson& json_context, int& instance);

    template<>
    PJson PSerializer::write(const unsigned int& instance);
    template<>
    unsigned int& PSerializer::read(const PJson& json_context, unsigned int& instance);

    template<>
    PJson PSerializer::write(const float& instance);
    template<>
    float& PSerializer::read(const PJson& json_context, float& instance);

    template<>
    PJson PSerializer::write(const double& instance);
    template<>
    double& PSerializer::read(const PJson& json_context, double& instance);

    template<>
    PJson PSerializer::write(const bool& instance);
    template<>
    bool& PSerializer::read(const PJson& json_context, bool& instance);

    template<>
    PJson PSerializer::write(const std::string& instance);
    template<>
    std::string& PSerializer::read(const PJson& json_context, std::string& instance);
}
