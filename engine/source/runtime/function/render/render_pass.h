#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_pass_base.h"
#include "runtime/function/render/render_resource.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace Compass
{
    class VulkanRHI;
    
    // 匿名枚举 <=> 定义
    // subpass
    enum 
    {
        _main_camera_pass_gbuffer_a                     = 0,
        _main_camera_pass_gbuffer_b                     = 1,
        _main_camera_pass_gbuffer_c                     = 2,
        _main_camera_pass_gbuffer_pos                   = 3,
        _main_camera_pass_gbuffer_normal                = 4,
        _main_camera_pass_backup_buffer_odd             = 5,
        _main_camera_pass_backup_buffer_even            = 6,
        _main_camera_pass_ssao                          = 7,      
        _main_camera_pass_ssao_blur                     = 8,    // custom_attachment
        _main_camera_pass_post_process_buffer_odd       = 9,
        _main_camera_pass_post_process_buffer_even      = 10,
        _main_camera_pass_depth                         = 11,
        _main_camera_pass_swap_chain_image              = 12,
        _main_camera_pass_custom_attachment_count       = 9,
        _main_camera_pass_post_process_attachment_count = 2,
        _main_camera_pass_attachment_count              = 13,
    };

    enum
    {
        _main_camera_subpass_gbuffer = 0,
        _main_camera_subpass_ssao,
        _main_camera_subpass_ssao_blur,
        _main_camera_subpass_deferred_lighting,
        _main_camera_subpass_forward_lighting,
        _main_camera_subpass_light_cube,
        _main_camera_subpass_tone_mapping,
        _main_camera_subpass_color_grading,
        _main_camera_subpass_fxaa,
        _main_camera_subpass_ui,
        _main_camera_subpass_combine_ui,
        _main_camera_subpass_count
    };

    struct VisibleNodes
    {
        std::vector<RenderMeshNode>*              p_directional_light_visible_mesh_nodes {nullptr};
        std::vector<RenderMeshNode>*              p_point_lights_visible_mesh_nodes {nullptr};
        std::vector<RenderMeshNode>*              p_main_camera_visible_mesh_nodes {nullptr};
        RenderAxisNode*                           p_axis_node {nullptr};
        std::vector<RenderLightCubeNode>*         p_light_cube_nodes {nullptr};
    };

    class RenderPass : public RenderPassBase
    {
    public:
        struct FrameBufferAttachment
        {
            RHIImage*        image;
            RHIDeviceMemory* mem;
            RHIImageView*    view;
            RHIFormat       format;
        };

        struct Framebuffer
        {
            int           width;
            int           height;
            RHIFramebuffer* framebuffer;
            RHIRenderPass*  render_pass;

            std::vector<FrameBufferAttachment> attachments;
        };

        struct Descriptor
        {
            RHIDescriptorSetLayout* layout;
            RHIDescriptorSet*       descriptor_set;
        };

        struct RenderPipelineBase
        {
            RHIPipelineLayout* layout;
            RHIPipeline*       pipeline;
        };

        GlobalRenderResource*      m_global_render_resource {nullptr};

        std::vector<Descriptor>         m_descriptor_infos;
        std::vector<RenderPipelineBase> m_render_pipelines;
        Framebuffer                     m_framebuffer;

        void initialize(const RenderPassInitInfo* init_info) override;
        void postInitialize() override;

        virtual void draw();

        virtual RHIRenderPass*                       getRenderPass() const;
        virtual std::vector<RHIImageView*>           getFramebufferImageViews() const;
        virtual std::vector<RHIDescriptorSetLayout*> getDescriptorSetLayouts() const;

        static VisibleNodes m_visible_nodes;
    };
} // namespace Compass
