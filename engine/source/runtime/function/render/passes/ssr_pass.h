#pragma once

#include "runtime/function/render/render_pass.h"

namespace Compass
{
    struct SSRPassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* render_pass {nullptr};
        RHIImageView*  scene_color_input_attachment {nullptr};
        RHIImageView*  gbuffer_metallic_roughness_input_attachment {nullptr};
        RHIImageView*  gbuffer_position_input_attachment {nullptr};
        RHIImageView*  gbuffer_normal_input_attachment {nullptr};
        RHIImageView*  hiz_input_attachment {nullptr};
        RHISampler*    hiz_sampler {nullptr};
    };

    class SSRPass : public RenderPass
    {
    public:
        enum RenderPipelineType : uint8_t
        {
            _render_pipeline_type_ssr = 0,
            _render_pipeline_type_passthrough,
            _render_pipeline_type_count
        };

        void initialize(const RenderPassInitInfo* init_info) override final;
        void draw() override final;
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void setEnableSSR(bool enable);

        void updateAfterFramebufferRecreate(RHIImageView* scene_color_input_attachment,
                                            RHIImageView* gbuffer_metallic_roughness_input_attachment,
                                            RHIImageView* gbuffer_position_input_attachment,
                                            RHIImageView* gbuffer_normal_input_attachment,
                                            RHIImageView* hiz_input_attachment,
                                            RHISampler*   hiz_sampler);

    private:
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();

    private:
        SSAOKernelObject m_ssr_storage_buffer_object;
        bool             m_enable_ssr {true};
    };
} // namespace Compass
