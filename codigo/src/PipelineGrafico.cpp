#include "PipelineGrafico.hpp"

PipelineGrafico::PipelineGrafico(const DispositivoVulkan& dispositivo,
                                 const GerenciadorSwapchain& swapchain,
                                 VkShaderModule moduloVertexShader,
                                 VkShaderModule moduloFragmentShader) {
    dispositivoLogicoReferencia = dispositivo.obterDispositivoLogico();

    criarRenderPass(dispositivo, swapchain.obterFormatoImagem());
    criarLayoutPipeline(dispositivoLogicoReferencia);
    criarPipelineGrafico(dispositivoLogicoReferencia, moduloVertexShader, moduloFragmentShader);
    criarFramebuffers(dispositivoLogicoReferencia, swapchain);
}

PipelineGrafico::~PipelineGrafico() {
    limparFramebuffers(dispositivoLogicoReferencia);

    if (pipelineGrafico != VK_NULL_HANDLE) {
        vkDestroyPipeline(dispositivoLogicoReferencia, pipelineGrafico, nullptr);
        pipelineGrafico = VK_NULL_HANDLE;
    }
    if (layoutPipeline != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(dispositivoLogicoReferencia, layoutPipeline, nullptr);
        layoutPipeline = VK_NULL_HANDLE;
    }
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(dispositivoLogicoReferencia, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}

void PipelineGrafico::limparFramebuffers(VkDevice dispositivoLogico) {
    for (auto framebuffer : framebuffersSwapchain) {
        vkDestroyFramebuffer(dispositivoLogico, framebuffer, nullptr);
    }
    framebuffersSwapchain.clear();
}

void PipelineGrafico::recriarFramebuffers(const DispositivoVulkan& dispositivo, const GerenciadorSwapchain& swapchain) {
    limparFramebuffers(dispositivo.obterDispositivoLogico());
    criarFramebuffers(dispositivo.obterDispositivoLogico(), swapchain);
}

void PipelineGrafico::criarRenderPass(const DispositivoVulkan& dispositivo, VkFormat formatoCor) {
    VkAttachmentDescription anexoCor{};
    anexoCor.format = formatoCor;
    anexoCor.samples = VK_SAMPLE_COUNT_1_BIT;
    anexoCor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    anexoCor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    anexoCor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    anexoCor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    anexoCor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    anexoCor.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference referenciaAnexoCor{};
    referenciaAnexoCor.attachment = 0;
    referenciaAnexoCor.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &referenciaAnexoCor;

    VkSubpassDependency dependencia{};
    dependencia.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencia.dstSubpass = 0;
    dependencia.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencia.srcAccessMask = 0;
    dependencia.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencia.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo infoRenderPass{};
    infoRenderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    infoRenderPass.attachmentCount = 1;
    infoRenderPass.pAttachments = &anexoCor;
    infoRenderPass.subpassCount = 1;
    infoRenderPass.pSubpasses = &subpass;
    infoRenderPass.dependencyCount = 1;
    infoRenderPass.pDependencies = &dependencia;

    verificarResultadoVulkan(
        vkCreateRenderPass(dispositivo.obterDispositivoLogico(), &infoRenderPass, nullptr, &renderPass),
        "Falha ao criar o Render Pass do pipeline gráfico."
    );
}

void PipelineGrafico::criarLayoutPipeline(VkDevice dispositivoLogico) {
    VkPipelineLayoutCreateInfo infoLayout{};
    infoLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    infoLayout.setLayoutCount = 0;
    infoLayout.pSetLayouts = nullptr;
    infoLayout.pushConstantRangeCount = 0;
    infoLayout.pPushConstantRanges = nullptr;

    verificarResultadoVulkan(
        vkCreatePipelineLayout(dispositivoLogico, &infoLayout, nullptr, &layoutPipeline),
        "Falha ao criar o layout do pipeline gráfico (VkPipelineLayout)."
    );
}

void PipelineGrafico::criarPipelineGrafico(VkDevice dispositivoLogico,
                                          VkShaderModule moduloVertexShader,
                                          VkShaderModule moduloFragmentShader) {
    // Configuração dos estágios programáveis de shaders
    VkPipelineShaderStageCreateInfo estagioVertex{};
    estagioVertex.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    estagioVertex.stage = VK_SHADER_STAGE_VERTEX_BIT;
    estagioVertex.module = moduloVertexShader;
    estagioVertex.pName = "main";

    VkPipelineShaderStageCreateInfo estagioFragment{};
    estagioFragment.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    estagioFragment.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    estagioFragment.module = moduloFragmentShader;
    estagioFragment.pName = "main";

    VkPipelineShaderStageCreateInfo estagiosShaders[] = { estagioVertex, estagioFragment };

    // Configuração dos vértices (o fullscreen.vert gera os vértices dinamicamente no hardware)
    VkPipelineVertexInputStateCreateInfo infoEntradaVertices{};
    infoEntradaVertices.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    infoEntradaVertices.vertexBindingDescriptionCount = 0;
    infoEntradaVertices.pVertexBindingDescriptions = nullptr;
    infoEntradaVertices.vertexAttributeDescriptionCount = 0;
    infoEntradaVertices.pVertexAttributeDescriptions = nullptr;

    // Montagem da topologia geométrica
    VkPipelineInputAssemblyStateCreateInfo montagemEntrada{};
    montagemEntrada.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    montagemEntrada.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    montagemEntrada.primitiveRestartEnable = VK_FALSE;

    // Configuração de Viewport e Scissor dinâmicos
    std::vector<VkDynamicState> estadosDinamicos = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo infoEstadosDinamicos{};
    infoEstadosDinamicos.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    infoEstadosDinamicos.dynamicStateCount = static_cast<uint32_t>(estadosDinamicos.size());
    infoEstadosDinamicos.pDynamicStates = estadosDinamicos.data();

    VkPipelineViewportStateCreateInfo infoViewport{};
    infoViewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    infoViewport.viewportCount = 1;
    infoViewport.scissorCount = 1;

    // Configuração do Rasterizador
    VkPipelineRasterizationStateCreateInfo rasterizador{};
    rasterizador.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizador.depthClampEnable = VK_FALSE;
    rasterizador.rasterizerDiscardEnable = VK_FALSE;
    rasterizador.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizador.lineWidth = 1.0f;
    rasterizador.cullMode = VK_CULL_MODE_NONE; // Culling desabilitado para tela cheia
    rasterizador.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizador.depthBiasEnable = VK_FALSE;

    // Multisampling (MSAA desativado neste passe de tela)
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Mistura de cores (Color Blending)
    VkPipelineColorBlendAttachmentState anexoMisturaCor{};
    anexoMisturaCor.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                     VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT |
                                     VK_COLOR_COMPONENT_A_BIT;
    anexoMisturaCor.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo misturaCor{};
    misturaCor.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    misturaCor.logicOpEnable = VK_FALSE;
    misturaCor.attachmentCount = 1;
    misturaCor.pAttachments = &anexoMisturaCor;

    // Criação final do VkPipeline
    VkGraphicsPipelineCreateInfo infoCriacaoPipeline{};
    infoCriacaoPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    infoCriacaoPipeline.stageCount = 2;
    infoCriacaoPipeline.pStages = estagiosShaders;
    infoCriacaoPipeline.pVertexInputState = &infoEntradaVertices;
    infoCriacaoPipeline.pInputAssemblyState = &montagemEntrada;
    infoCriacaoPipeline.pViewportState = &infoViewport;
    infoCriacaoPipeline.pRasterizationState = &rasterizador;
    infoCriacaoPipeline.pMultisampleState = &multisample;
    infoCriacaoPipeline.pDepthStencilState = nullptr;
    infoCriacaoPipeline.pColorBlendState = &misturaCor;
    infoCriacaoPipeline.pDynamicState = &infoEstadosDinamicos;
    infoCriacaoPipeline.layout = layoutPipeline;
    infoCriacaoPipeline.renderPass = renderPass;
    infoCriacaoPipeline.subpass = 0;
    infoCriacaoPipeline.basePipelineHandle = VK_NULL_HANDLE;
    infoCriacaoPipeline.basePipelineIndex = -1;

    verificarResultadoVulkan(
        vkCreateGraphicsPipelines(dispositivoLogico, VK_NULL_HANDLE, 1, &infoCriacaoPipeline, nullptr, &pipelineGrafico),
        "Falha ao criar o Pipeline Gráfico do Vulkan."
    );
}

void PipelineGrafico::criarFramebuffers(VkDevice dispositivoLogico, const GerenciadorSwapchain& swapchain) {
    const auto& visualizacoes = swapchain.obterVisualizacoesImagens();
    framebuffersSwapchain.resize(visualizacoes.size());

    VkExtent2D dimensao = swapchain.obterExtensao();

    for (size_t i = 0; i < visualizacoes.size(); i++) {
        VkImageView anexos[] = { visualizacoes[i] };

        VkFramebufferCreateInfo infoFramebuffer{};
        infoFramebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        infoFramebuffer.renderPass = renderPass;
        infoFramebuffer.attachmentCount = 1;
        infoFramebuffer.pAttachments = anexos;
        infoFramebuffer.width = dimensao.width;
        infoFramebuffer.height = dimensao.height;
        infoFramebuffer.layers = 1;

        verificarResultadoVulkan(
            vkCreateFramebuffer(dispositivoLogico, &infoFramebuffer, nullptr, &framebuffersSwapchain[i]),
            "Falha ao criar Framebuffer da swapchain."
        );
    }
}
