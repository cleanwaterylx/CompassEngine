#include <assert.h>

#include "runtime/core/meta/json.h"
#include "runtime/core/meta/reflection/reflection.h"
#include "runtime/core/meta/reflection/reflection_register.h"
#include "runtime/core/meta/serializer/serializer.h"

#include "_generated/reflection/all_reflection.h"
#include "_generated/serializer/all_serializer.ipp"

namespace Compass
{
    namespace Reflection
    {
        void Reflection::TypeMetaRegister::Unregister()
        {
            TypeMetaRegisterInterface::unregisterAll();
        }
    } // namespace Reflection

} // namespace Compass
