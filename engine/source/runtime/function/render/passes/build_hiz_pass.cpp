#include "runtime/function/render/passes/build_hiz_pass.h"
#include "runtime/function/render/render_helper.h"

#include <build_hiz_copy_comp.h>
#include <build_hiz_reduce_comp.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Compass
{
    namespace
    {
        uint32_t ceilDiv(uint32_t x, uint32_t y)
        {
            return (x + y - 1) / y;
        }

        struct HiZRect
        {
            int offset_x {0};
            int offset_y {0};
            int width {0};
            int height {0};
        };

        HiZRect getViewportRect(const RHIViewport* viewport, const RHIExtent2D& extent)
        {
            const int max_x = extent.width > 0 ? static_cast<int>(extent.width - 1) : 0;
            const int max_y = extent.height > 0 ? static_cast<int>(extent.height - 1) : 0;

            HiZRect rect;
            rect.offset_x = std::clamp(static_cast<int>(viewport->x), 0, max_x);
            rect.offset_y = std::clamp(static_cast<int>(viewport->y), 0, max_y);
            rect.width    = std::max(1, static_cast<int>(viewport->width));
            rect.height   = std::max(1, static_cast<int>(viewport->height));
            rect.width    = std::min(rect.width, static_cast<int>(extent.width) - rect.offset_x);
            rect.height   = std::min(rect.height, static_cast<int>(extent.height) - rect.offset_y);
            return rect;
        }

        HiZRect downsampleRect(const HiZRect& src_rect)
        {
            HiZRect dst_rect;
            dst_rect.offset_x = src_rect.offset_x / 2;
            dst_rect.offset_y = src_rect.offset_y / 2;

            const int src_max_x = src_rect.offset_x + src_rect.width;
            const int src_max_y = src_rect.offset_y + src_rect.height;
            const int dst_max_x = (src_max_x + 1) / 2;
            const int dst_max_y = (src_max_y + 1) / 2;

            dst_rect.width  = std::max(0, dst_max_x - dst_rect.offset_x);
            dst_rect.height = std::max(0, dst_max_y - dst_rect.offset_y);
            return dst_rect;
        }
    } // namespace

    void BuildHiZPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const BuildHiZPassInitInfo* _init_info = static_cast<const BuildHiZPassInitInfo*>(init_info);
        m_input_linear_depth_image             = _init_info->input_linear_depth_image;
        m_input_linear_depth                   = _init_info->input_linear_depth;

        setupAttachments();
        setupViewportBuffer();
        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
    }

    void BuildHiZPass::preparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        (void)render_resource;

        if (!m_hiz_viewport_buffer_mapped)
        {
            return;
        }

        const RHISwapChainDesc swapchain_info = m_rhi->getSwapchainInfo();
        HiZRect                current_rect   = getViewportRect(swapchain_info.viewport, swapchain_info.extent);

        std::vector<HiZDispatchParams> dispatch_params(m_hiz_mip_count);
        for (uint32_t mip = 0; mip < m_hiz_mip_count; ++mip)
        {
            HiZDispatchParams& mip_params = dispatch_params[mip];
            mip_params.input_offset_x     = current_rect.offset_x;
            mip_params.input_offset_y     = current_rect.offset_y;
            mip_params.input_width        = current_rect.width;
            mip_params.input_height       = current_rect.height;

            const HiZRect output_rect = (mip == 0) ? current_rect : downsampleRect(current_rect);
            mip_params.output_offset_x = output_rect.offset_x;
            mip_params.output_offset_y = output_rect.offset_y;
            mip_params.output_width    = output_rect.width;
            mip_params.output_height   = output_rect.height;

            std::memcpy(reinterpret_cast<char*>(m_hiz_viewport_buffer_mapped) + mip * m_hiz_dispatch_param_stride,
                        &mip_params,
                        sizeof(HiZDispatchParams));

            current_rect = output_rect;
        }
    }

    void BuildHiZPass::updateAfterFramebufferRecreate(RHIImage* input_linear_depth_image, RHIImageView* input_linear_depth)
    {
        m_input_linear_depth_image = input_linear_depth_image;
        m_input_linear_depth       = input_linear_depth;
        destroyAttachments();
        destroyViewportBuffer();
        setupAttachments();
        setupViewportBuffer();
        setupDescriptorSet();
        m_hiz_initialized = false;
    }

    void BuildHiZPass::setupAttachments()
    {
        const uint32_t width   = m_rhi->getSwapchainInfo().extent.width;
        const uint32_t height  = m_rhi->getSwapchainInfo().extent.height;
        const uint32_t max_dim = std::max(width, height);

        m_hiz_mip_count = 1;
        for (uint32_t dimension = max_dim; dimension > 1; dimension = std::max(1u, dimension >> 1))
        {
            ++m_hiz_mip_count;
        }

        m_rhi->createImage(width,
                           height,
                           RHI_FORMAT_R32_SFLOAT,
                           RHI_IMAGE_TILING_OPTIMAL,
                           RHI_IMAGE_USAGE_SAMPLED_BIT | RHI_IMAGE_USAGE_STORAGE_BIT,
                           RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           m_hiz_image,
                           m_hiz_image_memory,
                           0,
                           1,
                           m_hiz_mip_count);

        m_rhi->createImageView(m_hiz_image,
                               RHI_FORMAT_R32_SFLOAT,
                               RHI_IMAGE_ASPECT_COLOR_BIT,
                               RHI_IMAGE_VIEW_TYPE_2D,
                               1,
                               m_hiz_mip_count,
                               m_hiz_image_view);

        m_hiz_mip_views.resize(m_hiz_mip_count);
        for (uint32_t mip = 0; mip < m_hiz_mip_count; ++mip)
        {
            m_rhi->createImageView(m_hiz_image,
                                   RHI_FORMAT_R32_SFLOAT,
                                   RHI_IMAGE_ASPECT_COLOR_BIT,
                                   RHI_IMAGE_VIEW_TYPE_2D,
                                   1,
                                   1,
                                   m_hiz_mip_views[mip],
                                   mip);
        }

        RHISamplerCreateInfo sampler_create_info {};
        sampler_create_info.sType            = RHI_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.magFilter        = RHI_FILTER_NEAREST;
        sampler_create_info.minFilter        = RHI_FILTER_NEAREST;
        sampler_create_info.mipmapMode       = RHI_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_create_info.addressModeU     = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeV     = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeW     = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.mipLodBias       = 0.0f;
        sampler_create_info.anisotropyEnable = RHI_FALSE;
        sampler_create_info.maxAnisotropy    = 1.0f;
        sampler_create_info.compareEnable    = RHI_FALSE;
        sampler_create_info.compareOp        = RHI_COMPARE_OP_NEVER;
        sampler_create_info.minLod           = 0.0f;
        sampler_create_info.maxLod           = static_cast<float>(m_hiz_mip_count - 1);
        sampler_create_info.borderColor      = RHI_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_create_info.unnormalizedCoordinates = RHI_FALSE;

        if (RHI_SUCCESS != m_rhi->createSampler(&sampler_create_info, m_hiz_sampler))
        {
            throw std::runtime_error("create hiz sampler");
        }
    }

    void BuildHiZPass::setupViewportBuffer()
    {
        RHIPhysicalDeviceProperties device_properties {};
        m_rhi->getPhysicalDeviceProperties(&device_properties);

        const uint32_t uniform_alignment = std::max<uint32_t>(
            1u, static_cast<uint32_t>(device_properties.limits.minUniformBufferOffsetAlignment));
        m_hiz_dispatch_param_stride = roundUp(sizeof(HiZDispatchParams), uniform_alignment);

        m_rhi->createBufferAndInitialize(RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         m_hiz_viewport_buffer,
                                         m_hiz_viewport_buffer_memory,
                                         m_hiz_dispatch_param_stride * m_hiz_mip_count);

        if (RHI_SUCCESS !=
            m_rhi->mapMemory(m_hiz_viewport_buffer_memory, 0, RHI_WHOLE_SIZE, 0, &m_hiz_viewport_buffer_mapped))
        {
            throw std::runtime_error("map hiz viewport buffer");
        }
    }

    void BuildHiZPass::destroyAttachments()
    {
        for (RHIImageView* mip_view : m_hiz_mip_views)
        {
            m_rhi->destroyImageView(mip_view);
        }
        m_hiz_mip_views.clear();

        if (m_hiz_image_view)
        {
            m_rhi->destroyImageView(m_hiz_image_view);
            m_hiz_image_view = nullptr;
        }

        if (m_hiz_image)
        {
            m_rhi->destroyImage(m_hiz_image);
            m_hiz_image = nullptr;
        }

        if (m_hiz_image_memory)
        {
            m_rhi->freeMemory(m_hiz_image_memory);
            m_hiz_image_memory = nullptr;
        }

        if (m_hiz_sampler)
        {
            m_rhi->destroySampler(m_hiz_sampler);
            m_hiz_sampler = nullptr;
        }
    }

    void BuildHiZPass::destroyViewportBuffer()
    {
        if (m_hiz_viewport_buffer_mapped)
        {
            m_rhi->unmapMemory(m_hiz_viewport_buffer_memory);
            m_hiz_viewport_buffer_mapped = nullptr;
        }

        if (m_hiz_viewport_buffer)
        {
            m_rhi->destroyBuffer(m_hiz_viewport_buffer);
            m_hiz_viewport_buffer = nullptr;
        }

        if (m_hiz_viewport_buffer_memory)
        {
            m_rhi->freeMemory(m_hiz_viewport_buffer_memory);
            m_hiz_viewport_buffer_memory = nullptr;
        }
    }

    void BuildHiZPass::setupDescriptorSetLayout()
    {
        m_descriptor_infos.resize(1);

        RHIDescriptorSetLayoutBinding bindings[3] = {};

        bindings[0].binding            = 0;
        bindings[0].descriptorType     = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount    = 1;
        bindings[0].stageFlags         = RHI_SHADER_STAGE_COMPUTE_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding            = 1;
        bindings[1].descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount    = 1;
        bindings[1].stageFlags         = RHI_SHADER_STAGE_COMPUTE_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2].binding            = 2;
        bindings[2].descriptorType     = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[2].descriptorCount    = 1;
        bindings[2].stageFlags         = RHI_SHADER_STAGE_COMPUTE_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        RHIDescriptorSetLayoutCreateInfo layout_create_info {};
        layout_create_info.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layout_create_info.pBindings    = bindings;

        if (RHI_SUCCESS != m_rhi->createDescriptorSetLayout(&layout_create_info, m_descriptor_infos[0].layout))
        {
            throw std::runtime_error("create hiz descriptor set layout");
        }
    }

    void BuildHiZPass::setupPipelines()
    {
        m_render_pipelines.resize(_render_pipeline_type_count);

        RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts    = &m_descriptor_infos[0].layout;

        for (uint32_t pipeline_index = 0; pipeline_index < _render_pipeline_type_count; ++pipeline_index)
        {
            if (RHI_SUCCESS != m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[pipeline_index].layout))
            {
                throw std::runtime_error("create hiz pipeline layout");
            }
        }

        RHIComputePipelineCreateInfo pipeline_create_info {};
        pipeline_create_info.sType = RHI_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_create_info.flags = 0;

        RHIPipelineShaderStageCreateInfo shader_stage {};
        shader_stage.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage.stage = RHI_SHADER_STAGE_COMPUTE_BIT;
        shader_stage.pName = "main";

        RHIShader* copy_shader = m_rhi->createShaderModule(BUILD_HIZ_COPY_COMP);
        shader_stage.module    = copy_shader;
        pipeline_create_info.pStages = &shader_stage;
        pipeline_create_info.layout  = m_render_pipelines[_render_pipeline_type_copy].layout;
        if (RHI_SUCCESS != m_rhi->createComputePipelines(nullptr, 1, &pipeline_create_info, m_render_pipelines[_render_pipeline_type_copy].pipeline))
        {
            throw std::runtime_error("create hiz copy pipeline");
        }

        RHIShader* reduce_shader = m_rhi->createShaderModule(BUILD_HIZ_REDUCE_COMP);
        shader_stage.module      = reduce_shader;
        pipeline_create_info.layout = m_render_pipelines[_render_pipeline_type_reduce].layout;
        if (RHI_SUCCESS != m_rhi->createComputePipelines(nullptr, 1, &pipeline_create_info, m_render_pipelines[_render_pipeline_type_reduce].pipeline))
        {
            throw std::runtime_error("create hiz reduce pipeline");
        }

        m_rhi->destroyShaderModule(copy_shader);
        m_rhi->destroyShaderModule(reduce_shader);
    }

    void BuildHiZPass::setupDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo descriptor_set_alloc_info {};
        descriptor_set_alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_alloc_info.descriptorPool     = m_rhi->getDescriptorPoor();
        descriptor_set_alloc_info.descriptorSetCount = 1;
        descriptor_set_alloc_info.pSetLayouts        = &m_descriptor_infos[0].layout;

        while (m_hiz_descriptor_sets.size() < m_hiz_mip_count)
        {
            RHIDescriptorSet* descriptor_set = nullptr;
            if (RHI_SUCCESS != m_rhi->allocateDescriptorSets(&descriptor_set_alloc_info, descriptor_set))
            {
                throw std::runtime_error("allocate hiz descriptor set");
            }
            m_hiz_descriptor_sets.push_back(descriptor_set);
        }

        if (!m_hiz_descriptor_sets.empty())
        {
            m_descriptor_infos[0].descriptor_set = m_hiz_descriptor_sets[0];
        }

        for (uint32_t mip = 0; mip < m_hiz_mip_count; ++mip)
        {
            updateDescriptorSet(m_hiz_descriptor_sets[mip],
                                mip == 0 ? m_input_linear_depth : m_hiz_mip_views[mip - 1],
                                m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest),
                                mip == 0 ? RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : RHI_IMAGE_LAYOUT_GENERAL,
                                m_hiz_mip_views[mip],
                                mip * m_hiz_dispatch_param_stride);
        }
    }

    void BuildHiZPass::updateDescriptorSet(RHIDescriptorSet* descriptor_set,
                                           RHIImageView*     input_image_view,
                                           RHISampler*       input_sampler,
                                           RHIImageLayout    input_layout,
                                           RHIImageView*     output_image_view,
                                           RHIDeviceSize     viewport_buffer_offset)
    {
        RHIDescriptorImageInfo input_image_info {};
        input_image_info.sampler     = input_sampler;
        input_image_info.imageView   = input_image_view;
        input_image_info.imageLayout = input_layout;

        RHIDescriptorImageInfo output_image_info {};
        output_image_info.sampler     = nullptr;
        output_image_info.imageView   = output_image_view;
        output_image_info.imageLayout = RHI_IMAGE_LAYOUT_GENERAL;

        RHIDescriptorBufferInfo viewport_buffer_info {};
        viewport_buffer_info.offset = viewport_buffer_offset;
        viewport_buffer_info.range  = sizeof(HiZDispatchParams);
        viewport_buffer_info.buffer = m_hiz_viewport_buffer;

        RHIWriteDescriptorSet descriptor_writes[3] = {};

        descriptor_writes[0].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet          = descriptor_set;
        descriptor_writes[0].dstBinding      = 0;
        descriptor_writes[0].descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pImageInfo      = &input_image_info;

        descriptor_writes[1].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet          = descriptor_set;
        descriptor_writes[1].dstBinding      = 1;
        descriptor_writes[1].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pImageInfo      = &output_image_info;

        descriptor_writes[2].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[2].dstSet          = descriptor_set;
        descriptor_writes[2].dstBinding      = 2;
        descriptor_writes[2].descriptorType  = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[2].descriptorCount = 1;
        descriptor_writes[2].pBufferInfo     = &viewport_buffer_info;

        m_rhi->updateDescriptorSets(sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
    }

    void BuildHiZPass::draw()
    {
        if (!m_input_linear_depth_image || !m_input_linear_depth || !m_hiz_image_view ||
            m_hiz_descriptor_sets.size() < m_hiz_mip_count)
        {
            return;
        }

        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "Build HiZ", color);

        RHIImageSubresourceRange linear_depth_range = {RHI_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        RHIImageMemoryBarrier    linear_depth_barrier {};
        linear_depth_barrier.sType               = RHI_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        linear_depth_barrier.srcAccessMask       = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        linear_depth_barrier.dstAccessMask       = RHI_ACCESS_SHADER_READ_BIT;
        linear_depth_barrier.oldLayout           = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        linear_depth_barrier.newLayout           = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        linear_depth_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        linear_depth_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        linear_depth_barrier.image               = m_input_linear_depth_image;
        linear_depth_barrier.subresourceRange    = linear_depth_range;

        RHIImageMemoryBarrier hiz_image_barrier {};
        hiz_image_barrier.sType               = RHI_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        hiz_image_barrier.srcAccessMask       = m_hiz_initialized ? (RHI_ACCESS_SHADER_WRITE_BIT | RHI_ACCESS_SHADER_READ_BIT) : 0;
        hiz_image_barrier.dstAccessMask       = RHI_ACCESS_SHADER_WRITE_BIT;
        hiz_image_barrier.oldLayout =
            m_hiz_initialized ? RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : RHI_IMAGE_LAYOUT_UNDEFINED;
        hiz_image_barrier.newLayout           = RHI_IMAGE_LAYOUT_GENERAL;
        hiz_image_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        hiz_image_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        hiz_image_barrier.image               = m_hiz_image;
        hiz_image_barrier.subresourceRange    = {RHI_IMAGE_ASPECT_COLOR_BIT, 0, m_hiz_mip_count, 0, 1};

        RHIImageMemoryBarrier setup_barriers[2] = {linear_depth_barrier, hiz_image_barrier};
        const RHIPipelineStageFlags setup_src_stage =
            m_hiz_initialized ? (RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                                 RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) :
                                RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        m_rhi->cmdPipelineBarrier(m_rhi->getCurrentCommandBuffer(),
                                  setup_src_stage,
                                  RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                  0,
                                  0,
                                  nullptr,
                                  0,
                                  nullptr,
                                  2,
                                  setup_barriers);

        m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(),
                                  RHI_PIPELINE_BIND_POINT_COMPUTE,
                                  m_render_pipelines[_render_pipeline_type_copy].pipeline);
        m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_COMPUTE,
                                        m_render_pipelines[_render_pipeline_type_copy].layout,
                                        0,
                                        1,
                                        &m_hiz_descriptor_sets[0],
                                        0,
                                        nullptr);
        m_rhi->cmdDispatch(m_rhi->getCurrentCommandBuffer(),
                           ceilDiv(m_rhi->getSwapchainInfo().extent.width, 8),
                           ceilDiv(m_rhi->getSwapchainInfo().extent.height, 8),
                           1);

        for (uint32_t mip = 1; mip < m_hiz_mip_count; ++mip)
        {
            RHIImageMemoryBarrier mip_barrier {};
            mip_barrier.sType               = RHI_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            mip_barrier.srcAccessMask       = RHI_ACCESS_SHADER_WRITE_BIT;
            mip_barrier.dstAccessMask       = RHI_ACCESS_SHADER_READ_BIT;
            mip_barrier.oldLayout           = RHI_IMAGE_LAYOUT_GENERAL;
            mip_barrier.newLayout           = RHI_IMAGE_LAYOUT_GENERAL;
            mip_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            mip_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            mip_barrier.image               = m_hiz_image;
            mip_barrier.subresourceRange    = {RHI_IMAGE_ASPECT_COLOR_BIT, mip - 1, 1, 0, 1};

            m_rhi->cmdPipelineBarrier(m_rhi->getCurrentCommandBuffer(),
                                      RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                      RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                      0,
                                      0,
                                      nullptr,
                                      0,
                                      nullptr,
                                      1,
                                      &mip_barrier);

            m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(),
                                      RHI_PIPELINE_BIND_POINT_COMPUTE,
                                      m_render_pipelines[_render_pipeline_type_reduce].pipeline);
            m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                            RHI_PIPELINE_BIND_POINT_COMPUTE,
                                            m_render_pipelines[_render_pipeline_type_reduce].layout,
                                            0,
                                            1,
                                            &m_hiz_descriptor_sets[mip],
                                            0,
                                            nullptr);

            const uint32_t mip_width  = std::max(1u, m_rhi->getSwapchainInfo().extent.width >> mip);
            const uint32_t mip_height = std::max(1u, m_rhi->getSwapchainInfo().extent.height >> mip);
            m_rhi->cmdDispatch(m_rhi->getCurrentCommandBuffer(), ceilDiv(mip_width, 8), ceilDiv(mip_height, 8), 1);
        }

        RHIImageMemoryBarrier final_barrier {};
        final_barrier.sType               = RHI_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        final_barrier.srcAccessMask       = RHI_ACCESS_SHADER_WRITE_BIT;
        final_barrier.dstAccessMask       = RHI_ACCESS_SHADER_READ_BIT;
        final_barrier.oldLayout           = RHI_IMAGE_LAYOUT_GENERAL;
        final_barrier.newLayout           = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        final_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        final_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        final_barrier.image               = m_hiz_image;
        final_barrier.subresourceRange    = {RHI_IMAGE_ASPECT_COLOR_BIT, 0, m_hiz_mip_count, 0, 1};

        m_rhi->cmdPipelineBarrier(m_rhi->getCurrentCommandBuffer(),
                                  RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                  RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                  0,
                                  0,
                                  nullptr,
                                  0,
                                  nullptr,
                                  1,
                                  &final_barrier);

        m_hiz_initialized = true;
        m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
    }
} // namespace Compass
