#include "runtime/function/render/passes/ssr_pass.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"

#include <ssr_frag.h>
#include <ssr_passthrough_frag.h>
#include <ssr_vert.h>

#include <stdexcept>

namespace Compass
{
    void SSRPass::setEnableSSR(bool enable) { m_enable_ssr = enable; }

    void SSRPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const SSRPassInitInfo* _init_info = static_cast<const SSRPassInitInfo*>(init_info);
        m_framebuffer.render_pass         = _init_info->render_pass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        updateAfterFramebufferRecreate(_init_info->scene_color_input_attachment,
                                       _init_info->gbuffer_metallic_roughness_input_attachment,
                                       _init_info->gbuffer_position_input_attachment,
                                       _init_info->gbuffer_normal_input_attachment);
    }

    void SSRPass::setupDescriptorSetLayout()
    {
        m_descriptor_infos.resize(1);

        RHIDescriptorSetLayoutBinding ssr_global_layout_bindings[5] = {};

        RHIDescriptorSetLayoutBinding& scene_color_binding = ssr_global_layout_bindings[0];
        scene_color_binding.binding                       = 0;
        scene_color_binding.descriptorType                = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        scene_color_binding.descriptorCount               = 1;
        scene_color_binding.stageFlags                    = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutBinding& gbuffer_metallic_roughness_binding = ssr_global_layout_bindings[1];
        gbuffer_metallic_roughness_binding.binding                        = 1;
        gbuffer_metallic_roughness_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbuffer_metallic_roughness_binding.descriptorCount                = 1;
        gbuffer_metallic_roughness_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutBinding& gbuffer_position_binding = ssr_global_layout_bindings[2];
        gbuffer_position_binding.binding                        = 2;
        gbuffer_position_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbuffer_position_binding.descriptorCount                = 1;
        gbuffer_position_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutBinding& gbuffer_normal_binding = ssr_global_layout_bindings[3];
        gbuffer_normal_binding.binding                        = 3;
        gbuffer_normal_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbuffer_normal_binding.descriptorCount                = 1;
        gbuffer_normal_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutBinding& ssr_storage_binding = ssr_global_layout_bindings[4];
        ssr_storage_binding.binding                        = 4;
        ssr_storage_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssr_storage_binding.descriptorCount                = 1;
        ssr_storage_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutCreateInfo ssr_global_layout_create_info {};
        ssr_global_layout_create_info.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ssr_global_layout_create_info.pNext        = nullptr;
        ssr_global_layout_create_info.flags        = 0;
        ssr_global_layout_create_info.bindingCount = sizeof(ssr_global_layout_bindings) / sizeof(ssr_global_layout_bindings[0]);
        ssr_global_layout_create_info.pBindings    = ssr_global_layout_bindings;

        if (RHI_SUCCESS != m_rhi->createDescriptorSetLayout(&ssr_global_layout_create_info, m_descriptor_infos[0].layout))
        {
            throw std::runtime_error("create ssr global layout");
        }
    }

    void SSRPass::setupPipelines()
    {
        m_render_pipelines.resize(_render_pipeline_type_count);

        RHIDescriptorSetLayout*       descriptorset_layouts[1] = {m_descriptor_infos[0].layout};
        for (uint8_t pipeline_index = 0; pipeline_index < _render_pipeline_type_count; ++pipeline_index)
        {
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
            pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

            if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[pipeline_index].layout) !=
                RHI_SUCCESS)
            {
                throw std::runtime_error("create ssr pipeline layout");
            }
        }

        RHIShader* vert_shader_module = m_rhi->createShaderModule(SSR_VERT);
        RHIShader* ssr_frag_shader_module = m_rhi->createShaderModule(SSR_FRAG);
        RHIShader* passthrough_frag_shader_module = m_rhi->createShaderModule(SSR_PASSTHROUGH_FRAG);

        RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
        vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
        vert_pipeline_shader_stage_create_info.module = vert_shader_module;
        vert_pipeline_shader_stage_create_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
        frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
        frag_pipeline_shader_stage_create_info.pName  = "main";

        RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
        vertex_input_state_create_info.sType                           = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
        vertex_input_state_create_info.pVertexBindingDescriptions      = nullptr;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_state_create_info.pVertexAttributeDescriptions    = nullptr;

        RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
        input_assembly_create_info.sType                  = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.topology               = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

        RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
        viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports    = m_rhi->getSwapchainInfo().viewport;
        viewport_state_create_info.scissorCount  = 1;
        viewport_state_create_info.pScissors     = m_rhi->getSwapchainInfo().scissor;

        RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
        rasterization_state_create_info.sType                   = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
        rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
        rasterization_state_create_info.lineWidth               = 1.0f;
        rasterization_state_create_info.cullMode                = RHI_CULL_MODE_BACK_BIT;
        rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        rasterization_state_create_info.depthBiasClamp          = 0.0f;
        rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

        RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
        multisample_state_create_info.sType                = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
        multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

        RHIPipelineColorBlendAttachmentState color_blend_attachment_state {};
        color_blend_attachment_state.colorWriteMask =
            RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT | RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
        color_blend_attachment_state.blendEnable         = RHI_FALSE;
        color_blend_attachment_state.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
        color_blend_attachment_state.colorBlendOp        = RHI_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
        color_blend_attachment_state.alphaBlendOp        = RHI_BLEND_OP_ADD;

        RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
        color_blend_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state_create_info.logicOpEnable     = RHI_FALSE;
        color_blend_state_create_info.logicOp           = RHI_LOGIC_OP_COPY;
        color_blend_state_create_info.attachmentCount   = 1;
        color_blend_state_create_info.pAttachments      = &color_blend_attachment_state;
        color_blend_state_create_info.blendConstants[0] = 0.0f;
        color_blend_state_create_info.blendConstants[1] = 0.0f;
        color_blend_state_create_info.blendConstants[2] = 0.0f;
        color_blend_state_create_info.blendConstants[3] = 0.0f;

        RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
        depth_stencil_create_info.sType                 = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_create_info.depthTestEnable       = RHI_TRUE;
        depth_stencil_create_info.depthWriteEnable      = RHI_TRUE;
        depth_stencil_create_info.depthCompareOp        = RHI_COMPARE_OP_LESS;
        depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
        depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

        RHIDynamicState dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};

        RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
        dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.dynamicStateCount = 2;
        dynamic_state_create_info.pDynamicStates    = dynamic_states;

        RHIGraphicsPipelineCreateInfo pipeline_info {};
        pipeline_info.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount          = 2;
        pipeline_info.pVertexInputState   = &vertex_input_state_create_info;
        pipeline_info.pInputAssemblyState = &input_assembly_create_info;
        pipeline_info.pViewportState      = &viewport_state_create_info;
        pipeline_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_info.pMultisampleState   = &multisample_state_create_info;
        pipeline_info.pColorBlendState    = &color_blend_state_create_info;
        pipeline_info.pDepthStencilState  = &depth_stencil_create_info;
        pipeline_info.renderPass          = m_framebuffer.render_pass;
        pipeline_info.subpass             = _main_camera_subpass_ssr;
        pipeline_info.basePipelineHandle  = RHI_NULL_HANDLE;
        pipeline_info.pDynamicState       = &dynamic_state_create_info;

        RHIShader* fragment_shader_modules[_render_pipeline_type_count] = {
            ssr_frag_shader_module,
            passthrough_frag_shader_module};

        for (uint8_t pipeline_index = 0; pipeline_index < _render_pipeline_type_count; ++pipeline_index)
        {
            frag_pipeline_shader_stage_create_info.module = fragment_shader_modules[pipeline_index];
            RHIPipelineShaderStageCreateInfo shader_stages[] = {
                vert_pipeline_shader_stage_create_info,
                frag_pipeline_shader_stage_create_info};

            pipeline_info.pStages = shader_stages;
            pipeline_info.layout  = m_render_pipelines[pipeline_index].layout;

            if (RHI_SUCCESS !=
                m_rhi->createGraphicsPipelines(
                    RHI_NULL_HANDLE, 1, &pipeline_info, m_render_pipelines[pipeline_index].pipeline))
            {
                throw std::runtime_error("create ssr graphics pipeline");
            }
        }

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(ssr_frag_shader_module);
        m_rhi->destroyShaderModule(passthrough_frag_shader_module);
    }

    void SSRPass::setupDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo ssr_global_descriptor_set_alloc_info {};
        ssr_global_descriptor_set_alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ssr_global_descriptor_set_alloc_info.pNext              = nullptr;
        ssr_global_descriptor_set_alloc_info.descriptorPool     = m_rhi->getDescriptorPoor();
        ssr_global_descriptor_set_alloc_info.descriptorSetCount = 1;
        ssr_global_descriptor_set_alloc_info.pSetLayouts        = &m_descriptor_infos[0].layout;

        if (RHI_SUCCESS !=
            m_rhi->allocateDescriptorSets(&ssr_global_descriptor_set_alloc_info, m_descriptor_infos[0].descriptor_set))
        {
            throw std::runtime_error("allocate ssr global descriptor set");
        }
    }

    void SSRPass::updateAfterFramebufferRecreate(RHIImageView* scene_color_input_attachment,
                                                 RHIImageView* gbuffer_metallic_roughness_input_attachment,
                                                 RHIImageView* gbuffer_position_input_attachment,
                                                 RHIImageView* gbuffer_normal_input_attachment)
    {
        RHIDescriptorImageInfo scene_color_input_attachment_info {};
        scene_color_input_attachment_info.sampler     = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Linear);
        scene_color_input_attachment_info.imageView   = scene_color_input_attachment;
        scene_color_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo gbuffer_metallic_roughness_input_attachment_info {};
        gbuffer_metallic_roughness_input_attachment_info.sampler = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
        gbuffer_metallic_roughness_input_attachment_info.imageView = gbuffer_metallic_roughness_input_attachment;
        gbuffer_metallic_roughness_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo gbuffer_position_input_attachment_info {};
        gbuffer_position_input_attachment_info.sampler     = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
        gbuffer_position_input_attachment_info.imageView   = gbuffer_position_input_attachment;
        gbuffer_position_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo gbuffer_normal_input_attachment_info {};
        gbuffer_normal_input_attachment_info.sampler     = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
        gbuffer_normal_input_attachment_info.imageView   = gbuffer_normal_input_attachment;
        gbuffer_normal_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorBufferInfo ssr_storage_buffer_info {};
        ssr_storage_buffer_info.offset = 0;
        ssr_storage_buffer_info.range  = sizeof(SSAOKernelObject);
        ssr_storage_buffer_info.buffer = m_global_render_resource->_storage_buffer._ssao_kernel_storage_buffer;

        RHIWriteDescriptorSet ssr_descriptor_writes_info[5];

        RHIWriteDescriptorSet& scene_color_descriptor_write_info = ssr_descriptor_writes_info[0];
        scene_color_descriptor_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        scene_color_descriptor_write_info.pNext           = nullptr;
        scene_color_descriptor_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        scene_color_descriptor_write_info.dstBinding      = 0;
        scene_color_descriptor_write_info.dstArrayElement = 0;
        scene_color_descriptor_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        scene_color_descriptor_write_info.descriptorCount = 1;
        scene_color_descriptor_write_info.pImageInfo      = &scene_color_input_attachment_info;

        RHIWriteDescriptorSet& gbuffer_metallic_roughness_descriptor_write_info = ssr_descriptor_writes_info[1];
        gbuffer_metallic_roughness_descriptor_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbuffer_metallic_roughness_descriptor_write_info.pNext           = nullptr;
        gbuffer_metallic_roughness_descriptor_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        gbuffer_metallic_roughness_descriptor_write_info.dstBinding      = 1;
        gbuffer_metallic_roughness_descriptor_write_info.dstArrayElement = 0;
        gbuffer_metallic_roughness_descriptor_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbuffer_metallic_roughness_descriptor_write_info.descriptorCount = 1;
        gbuffer_metallic_roughness_descriptor_write_info.pImageInfo = &gbuffer_metallic_roughness_input_attachment_info;

        RHIWriteDescriptorSet& gbuffer_position_descriptor_write_info = ssr_descriptor_writes_info[2];
        gbuffer_position_descriptor_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbuffer_position_descriptor_write_info.pNext           = nullptr;
        gbuffer_position_descriptor_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        gbuffer_position_descriptor_write_info.dstBinding      = 2;
        gbuffer_position_descriptor_write_info.dstArrayElement = 0;
        gbuffer_position_descriptor_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbuffer_position_descriptor_write_info.descriptorCount = 1;
        gbuffer_position_descriptor_write_info.pImageInfo      = &gbuffer_position_input_attachment_info;

        RHIWriteDescriptorSet& gbuffer_normal_descriptor_write_info = ssr_descriptor_writes_info[3];
        gbuffer_normal_descriptor_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbuffer_normal_descriptor_write_info.pNext           = nullptr;
        gbuffer_normal_descriptor_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        gbuffer_normal_descriptor_write_info.dstBinding      = 3;
        gbuffer_normal_descriptor_write_info.dstArrayElement = 0;
        gbuffer_normal_descriptor_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbuffer_normal_descriptor_write_info.descriptorCount = 1;
        gbuffer_normal_descriptor_write_info.pImageInfo      = &gbuffer_normal_input_attachment_info;

        RHIWriteDescriptorSet& ssr_storage_descriptor_write_info = ssr_descriptor_writes_info[4];
        ssr_storage_descriptor_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ssr_storage_descriptor_write_info.pNext           = nullptr;
        ssr_storage_descriptor_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        ssr_storage_descriptor_write_info.dstBinding      = 4;
        ssr_storage_descriptor_write_info.dstArrayElement = 0;
        ssr_storage_descriptor_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssr_storage_descriptor_write_info.descriptorCount = 1;
        ssr_storage_descriptor_write_info.pBufferInfo     = &ssr_storage_buffer_info;

        m_rhi->updateDescriptorSets(sizeof(ssr_descriptor_writes_info) / sizeof(ssr_descriptor_writes_info[0]),
                                    ssr_descriptor_writes_info,
                                    0,
                                    nullptr);
    }

    void SSRPass::preparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        const RenderResource* vulkan_resource = static_cast<const RenderResource*>(render_resource.get());
        if (vulkan_resource)
        {
            m_ssr_storage_buffer_object = vulkan_resource->m_ssao_kernel_storage_buffer_object;
        }

        m_ssr_storage_buffer_object.offset_x = static_cast<int>(m_rhi->getSwapchainInfo().viewport->x);
        m_ssr_storage_buffer_object.offset_y = static_cast<int>(m_rhi->getSwapchainInfo().viewport->y);
        m_ssr_storage_buffer_object.width    = static_cast<int>(m_rhi->getSwapchainInfo().viewport->width);
        m_ssr_storage_buffer_object.height   = static_cast<int>(m_rhi->getSwapchainInfo().viewport->height);
    }

    void SSRPass::draw()
    {
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "SSR", color);

        const uint8_t pipeline_index =
            m_enable_ssr ? _render_pipeline_type_ssr : _render_pipeline_type_passthrough;

        m_rhi->cmdBindPipelinePFN(
            m_rhi->getCurrentCommandBuffer(),
            RHI_PIPELINE_BIND_POINT_GRAPHICS,
            m_render_pipelines[pipeline_index].pipeline);
        m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().viewport);
        m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().scissor);
        m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                        m_render_pipelines[pipeline_index].layout,
                                        0,
                                        1,
                                        &m_descriptor_infos[0].descriptor_set,
                                        0,
                                        nullptr);

        (*reinterpret_cast<SSAOKernelObject*>(reinterpret_cast<uintptr_t>(
            m_global_render_resource->_storage_buffer._ssao_kernel_storage_buffer_memory_pointer))) =
            m_ssr_storage_buffer_object;

        m_rhi->cmdDraw(m_rhi->getCurrentCommandBuffer(), 3, 1, 0, 0);

        m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
    }
} // namespace Compass
