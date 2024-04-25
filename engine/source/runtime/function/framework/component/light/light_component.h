#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/resource/res_type/global/global_rendering.h"

namespace Compass
{
    REFLECTION_TYPE(LightComponent)
    CLASS(LightComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(LightComponent)

    public:
        LightComponent() = default;

        void postLoadResource(std::weak_ptr<GObject> parent_object) override;

        void tick(float delta_time) override;

        void setRadius(const float new_radius);

        void setIntensity(const float new_intensity);

    protected:
        META(Enable)
        PointLight m_light_res;

    };
} // namespace Compass
