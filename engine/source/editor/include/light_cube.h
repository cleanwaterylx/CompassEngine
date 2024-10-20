#pragma once

#include "runtime/function/render/render_entity.h"
#include "runtime/function/render/render_type.h"

namespace Compass
{
    class EditorLightCube : public RenderEntity
    {
    public:
        EditorLightCube();
        RenderMeshData m_mesh_data;
    };
}