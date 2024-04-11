#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/core/math/vector3.h"

namespace Compass
{
    struct SSAOBlurPassInitInfo : RenderPassInitInfo
    {
        RHIRenderPass* render_pass;
        RHIImageView* input_attachment;
    };

    class SSAOBlurPass : public RenderPass
    {
    public:
        void initialize(const RenderPassInitInfo* init_info) override final;
        void draw() override final;
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;

        void updateAfterFramebufferRecreate(RHIImageView* input_attachment);

    public:
        SSAOKernelObject m_ssao_kernel_storage_buffer_object;

    private:
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();

    };
} // namespace Compass
