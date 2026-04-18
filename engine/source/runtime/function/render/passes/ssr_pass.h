#pragma once

#include "runtime/function/render/render_pass.h"

namespace Compass
{
    struct SSRPassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* render_pass;
        RHIImageView*  scene_color_input_attachment;
        RHIImageView*  gbuffer_metallic_roughness_input_attachment;
        RHIImageView*  gbuffer_position_input_attachment;
        RHIImageView*  gbuffer_normal_input_attachment;
    };

    class SSRPass : public RenderPass
    {
    public:
        void initialize(const RenderPassInitInfo* init_info) override final;
        void draw() override final;
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;

        void updateAfterFramebufferRecreate(RHIImageView* scene_color_input_attachment,
                                            RHIImageView* gbuffer_metallic_roughness_input_attachment,
                                            RHIImageView* gbuffer_position_input_attachment,
                                            RHIImageView* gbuffer_normal_input_attachment);

    private:
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();

    private:
        SSAOKernelObject m_ssr_storage_buffer_object;
    };
} // namespace Compass
