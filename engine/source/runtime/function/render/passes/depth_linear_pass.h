#pragma once

#include "runtime/function/render/render_pass.h"

namespace Compass
{
    class RenderResourceBase;

    struct DepthLinearPassInitInfo : RenderPassInitInfo
    {
        RHIDescriptorSetLayout* per_mesh_layout {nullptr};
    };

    class DepthLinearPass : public RenderPass
    {
    public:
        void initialize(const RenderPassInitInfo* init_info) override final;
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void draw() override final;

        void updateAfterFramebufferRecreate();

        RHIImageView* getLinearDepthImageView() const { return m_framebuffer.attachments[0].view; }
        RHIImage*     getLinearDepthImage() const { return m_framebuffer.attachments[0].image; }

    private:
        void setupAttachments();
        void setupRenderPass();
        void setupFramebuffer();
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
        void drawModel();

    private:
        RHIDescriptorSetLayout*          m_per_mesh_layout {nullptr};
        MeshPerframeStorageBufferObject  m_mesh_perframe_storage_buffer_object;
    };
} // namespace Compass
