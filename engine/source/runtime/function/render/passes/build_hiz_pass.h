#pragma once

#include "runtime/function/render/render_pass.h"

namespace Compass
{
    struct BuildHiZPassInitInfo : RenderPassInitInfo
    {
        RHIImage*     input_linear_depth_image {nullptr};
        RHIImageView* input_linear_depth {nullptr};
    };

    class BuildHiZPass : public RenderPass
    {
    public:
        enum RenderPipelineType : uint8_t
        {
            _render_pipeline_type_copy = 0,
            _render_pipeline_type_reduce,
            _render_pipeline_type_count
        };

        void initialize(const RenderPassInitInfo* init_info) override final;
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
        void draw() override final;

        void updateAfterFramebufferRecreate(RHIImage* input_linear_depth_image, RHIImageView* input_linear_depth);

        RHIImageView* getHiZImageView() const { return m_hiz_image_view; }
        RHISampler*   getHiZSampler() const { return m_hiz_sampler; }
        uint32_t      getHiZMipCount() const { return m_hiz_mip_count; }

    private:
        void setupAttachments();
        void setupDescriptorSetLayout();
        void setupPipelines();
        void setupDescriptorSet();
        void setupViewportBuffer();
        void destroyAttachments();
        void destroyViewportBuffer();
        void updateDescriptorSet(RHIDescriptorSet* descriptor_set,
                                 RHIImageView*     input_image_view,
                                 RHISampler*       input_sampler,
                                 RHIImageLayout    input_layout,
                                 RHIImageView*     output_image_view,
                                 RHIDeviceSize     viewport_buffer_offset);

    private:
        RHIImage*                  m_input_linear_depth_image {nullptr};
        RHIImageView*              m_input_linear_depth {nullptr};
        RHIImage*                  m_hiz_image {nullptr};
        RHIDeviceMemory*           m_hiz_image_memory {nullptr};
        RHIImageView*              m_hiz_image_view {nullptr};
        std::vector<RHIImageView*> m_hiz_mip_views;
        std::vector<RHIDescriptorSet*> m_hiz_descriptor_sets;
        RHIBuffer*                 m_hiz_viewport_buffer {nullptr};
        RHIDeviceMemory*           m_hiz_viewport_buffer_memory {nullptr};
        void*                      m_hiz_viewport_buffer_mapped {nullptr};
        RHISampler*                m_hiz_sampler {nullptr};
        RHIDeviceSize              m_hiz_dispatch_param_stride {0};
        uint32_t                   m_hiz_mip_count {1};
        bool                       m_hiz_initialized {false};
    };
} // namespace Compass
