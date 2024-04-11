#include "runtime/function/render/passes/ssao_blur_pass.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"

#include <ssao_blur_frag.h>
#include <ssao_vert.h>

#include <stdexcept>
#include <random>
#include "ssao_pass.h"

namespace Compass
{
    void SSAOBlurPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const SSAOBlurPassInitInfo* _init_info = static_cast<const SSAOBlurPassInitInfo*>(init_info);

        // ssao_pass.renderpass = main_camera.renderpass
        m_framebuffer.render_pass                  = _init_info->render_pass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        updateAfterFramebufferRecreate(_init_info->input_attachment);
    }

    void SSAOBlurPass::setupDescriptorSetLayout()
    {
        m_descriptor_infos.resize(1);

        RHIDescriptorSetLayoutBinding ssao_global_layout_bindings[2] = {};
        // ssao blur_object

        RHIDescriptorSetLayoutBinding& ssao_attachment_binding =
            ssao_global_layout_bindings[0];
        ssao_attachment_binding.binding         = 0;
        ssao_attachment_binding.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        ssao_attachment_binding.descriptorCount = 1;
        ssao_attachment_binding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutBinding& ssao_kernel_layout_binding = ssao_global_layout_bindings[1];
        ssao_kernel_layout_binding.binding         = 1;
        ssao_kernel_layout_binding.descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssao_kernel_layout_binding.descriptorCount = 1;
        ssao_kernel_layout_binding.stageFlags      = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutCreateInfo ssao_global_layout_create_info;
        ssao_global_layout_create_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ssao_global_layout_create_info.pNext = NULL;
        ssao_global_layout_create_info.flags = 0;
        ssao_global_layout_create_info.bindingCount =
            sizeof(ssao_global_layout_bindings) / sizeof(ssao_global_layout_bindings[0]);
        ssao_global_layout_create_info.pBindings = ssao_global_layout_bindings;

        if (RHI_SUCCESS != m_rhi->createDescriptorSetLayout(&ssao_global_layout_create_info, m_descriptor_infos[0].layout))
        {
            throw std::runtime_error("create ssao global layout");
        }
    }

    void SSAOBlurPass::setupPipelines()
    {
        m_render_pipelines.resize(1);

        RHIDescriptorSetLayout*      descriptorset_layouts[1] = {m_descriptor_infos[0].layout};
        RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

        if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[0].layout) != RHI_SUCCESS)
        {
            throw std::runtime_error("create ssao pipeline layout");
        }

        RHIShader* vert_shader_module = m_rhi->createShaderModule(SSAO_VERT);
        RHIShader* frag_shader_module = m_rhi->createShaderModule(SSAO_BLUR_FRAG);

        RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info {};
        vert_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
        vert_pipeline_shader_stage_create_info.module = vert_shader_module;
        vert_pipeline_shader_stage_create_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info {};
        frag_pipeline_shader_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_pipeline_shader_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
        frag_pipeline_shader_stage_create_info.module = frag_shader_module;
        frag_pipeline_shader_stage_create_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_pipeline_shader_stage_create_info,
                                                           frag_pipeline_shader_stage_create_info};

        RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
        vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
        vertex_input_state_create_info.pVertexBindingDescriptions      = NULL;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_state_create_info.pVertexAttributeDescriptions    = NULL;

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
        rasterization_state_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.depthClampEnable = RHI_FALSE;
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

        // 片段着色器输出具体的颜色，它需要与帧缓冲区framebuffer中已经存在的颜色进行混合
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

        RHIGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = 2;
        pipelineInfo.pStages             = shader_stages;
        pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
        pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
        pipelineInfo.pViewportState      = &viewport_state_create_info;
        pipelineInfo.pRasterizationState = &rasterization_state_create_info;
        pipelineInfo.pMultisampleState   = &multisample_state_create_info;
        pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
        pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
        pipelineInfo.layout              = m_render_pipelines[0].layout;
        pipelineInfo.renderPass          = m_framebuffer.render_pass;
        pipelineInfo.subpass             = _main_camera_subpass_ssao_blur;
        pipelineInfo.basePipelineHandle  = RHI_NULL_HANDLE;
        pipelineInfo.pDynamicState       = &dynamic_state_create_info;

        if (RHI_SUCCESS != m_rhi->createGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipelineInfo, m_render_pipelines[0].pipeline))
        {
            throw std::runtime_error("create post process graphics pipeline");
        }

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);
    }

    void SSAOBlurPass::setupDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo ssao_global_descriptor_set_alloc_info;
        ssao_global_descriptor_set_alloc_info.sType          = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ssao_global_descriptor_set_alloc_info.pNext          = NULL;
        ssao_global_descriptor_set_alloc_info.descriptorPool = m_rhi->getDescriptorPoor();
        ssao_global_descriptor_set_alloc_info.descriptorSetCount = 1;
        ssao_global_descriptor_set_alloc_info.pSetLayouts        = &m_descriptor_infos[0].layout;

        if (RHI_SUCCESS != m_rhi->allocateDescriptorSets(&ssao_global_descriptor_set_alloc_info, m_descriptor_infos[0].descriptor_set))
        {
            throw std::runtime_error("allocate post process global descriptor set");
        }
    }

    // 实际为descriptor bind buffer sample 
    void SSAOBlurPass::updateAfterFramebufferRecreate(RHIImageView *input_attachment)
    {
        RHIDescriptorImageInfo ssao_input_attachment_info = {};
        ssao_input_attachment_info.sampler =
            m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
        ssao_input_attachment_info.imageView   = input_attachment;
        ssao_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorBufferInfo ssao_kernel_buffer_info = {};
        ssao_kernel_buffer_info.offset = 0;
        ssao_kernel_buffer_info.range = sizeof(SSAOKernelObject);
        ssao_kernel_buffer_info.buffer = m_global_render_resource->_storage_buffer._ssao_kernel_storage_buffer;

        RHIWriteDescriptorSet ssao_blur_descriptor_writes_info[2];

        RHIWriteDescriptorSet& ssao_blur_descriptor_ssao_write_info = ssao_blur_descriptor_writes_info[0];
        ssao_blur_descriptor_ssao_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ssao_blur_descriptor_ssao_write_info.pNext           = NULL;
        ssao_blur_descriptor_ssao_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        ssao_blur_descriptor_ssao_write_info.dstBinding      = 0;
        ssao_blur_descriptor_ssao_write_info.dstArrayElement = 0;
        ssao_blur_descriptor_ssao_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        ssao_blur_descriptor_ssao_write_info.descriptorCount = 1;
        ssao_blur_descriptor_ssao_write_info.pImageInfo = &ssao_input_attachment_info;

        RHIWriteDescriptorSet& ssao_descriptor_kernel_write_info = ssao_blur_descriptor_writes_info[1];
        ssao_descriptor_kernel_write_info.sType                 = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ssao_descriptor_kernel_write_info.pNext                 = NULL;
        ssao_descriptor_kernel_write_info.dstSet                = m_descriptor_infos[0].descriptor_set;
        ssao_descriptor_kernel_write_info.dstBinding            = 1;
        ssao_descriptor_kernel_write_info.dstArrayElement       = 0;
        ssao_descriptor_kernel_write_info.descriptorType        = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssao_descriptor_kernel_write_info.descriptorCount       = 1;
        ssao_descriptor_kernel_write_info.pBufferInfo           = &ssao_kernel_buffer_info;

        m_rhi->updateDescriptorSets(sizeof(ssao_blur_descriptor_writes_info) /
                                    sizeof(ssao_blur_descriptor_writes_info[0]),
                                    ssao_blur_descriptor_writes_info,
                                    0,
                                    NULL);
    }

    void SSAOBlurPass::preparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        const RenderResource* vulkan_resource = static_cast<const RenderResource*>(render_resource.get());
        if (vulkan_resource)
        {
            m_ssao_kernel_storage_buffer_object = vulkan_resource->m_ssao_kernel_storage_buffer_object;
        }

        m_ssao_kernel_storage_buffer_object.offset_x = m_rhi->getSwapchainInfo().viewport->x;
        m_ssao_kernel_storage_buffer_object.offset_y = m_rhi->getSwapchainInfo().viewport->y;
        m_ssao_kernel_storage_buffer_object.width = m_rhi->getSwapchainInfo().viewport->width;
        m_ssao_kernel_storage_buffer_object.height = m_rhi->getSwapchainInfo().viewport->height;
        
    }


    void SSAOBlurPass::draw()
    {
        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "SSAO Blur", color);

        m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, m_render_pipelines[0].pipeline);
        m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().viewport);
        m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().scissor);
        m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                        m_render_pipelines[0].layout,
                                        0,
                                        1,
                                        &m_descriptor_infos[0].descriptor_set,
                                        0,
                                        NULL);

        // set ssao kernel storage buffer data
        (*reinterpret_cast<SSAOKernelObject*>(reinterpret_cast<uintptr_t>(
            m_global_render_resource->_storage_buffer._ssao_kernel_storage_buffer_memory_pointer))) = 
            m_ssao_kernel_storage_buffer_object;

        m_rhi->cmdDraw(m_rhi->getCurrentCommandBuffer(), 3, 1, 0, 0);

        m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
    }

} // namespace Compass
