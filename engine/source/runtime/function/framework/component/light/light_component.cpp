#include "runtime/function/framework/component/light/light_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"

#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"

namespace Compass
{
    void LightComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        // get position
        TransformComponent* transform_component = parent_object.lock()->tryGetComponent(TransformComponent);
        m_light_res.m_position = transform_component->getPosition();

    }

    void LightComponent::setRadius(const float new_radius)
    {
        m_light_res.m_radius = new_radius;
    }

    void LightComponent::setIntensity(const float new_intensity)
    {
        m_light_res.m_intensity = new_intensity;
    }

    void LightComponent::tick(float delta_time)
    {
        if(!m_parent_object.lock())
            return;

        TransformComponent* transform_component = m_parent_object.lock()->tryGetComponent(TransformComponent);

        m_light_res.m_position = transform_component->getPosition();

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData&    logic_swap_data     = render_swap_context.getLogicSwapData();

        if(logic_swap_data.m_light_swap_data.has_value())
        {
            logic_swap_data.m_light_swap_data->m_point_lights.push_back(m_light_res);
        }
        else
        {
            LightSwapData light_swap_data;
            light_swap_data.m_point_lights.push_back(m_light_res);
            logic_swap_data.m_light_swap_data = light_swap_data;
        }
    }


} // namespace Compass
