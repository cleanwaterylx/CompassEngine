#include "runtime/function/render/passes/depth_linear_pass.h"

#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/render_mesh.h"
#include "runtime/function/render/render_resource.h"

#include <mesh_depth_linear_frag.h>
#include <mesh_vert.h>

#include <map>
#include <stdexcept>

namespace Compass
{
    void DepthLinearPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const DepthLinearPassInitInfo* _init_info = static_cast<const DepthLinearPassInitInfo*>(init_info);
        m_per_mesh_layout                           = _init_info->per_mesh_layout;

        setupAttachments();
        setupRenderPass();
        setupFramebuffer();
        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
    }

    void DepthLinearPass::preparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        const RenderResource* vulkan_resource = static_cast<const RenderResource*>(render_resource.get());
        if (vulkan_resource)
        {
            m_mesh_perframe_storage_buffer_object = vulkan_resource->m_mesh_perframe_storage_buffer_object;
        }
    }

    void DepthLinearPass::draw()
    {
        drawModel();
    }

    void DepthLinearPass::updateAfterFramebufferRecreate()
    {
        m_rhi->destroyImage(m_framebuffer.attachments[0].image);
        m_rhi->destroyImageView(m_framebuffer.attachments[0].view);
        m_rhi->freeMemory(m_framebuffer.attachments[0].mem);
        m_rhi->destroyFramebuffer(m_framebuffer.framebuffer);

        setupAttachments();
        setupFramebuffer();
    }

    void DepthLinearPass::setupAttachments()
    {
        m_framebuffer.attachments.resize(1);
        m_framebuffer.attachments[0].format = RHI_FORMAT_R32_SFLOAT;

        m_rhi->createImage(m_rhi->getSwapchainInfo().extent.width,
                           m_rhi->getSwapchainInfo().extent.height,
                           m_framebuffer.attachments[0].format,
                           RHI_IMAGE_TILING_OPTIMAL,
                           RHI_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | RHI_IMAGE_USAGE_SAMPLED_BIT,
                           RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           m_framebuffer.attachments[0].image,
                           m_framebuffer.attachments[0].mem,
                           0,
                           1,
                           1);
        m_rhi->createImageView(m_framebuffer.attachments[0].image,
                               m_framebuffer.attachments[0].format,
                               RHI_IMAGE_ASPECT_COLOR_BIT,
                               RHI_IMAGE_VIEW_TYPE_2D,
                               1,
                               1,
                               m_framebuffer.attachments[0].view);
    }

    void DepthLinearPass::setupRenderPass()
    {
        RHIAttachmentDescription attachments[2] = {};

        RHIAttachmentDescription& color_attachment_description = attachments[0];
        color_attachment_description.format         = m_framebuffer.attachments[0].format;
        color_attachment_description.samples        = RHI_SAMPLE_COUNT_1_BIT;
        color_attachment_description.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_description.storeOp        = RHI_ATTACHMENT_STORE_OP_STORE;
        color_attachment_description.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment_description.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment_description.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        color_attachment_description.finalLayout    = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIAttachmentDescription& depth_attachment_description = attachments[1];
        depth_attachment_description.format         = m_rhi->getDepthImageInfo().depth_image_format;
        depth_attachment_description.samples        = RHI_SAMPLE_COUNT_1_BIT;
        depth_attachment_description.loadOp         = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_description.storeOp        = RHI_ATTACHMENT_STORE_OP_STORE;
        depth_attachment_description.stencilLoadOp  = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment_description.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_description.initialLayout  = RHI_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment_description.finalLayout    = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RHIAttachmentReference color_attachment_reference {};
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout     = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RHIAttachmentReference depth_attachment_reference {};
        depth_attachment_reference.attachment = 1;
        depth_attachment_reference.layout     = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RHISubpassDescription subpass {};
        subpass.pipelineBindPoint       = RHI_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &color_attachment_reference;
        subpass.pDepthStencilAttachment = &depth_attachment_reference;

        RHISubpassDependency dependency {};
        dependency.srcSubpass      = 0;
        dependency.dstSubpass      = RHI_SUBPASS_EXTERNAL;
        dependency.srcStageMask    = RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask    = RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT | RHI_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask   = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask   = RHI_ACCESS_SHADER_READ_BIT;
        dependency.dependencyFlags = 0;

        RHIRenderPassCreateInfo renderpass_create_info {};
        renderpass_create_info.sType           = RHI_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
        renderpass_create_info.pAttachments    = attachments;
        renderpass_create_info.subpassCount    = 1;
        renderpass_create_info.pSubpasses      = &subpass;
        renderpass_create_info.dependencyCount = 1;
        renderpass_create_info.pDependencies   = &dependency;

        if (RHI_SUCCESS != m_rhi->createRenderPass(&renderpass_create_info, m_framebuffer.render_pass))
        {
            throw std::runtime_error("create depth linear render pass");
        }
    }

    void DepthLinearPass::setupFramebuffer()
    {
        RHIImageView* attachments[2] = {m_framebuffer.attachments[0].view, m_rhi->getDepthImageInfo().depth_image_view};

        RHIFramebufferCreateInfo framebuffer_create_info {};
        framebuffer_create_info.sType           = RHI_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass      = m_framebuffer.render_pass;
        framebuffer_create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
        framebuffer_create_info.pAttachments    = attachments;
        framebuffer_create_info.width           = m_rhi->getSwapchainInfo().extent.width;
        framebuffer_create_info.height          = m_rhi->getSwapchainInfo().extent.height;
        framebuffer_create_info.layers          = 1;

        if (RHI_SUCCESS != m_rhi->createFramebuffer(&framebuffer_create_info, m_framebuffer.framebuffer))
        {
            throw std::runtime_error("create depth linear framebuffer");
        }
    }

    void DepthLinearPass::setupDescriptorSetLayout()
    {
        m_descriptor_infos.resize(1);

        RHIDescriptorSetLayoutBinding mesh_global_layout_bindings[3] = {};

        mesh_global_layout_bindings[0].binding            = 0;
        mesh_global_layout_bindings[0].descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_global_layout_bindings[0].descriptorCount    = 1;
        mesh_global_layout_bindings[0].stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
        mesh_global_layout_bindings[0].pImmutableSamplers = nullptr;

        mesh_global_layout_bindings[1].binding            = 1;
        mesh_global_layout_bindings[1].descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_global_layout_bindings[1].descriptorCount    = 1;
        mesh_global_layout_bindings[1].stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
        mesh_global_layout_bindings[1].pImmutableSamplers = nullptr;

        mesh_global_layout_bindings[2].binding            = 2;
        mesh_global_layout_bindings[2].descriptorType     = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_global_layout_bindings[2].descriptorCount    = 1;
        mesh_global_layout_bindings[2].stageFlags         = RHI_SHADER_STAGE_VERTEX_BIT;
        mesh_global_layout_bindings[2].pImmutableSamplers = nullptr;

        RHIDescriptorSetLayoutCreateInfo layout_create_info {};
        layout_create_info.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_create_info.bindingCount = sizeof(mesh_global_layout_bindings) / sizeof(mesh_global_layout_bindings[0]);
        layout_create_info.pBindings    = mesh_global_layout_bindings;

        if (RHI_SUCCESS != m_rhi->createDescriptorSetLayout(&layout_create_info, m_descriptor_infos[0].layout))
        {
            throw std::runtime_error("create depth linear global layout");
        }
    }

    void DepthLinearPass::setupPipelines()
    {
        m_render_pipelines.resize(1);

        RHIDescriptorSetLayout* descriptorset_layouts[] = {m_descriptor_infos[0].layout, m_per_mesh_layout};

        RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = sizeof(descriptorset_layouts) / sizeof(descriptorset_layouts[0]);
        pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

        if (RHI_SUCCESS != m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[0].layout))
        {
            throw std::runtime_error("create depth linear pipeline layout");
        }

        RHIShader* vert_shader_module = m_rhi->createShaderModule(MESH_VERT);
        RHIShader* frag_shader_module = m_rhi->createShaderModule(MESH_DEPTH_LINEAR_FRAG);

        RHIPipelineShaderStageCreateInfo vert_stage_create_info {};
        vert_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_stage_create_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
        vert_stage_create_info.module = vert_shader_module;
        vert_stage_create_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo frag_stage_create_info {};
        frag_stage_create_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_stage_create_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
        frag_stage_create_info.module = frag_shader_module;
        frag_stage_create_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_create_info, frag_stage_create_info};

        auto vertex_binding_descriptions   = MeshVertex::getBindingDescriptions();
        auto vertex_attribute_descriptions = MeshVertex::getAttributeDescriptions();

        RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
        vertex_input_state_create_info.sType                           = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount   = vertex_binding_descriptions.size();
        vertex_input_state_create_info.pVertexBindingDescriptions      = vertex_binding_descriptions.data();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
        vertex_input_state_create_info.pVertexAttributeDescriptions    = vertex_attribute_descriptions.data();

        RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
        input_assembly_create_info.sType                  = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.topology               = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
        rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;

        RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
        multisample_state_create_info.sType                = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
        multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

        RHIPipelineColorBlendAttachmentState color_blend_attachment_state {};
        color_blend_attachment_state.colorWriteMask      = RHI_COLOR_COMPONENT_R_BIT;
        color_blend_attachment_state.blendEnable         = RHI_FALSE;
        color_blend_attachment_state.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
        color_blend_attachment_state.colorBlendOp        = RHI_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
        color_blend_attachment_state.alphaBlendOp        = RHI_BLEND_OP_ADD;

        RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
        color_blend_state_create_info.sType           = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state_create_info.logicOpEnable   = RHI_FALSE;
        color_blend_state_create_info.logicOp         = RHI_LOGIC_OP_COPY;
        color_blend_state_create_info.attachmentCount = 1;
        color_blend_state_create_info.pAttachments    = &color_blend_attachment_state;

        RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
        depth_stencil_create_info.sType            = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_create_info.depthTestEnable  = RHI_TRUE;
        depth_stencil_create_info.depthWriteEnable = RHI_TRUE;
        depth_stencil_create_info.depthCompareOp   = RHI_COMPARE_OP_LESS;
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
        pipeline_info.pStages             = shader_stages;
        pipeline_info.pVertexInputState   = &vertex_input_state_create_info;
        pipeline_info.pInputAssemblyState = &input_assembly_create_info;
        pipeline_info.pViewportState      = &viewport_state_create_info;
        pipeline_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_info.pMultisampleState   = &multisample_state_create_info;
        pipeline_info.pColorBlendState    = &color_blend_state_create_info;
        pipeline_info.pDepthStencilState  = &depth_stencil_create_info;
        pipeline_info.layout              = m_render_pipelines[0].layout;
        pipeline_info.renderPass          = m_framebuffer.render_pass;
        pipeline_info.subpass             = 0;
        pipeline_info.basePipelineHandle  = RHI_NULL_HANDLE;
        pipeline_info.pDynamicState       = &dynamic_state_create_info;

        if (RHI_SUCCESS != m_rhi->createGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipeline_info, m_render_pipelines[0].pipeline))
        {
            throw std::runtime_error("create depth linear graphics pipeline");
        }

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);
    }

    void DepthLinearPass::setupDescriptorSet()
    {
        RHIDescriptorSetAllocateInfo descriptor_set_alloc_info {};
        descriptor_set_alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_alloc_info.descriptorPool     = m_rhi->getDescriptorPoor();
        descriptor_set_alloc_info.descriptorSetCount = 1;
        descriptor_set_alloc_info.pSetLayouts        = &m_descriptor_infos[0].layout;

        if (RHI_SUCCESS != m_rhi->allocateDescriptorSets(&descriptor_set_alloc_info, m_descriptor_infos[0].descriptor_set))
        {
            throw std::runtime_error("allocate depth linear descriptor set");
        }

        RHIDescriptorBufferInfo perframe_storage_buffer_info {};
        perframe_storage_buffer_info.offset = 0;
        perframe_storage_buffer_info.range  = sizeof(MeshPerframeStorageBufferObject);
        perframe_storage_buffer_info.buffer = m_global_render_resource->_storage_buffer._global_upload_ringbuffer;

        RHIDescriptorBufferInfo perdrawcall_storage_buffer_info {};
        perdrawcall_storage_buffer_info.offset = 0;
        perdrawcall_storage_buffer_info.range  = sizeof(MeshPerdrawcallStorageBufferObject);
        perdrawcall_storage_buffer_info.buffer = m_global_render_resource->_storage_buffer._global_upload_ringbuffer;

        RHIDescriptorBufferInfo perdrawcall_vertex_blending_storage_buffer_info {};
        perdrawcall_vertex_blending_storage_buffer_info.offset = 0;
        perdrawcall_vertex_blending_storage_buffer_info.range  = sizeof(MeshPerdrawcallVertexBlendingStorageBufferObject);
        perdrawcall_vertex_blending_storage_buffer_info.buffer = m_global_render_resource->_storage_buffer._global_upload_ringbuffer;

        RHIWriteDescriptorSet descriptor_writes[3] = {};

        descriptor_writes[0].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet          = m_descriptor_infos[0].descriptor_set;
        descriptor_writes[0].dstBinding      = 0;
        descriptor_writes[0].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo     = &perframe_storage_buffer_info;

        descriptor_writes[1].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet          = m_descriptor_infos[0].descriptor_set;
        descriptor_writes[1].dstBinding      = 1;
        descriptor_writes[1].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo     = &perdrawcall_storage_buffer_info;

        descriptor_writes[2].sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[2].dstSet          = m_descriptor_infos[0].descriptor_set;
        descriptor_writes[2].dstBinding      = 2;
        descriptor_writes[2].descriptorType  = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        descriptor_writes[2].descriptorCount = 1;
        descriptor_writes[2].pBufferInfo     = &perdrawcall_vertex_blending_storage_buffer_info;

        m_rhi->updateDescriptorSets(sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
    }

    void DepthLinearPass::drawModel()
    {
        struct MeshNode
        {
            const Matrix4x4* model_matrix {nullptr};
            const Matrix4x4* joint_matrices {nullptr};
            uint32_t         joint_count {0};
        };

        std::map<VulkanMesh*, std::vector<MeshNode>> drawcall_batch;

        for (RenderMeshNode& node : *(m_visible_nodes.p_main_camera_visible_mesh_nodes))
        {
            auto& mesh_nodes = drawcall_batch[node.ref_mesh];

            MeshNode temp;
            temp.model_matrix = node.model_matrix;
            if (node.enable_vertex_blending)
            {
                temp.joint_matrices = node.joint_matrices;
                temp.joint_count    = node.joint_count;
            }

            mesh_nodes.push_back(temp);
        }

        RHIRenderPassBeginInfo renderpass_begin_info {};
        renderpass_begin_info.sType             = RHI_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpass_begin_info.renderPass        = m_framebuffer.render_pass;
        renderpass_begin_info.framebuffer       = m_framebuffer.framebuffer;
        renderpass_begin_info.renderArea.offset = {0, 0};
        renderpass_begin_info.renderArea.extent = m_rhi->getSwapchainInfo().extent;

        RHIClearValue clear_values[2] {};
        clear_values[0].color        = {{1000000.0f, 0.0f, 0.0f, 0.0f}};
        clear_values[1].depthStencil = {1.0f, 0};
        renderpass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]);
        renderpass_begin_info.pClearValues    = clear_values;

        m_rhi->cmdBeginRenderPassPFN(m_rhi->getCurrentCommandBuffer(), &renderpass_begin_info, RHI_SUBPASS_CONTENTS_INLINE);

        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "Depth Linear", color);

        m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, m_render_pipelines[0].pipeline);
        m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().viewport);
        m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().scissor);

        uint32_t perframe_dynamic_offset =
            roundUp(m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()],
                    m_global_render_resource->_storage_buffer._min_storage_buffer_offset_alignment);

        m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()] =
            perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
        assert(m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()] <=
               (m_global_render_resource->_storage_buffer._global_upload_ringbuffers_begin[m_rhi->getCurrentFrameIndex()] +
                m_global_render_resource->_storage_buffer._global_upload_ringbuffers_size[m_rhi->getCurrentFrameIndex()]));

        (*reinterpret_cast<MeshPerframeStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(m_global_render_resource->_storage_buffer._global_upload_ringbuffer_memory_pointer) +
            perframe_dynamic_offset)) = m_mesh_perframe_storage_buffer_object;

        for (auto& pair : drawcall_batch)
        {
            VulkanMesh& mesh       = (*pair.first);
            auto&       mesh_nodes = pair.second;

            uint32_t total_instance_count = static_cast<uint32_t>(mesh_nodes.size());
            if (total_instance_count == 0)
            {
                continue;
            }

            m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                            RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                            m_render_pipelines[0].layout,
                                            1,
                                            1,
                                            &mesh.mesh_vertex_blending_descriptor_set,
                                            0,
                                            nullptr);

            RHIBuffer* vertex_buffers[] = {
                mesh.mesh_vertex_position_buffer,
                mesh.mesh_vertex_varying_enable_blending_buffer,
                mesh.mesh_vertex_varying_buffer};
            RHIDeviceSize offsets[] = {0, 0, 0};
            m_rhi->cmdBindVertexBuffersPFN(m_rhi->getCurrentCommandBuffer(),
                                           0,
                                           sizeof(vertex_buffers) / sizeof(vertex_buffers[0]),
                                           vertex_buffers,
                                           offsets);
            m_rhi->cmdBindIndexBufferPFN(m_rhi->getCurrentCommandBuffer(), mesh.mesh_index_buffer, 0, RHI_INDEX_TYPE_UINT16);

            uint32_t drawcall_max_instance_count =
                sizeof(MeshPerdrawcallStorageBufferObject::mesh_instances) /
                sizeof(MeshPerdrawcallStorageBufferObject::mesh_instances[0]);
            uint32_t drawcall_count =
                roundUp(total_instance_count, drawcall_max_instance_count) / drawcall_max_instance_count;

            for (uint32_t drawcall_index = 0; drawcall_index < drawcall_count; ++drawcall_index)
            {
                uint32_t current_instance_count =
                    ((total_instance_count - drawcall_max_instance_count * drawcall_index) < drawcall_max_instance_count) ?
                        (total_instance_count - drawcall_max_instance_count * drawcall_index) :
                        drawcall_max_instance_count;

                uint32_t perdrawcall_dynamic_offset =
                    roundUp(m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()],
                            m_global_render_resource->_storage_buffer._min_storage_buffer_offset_alignment);
                m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()] =
                    perdrawcall_dynamic_offset + sizeof(MeshPerdrawcallStorageBufferObject);
                assert(m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()] <=
                       (m_global_render_resource->_storage_buffer._global_upload_ringbuffers_begin[m_rhi->getCurrentFrameIndex()] +
                        m_global_render_resource->_storage_buffer._global_upload_ringbuffers_size[m_rhi->getCurrentFrameIndex()]));

                MeshPerdrawcallStorageBufferObject& perdrawcall_storage_buffer_object =
                    (*reinterpret_cast<MeshPerdrawcallStorageBufferObject*>(
                        reinterpret_cast<uintptr_t>(m_global_render_resource->_storage_buffer._global_upload_ringbuffer_memory_pointer) +
                        perdrawcall_dynamic_offset));

                for (uint32_t i = 0; i < current_instance_count; ++i)
                {
                    perdrawcall_storage_buffer_object.mesh_instances[i].model_matrix =
                        *mesh_nodes[drawcall_max_instance_count * drawcall_index + i].model_matrix;
                    perdrawcall_storage_buffer_object.mesh_instances[i].enable_vertex_blending =
                        mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices ? 1.0f : -1.0f;
                }

                uint32_t per_drawcall_vertex_blending_dynamic_offset = 0;
                bool     least_one_enable_vertex_blending            = true;
                for (uint32_t i = 0; i < current_instance_count; ++i)
                {
                    if (!mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices)
                    {
                        least_one_enable_vertex_blending = false;
                        break;
                    }
                }

                if (least_one_enable_vertex_blending)
                {
                    per_drawcall_vertex_blending_dynamic_offset =
                        roundUp(m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()],
                                m_global_render_resource->_storage_buffer._min_storage_buffer_offset_alignment);
                    m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()] =
                        per_drawcall_vertex_blending_dynamic_offset + sizeof(MeshPerdrawcallVertexBlendingStorageBufferObject);
                    assert(m_global_render_resource->_storage_buffer._global_upload_ringbuffers_end[m_rhi->getCurrentFrameIndex()] <=
                           (m_global_render_resource->_storage_buffer._global_upload_ringbuffers_begin[m_rhi->getCurrentFrameIndex()] +
                            m_global_render_resource->_storage_buffer._global_upload_ringbuffers_size[m_rhi->getCurrentFrameIndex()]));

                    MeshPerdrawcallVertexBlendingStorageBufferObject& per_drawcall_vertex_blending_storage_buffer_object =
                        (*reinterpret_cast<MeshPerdrawcallVertexBlendingStorageBufferObject*>(
                            reinterpret_cast<uintptr_t>(m_global_render_resource->_storage_buffer._global_upload_ringbuffer_memory_pointer) +
                            per_drawcall_vertex_blending_dynamic_offset));

                    for (uint32_t i = 0; i < current_instance_count; ++i)
                    {
                        if (mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices)
                        {
                            for (uint32_t j = 0; j < mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_count; ++j)
                            {
                                per_drawcall_vertex_blending_storage_buffer_object
                                    .joint_matrices[s_mesh_vertex_blending_max_joint_count * i + j] =
                                    mesh_nodes[drawcall_max_instance_count * drawcall_index + i].joint_matrices[j];
                            }
                        }
                    }
                }

                uint32_t dynamic_offsets[3] = {
                    perframe_dynamic_offset,
                    perdrawcall_dynamic_offset,
                    per_drawcall_vertex_blending_dynamic_offset};

                m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                                RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                                m_render_pipelines[0].layout,
                                                0,
                                                1,
                                                &m_descriptor_infos[0].descriptor_set,
                                                3,
                                                dynamic_offsets);

                m_rhi->cmdDrawIndexedPFN(m_rhi->getCurrentCommandBuffer(),
                                         mesh.mesh_index_count,
                                         current_instance_count,
                                         0,
                                         0,
                                         0);
            }
        }

        m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
        m_rhi->cmdEndRenderPassPFN(m_rhi->getCurrentCommandBuffer());
    }
} // namespace Compass
