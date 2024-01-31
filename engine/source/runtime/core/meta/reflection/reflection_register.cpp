#include "runtime/core/meta/reflection/reflection.h"
#include "reflection_register.h"

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
