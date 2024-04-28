#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/core/math/matrix4.h"

#include <vector>

namespace Compass
{
    struct LightCubePassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* render_pass;
    };

    struct LightCubeVertex
    {
        struct VulkanMeshVertexPosition
        {
            Vector3 position;
        };

        static std::array<RHIVertexInputBindingDescription, 1> getBindingDescriptions()
        {
            std::array<RHIVertexInputBindingDescription, 1> binding_descriptions {};

            // position
            binding_descriptions[0].binding   = 0;
            binding_descriptions[0].stride    = sizeof(VulkanMeshVertexPosition);
            binding_descriptions[0].inputRate = RHI_VERTEX_INPUT_RATE_VERTEX;

            return binding_descriptions;
        }

        static std::array<RHIVertexInputAttributeDescription, 1> getAttributeDescriptions()
        {
            std::array<RHIVertexInputAttributeDescription, 1> attribute_descriptions {};

            // position
            attribute_descriptions[0].binding  = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format   = RHI_FORMAT_R32G32B32_SFLOAT;
            attribute_descriptions[0].offset   = offsetof(VulkanMeshVertexPosition, position);

            return attribute_descriptions;
        }
    };

    class LightCubePass : public RenderPass
    {
    public:
        void initialize(const RenderPassInitInfo* init_info) override final;
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void draw() override final;

        void updateAfterFramebufferRecreate();

    private:
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
        void prepareUniformBuffer();
        void updateUniformBuffer(Matrix4x4 model);
        void createVertexBuffer();

        RHIBuffer* m_light_cube_uniform_buffer = nullptr;
        RHIBuffer* m_vertex_buffer = nullptr;
        RHIDeviceMemory* m_light_cube_uniform_buffer_memory = nullptr;
        RHIDeviceMemory* m_vertex_buffer_memory = nullptr;
        void* m_uniform_buffer_mapped = nullptr;
        void* m_vertex_buffer_mapped = nullptr;

        struct LightCubeUniformBuffer
        {
            Matrix4x4 proj_view;
            Matrix4x4 model;
        } m_ubo;

        std::vector<Vector3> m_lights_position;

        std::vector<float> vertices = {
            // back face
            -1.0f, -1.0f, -1.0f,  // bottom-left
             1.0f,  1.0f, -1.0f,  // top-right
             1.0f, -1.0f, -1.0f,  // bottom-right         
             1.0f,  1.0f, -1.0f,  // top-right
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f,  1.0f, -1.0f,  // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  // bottom-left
             1.0f, -1.0f,  1.0f,  // bottom-right
             1.0f,  1.0f,  1.0f,  // top-right
             1.0f,  1.0f,  1.0f,  // top-right
            -1.0f,  1.0f,  1.0f,  // top-left
            -1.0f, -1.0f,  1.0f,  // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f,  // top-right
            -1.0f,  1.0f, -1.0f,  // top-left
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f,  // bottom-right
            -1.0f,  1.0f,  1.0f,  // top-right
            // right face
             1.0f,  1.0f,  1.0f,  // top-left
             1.0f, -1.0f, -1.0f,  // bottom-right
             1.0f,  1.0f, -1.0f,  // top-right         
             1.0f, -1.0f, -1.0f,  // bottom-right
             1.0f,  1.0f,  1.0f,  // top-left
             1.0f, -1.0f,  1.0f,  // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  // top-right
             1.0f, -1.0f, -1.0f,  // top-left
             1.0f, -1.0f,  1.0f,  // bottom-left
             1.0f, -1.0f,  1.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f,  // bottom-right
            -1.0f, -1.0f, -1.0f,  // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  // top-left
             1.0f,  1.0f , 1.0f,  // bottom-right
             1.0f,  1.0f, -1.0f,  // top-right     
             1.0f,  1.0f,  1.0f,  // bottom-right
            -1.0f,  1.0f, -1.0f,  // top-left
            -1.0f,  1.0f,  1.0f,  // bottom-left        
        };

    };
} // namespace Compass
