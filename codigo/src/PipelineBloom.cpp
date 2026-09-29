#include "PipelineBloom.hpp"
#include <algorithm>
#include <cstring>

PipelineBloom::PipelineBloom(
    const DispositivoVulkan& dispositivo,
    const GerenciadorSwapchain& swapchain,
    const CarregadorShader& carregadorShader,
    VkCommandPool poolComandos,
    VkQueue fila) {

    dispositivoLogicoReferencia = dispositivo.obterDispositivoLogico();
    extensaoAtual = swapchain.obterExtensao();

    criarAmostradores(dispositivoLogicoReferencia);
    texturaMaterialCena = RecursosGraficos::criarTexturaMaterialProcedural(dispositivo, poolComandos, fila);

    criarLayoutsDescritores(dispositivoLogicoReferencia);
    criarBuffersUBO(dispositivo);

    criarRecursosImagens(dispositivo, extensaoAtual);
    criarRenderPasses(dispositivo, swapchain.obterFormatoImagem());
    criarPipelines(dispositivoLogicoReferencia, carregadorShader);
    criarPoolEDescritores(dispositivoLogicoReferencia);
    criarFramebuffers(dispositivoLogicoReferencia, swapchain);
}

PipelineBloom::~PipelineBloom() {
    VkDevice dev = dispositivoLogicoReferencia;
    limparRecursosDependentesSwapchain(dev);

    // Destruição dos Pipelines e Layouts
    if (pipelineCena != VK_NULL_HANDLE) vkDestroyPipeline(dev, pipelineCena, nullptr);
    if (layoutCena != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, layoutCena, nullptr);

    if (pipelineBright != VK_NULL_HANDLE) vkDestroyPipeline(dev, pipelineBright, nullptr);
    if (layoutBright != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, layoutBright, nullptr);

    if (pipelineDownsample != VK_NULL_HANDLE) vkDestroyPipeline(dev, pipelineDownsample, nullptr);
    if (pipelineUpsample != VK_NULL_HANDLE) vkDestroyPipeline(dev, pipelineUpsample, nullptr);
    if (layoutDownsampleUpsample != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, layoutDownsampleUpsample, nullptr);

    if (pipelineComposite != VK_NULL_HANDLE) vkDestroyPipeline(dev, pipelineComposite, nullptr);
    if (layoutComposite != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, layoutComposite, nullptr);

    // Render Passes
    if (renderPassCena != VK_NULL_HANDLE) vkDestroyRenderPass(dev, renderPassCena, nullptr);
    if (renderPassBright != VK_NULL_HANDLE) vkDestroyRenderPass(dev, renderPassBright, nullptr);
    if (renderPassDownsample != VK_NULL_HANDLE) vkDestroyRenderPass(dev, renderPassDownsample, nullptr);
    if (renderPassUpsample != VK_NULL_HANDLE) vkDestroyRenderPass(dev, renderPassUpsample, nullptr);
    if (renderPassComposite != VK_NULL_HANDLE) vkDestroyRenderPass(dev, renderPassComposite, nullptr);

    // Descriptor Layouts e Pool
    if (poolDescritores != VK_NULL_HANDLE) vkDestroyDescriptorPool(dev, poolDescritores, nullptr);
    if (layoutDescCena != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, layoutDescCena, nullptr);
    if (layoutDescBright != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, layoutDescBright, nullptr);
    if (layoutDescDownsampleUpsample != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, layoutDescDownsampleUpsample, nullptr);
    if (layoutDescComposite != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, layoutDescComposite, nullptr);

    // UBOs
    if (bufferCamera.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(dev, bufferCamera.buffer, nullptr);
        vkFreeMemory(dev, bufferCamera.memoria, nullptr);
    }
    if (bufferBloomParams.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(dev, bufferBloomParams.buffer, nullptr);
        vkFreeMemory(dev, bufferBloomParams.memoria, nullptr);
    }

    // Textura Material e Amostradores
    if (texturaMaterialCena.visualizacao != VK_NULL_HANDLE) vkDestroyImageView(dev, texturaMaterialCena.visualizacao, nullptr);
    if (texturaMaterialCena.imagem != VK_NULL_HANDLE) vkDestroyImage(dev, texturaMaterialCena.imagem, nullptr);
    if (texturaMaterialCena.memoria != VK_NULL_HANDLE) vkFreeMemory(dev, texturaMaterialCena.memoria, nullptr);

    if (amostradorLinear != VK_NULL_HANDLE) vkDestroySampler(dev, amostradorLinear, nullptr);
    if (amostradorMaterial != VK_NULL_HANDLE) vkDestroySampler(dev, amostradorMaterial, nullptr);
}

void PipelineBloom::limparRecursosDependentesSwapchain(VkDevice dev) {
    for (auto fb : framebuffersComposite) vkDestroyFramebuffer(dev, fb, nullptr);
    framebuffersComposite.clear();

    for (auto fb : framebuffersUpsample) vkDestroyFramebuffer(dev, fb, nullptr);
    framebuffersUpsample.clear();

    for (auto fb : framebuffersDownsample) vkDestroyFramebuffer(dev, fb, nullptr);
    framebuffersDownsample.clear();

    if (framebufferBright != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(dev, framebufferBright, nullptr);
        framebufferBright = VK_NULL_HANDLE;
    }
    if (framebufferCena != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(dev, framebufferCena, nullptr);
        framebufferCena = VK_NULL_HANDLE;
    }

    for (auto vis : visualizacoesMipsIndividuais) vkDestroyImageView(dev, vis, nullptr);
    visualizacoesMipsIndividuais.clear();

    if (imagemBloomMips.visualizacao != VK_NULL_HANDLE) vkDestroyImageView(dev, imagemBloomMips.visualizacao, nullptr);
    if (imagemBloomMips.imagem != VK_NULL_HANDLE) vkDestroyImage(dev, imagemBloomMips.imagem, nullptr);
    if (imagemBloomMips.memoria != VK_NULL_HANDLE) vkFreeMemory(dev, imagemBloomMips.memoria, nullptr);

    if (imagemProfundidade.visualizacao != VK_NULL_HANDLE) vkDestroyImageView(dev, imagemProfundidade.visualizacao, nullptr);
    if (imagemProfundidade.imagem != VK_NULL_HANDLE) vkDestroyImage(dev, imagemProfundidade.imagem, nullptr);
    if (imagemProfundidade.memoria != VK_NULL_HANDLE) vkFreeMemory(dev, imagemProfundidade.memoria, nullptr);

    if (imagemSceneHDR.visualizacao != VK_NULL_HANDLE) vkDestroyImageView(dev, imagemSceneHDR.visualizacao, nullptr);
    if (imagemSceneHDR.imagem != VK_NULL_HANDLE) vkDestroyImage(dev, imagemSceneHDR.imagem, nullptr);
    if (imagemSceneHDR.memoria != VK_NULL_HANDLE) vkFreeMemory(dev, imagemSceneHDR.memoria, nullptr);
}

void PipelineBloom::criarAmostradores(VkDevice dev) {
    amostradorLinear = RecursosGraficos::criarAmostradorLinear(dev);
    amostradorMaterial = RecursosGraficos::criarAmostradorTextura(dev);
}

void PipelineBloom::criarBuffersUBO(const DispositivoVulkan& dispositivo) {
    RecursosGraficos::criarBuffer(
        dispositivo, sizeof(CameraUBO),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferCamera.buffer, bufferCamera.memoria
    );
    bufferCamera.tamanho = sizeof(CameraUBO);

    RecursosGraficos::criarBuffer(
        dispositivo, sizeof(ParametrosBloomUBO),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferBloomParams.buffer, bufferBloomParams.memoria
    );
    bufferBloomParams.tamanho = sizeof(ParametrosBloomUBO);
}

void PipelineBloom::criarRecursosImagens(const DispositivoVulkan& dispositivo, VkExtent2D dimensao) {
    VkDevice dev = dispositivo.obterDispositivoLogico();

    // 1. Textura sceneHDR (Ponto flutuante 16 bits RGBA)
    imagemSceneHDR.formato = VK_FORMAT_R16G16B16A16_SFLOAT;
    imagemSceneHDR.extensao = dimensao;
    RecursosGraficos::criarImagem(
        dispositivo, dimensao.width, dimensao.height, 1,
        imagemSceneHDR.formato, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        imagemSceneHDR.imagem, imagemSceneHDR.memoria
    );
    imagemSceneHDR.visualizacao = RecursosGraficos::criarVisualizacaoImagem(
        dev, imagemSceneHDR.imagem, imagemSceneHDR.formato, VK_IMAGE_ASPECT_COLOR_BIT
    );

    // 2. Anexo de Profundidade para a cena 3D
    imagemProfundidade.formato = RecursosGraficos::encontrarFormatoProfundidade(dispositivo);
    imagemProfundidade.extensao = dimensao;
    RecursosGraficos::criarImagem(
        dispositivo, dimensao.width, dimensao.height, 1,
        imagemProfundidade.formato, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        imagemProfundidade.imagem, imagemProfundidade.memoria
    );
    imagemProfundidade.visualizacao = RecursosGraficos::criarVisualizacaoImagem(
        dev, imagemProfundidade.imagem, imagemProfundidade.formato, VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // 3. Imagem da Pirâmide de Bloom (5 níveis de mip)
    uint32_t larguraMip0 = std::max(1u, dimensao.width / 2);
    uint32_t alturaMip0 = std::max(1u, dimensao.height / 2);

    dimensoesMips.resize(TOTAL_NIVEIS_MIP);
    uint32_t w = larguraMip0, h = alturaMip0;
    for (uint32_t i = 0; i < TOTAL_NIVEIS_MIP; i++) {
        dimensoesMips[i] = { w, h };
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }

    imagemBloomMips.formato = VK_FORMAT_R16G16B16A16_SFLOAT;
    imagemBloomMips.extensao = { larguraMip0, alturaMip0 };
    imagemBloomMips.totalNiveisMip = TOTAL_NIVEIS_MIP;

    RecursosGraficos::criarImagem(
        dispositivo, larguraMip0, alturaMip0, TOTAL_NIVEIS_MIP,
        imagemBloomMips.formato, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        imagemBloomMips.imagem, imagemBloomMips.memoria
    );

    visualizacoesMipsIndividuais.resize(TOTAL_NIVEIS_MIP);
    for (uint32_t i = 0; i < TOTAL_NIVEIS_MIP; i++) {
        visualizacoesMipsIndividuais[i] = RecursosGraficos::criarVisualizacaoImagem(
            dev, imagemBloomMips.imagem, imagemBloomMips.formato, VK_IMAGE_ASPECT_COLOR_BIT, i, 1
        );
    }
}

void PipelineBloom::criarRenderPasses(const DispositivoVulkan& dispositivo, VkFormat formatoSwapchain) {
    VkDevice dev = dispositivo.obterDispositivoLogico();

    // -------------------------------------------------------------
    // RP 0: Passe da Cena (Scene Pass)
    // -------------------------------------------------------------
    VkAttachmentDescription anexoCorCena{};
    anexoCorCena.format = imagemSceneHDR.formato;
    anexoCorCena.samples = VK_SAMPLE_COUNT_1_BIT;
    anexoCorCena.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    anexoCorCena.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    anexoCorCena.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    anexoCorCena.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    anexoCorCena.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    anexoCorCena.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference refCorCena{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentDescription anexoProfundidade{};
    anexoProfundidade.format = imagemProfundidade.formato;
    anexoProfundidade.samples = VK_SAMPLE_COUNT_1_BIT;
    anexoProfundidade.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    anexoProfundidade.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    anexoProfundidade.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    anexoProfundidade.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    anexoProfundidade.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    anexoProfundidade.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference refProfundidade{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassCena{};
    subpassCena.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassCena.colorAttachmentCount = 1;
    subpassCena.pColorAttachments = &refCorCena;
    subpassCena.pDepthStencilAttachment = &refProfundidade;

    VkSubpassDependency depCena{};
    depCena.srcSubpass = VK_SUBPASS_EXTERNAL;
    depCena.dstSubpass = 0;
    depCena.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    depCena.srcAccessMask = 0;
    depCena.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    depCena.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription anexosCena[] = { anexoCorCena, anexoProfundidade };
    VkRenderPassCreateInfo infoRPCena{};
    infoRPCena.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    infoRPCena.attachmentCount = 2;
    infoRPCena.pAttachments = anexosCena;
    infoRPCena.subpassCount = 1;
    infoRPCena.pSubpasses = &subpassCena;
    infoRPCena.dependencyCount = 1;
    infoRPCena.pDependencies = &depCena;
    verificarResultadoVulkan(vkCreateRenderPass(dev, &infoRPCena, nullptr, &renderPassCena), "Falha ao criar renderPassCena.");

    // -------------------------------------------------------------
    // RP 1: Bright Pass (Escreve em bloomMip[0])
    // -------------------------------------------------------------
    VkAttachmentDescription anexoBright{};
    anexoBright.format = imagemBloomMips.formato;
    anexoBright.samples = VK_SAMPLE_COUNT_1_BIT;
    anexoBright.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    anexoBright.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    anexoBright.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    anexoBright.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference refBright{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpassBright{};
    subpassBright.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassBright.colorAttachmentCount = 1;
    subpassBright.pColorAttachments = &refBright;

    VkRenderPassCreateInfo infoRPBright{};
    infoRPBright.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    infoRPBright.attachmentCount = 1;
    infoRPBright.pAttachments = &anexoBright;
    infoRPBright.subpassCount = 1;
    infoRPBright.pSubpasses = &subpassBright;
    verificarResultadoVulkan(vkCreateRenderPass(dev, &infoRPBright, nullptr, &renderPassBright), "Falha ao criar renderPassBright.");

    // -------------------------------------------------------------
    // RP 2: Downsample (Passes 2 a 5: DONT_CARE / STORE)
    // -------------------------------------------------------------
    VkAttachmentDescription anexoDownsample = anexoBright;
    VkRenderPassCreateInfo infoRPDown{};
    infoRPDown.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    infoRPDown.attachmentCount = 1;
    infoRPDown.pAttachments = &anexoDownsample;
    infoRPDown.subpassCount = 1;
    infoRPDown.pSubpasses = &subpassBright;
    verificarResultadoVulkan(vkCreateRenderPass(dev, &infoRPDown, nullptr, &renderPassDownsample), "Falha ao criar renderPassDownsample.");

    // -------------------------------------------------------------
    // RP 3: Upsample (Passes 6 a 9: LOAD_OP_LOAD para manter valor anterior e somar)
    // -------------------------------------------------------------
    VkAttachmentDescription anexoUpsample = anexoBright;
    anexoUpsample.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Crucial: preserva conteúdo para soma aditiva
    anexoUpsample.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    anexoUpsample.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderPassCreateInfo infoRPUpsample{};
    infoRPUpsample.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    infoRPUpsample.attachmentCount = 1;
    infoRPUpsample.pAttachments = &anexoUpsample;
    infoRPUpsample.subpassCount = 1;
    infoRPUpsample.pSubpasses = &subpassBright;
    verificarResultadoVulkan(vkCreateRenderPass(dev, &infoRPUpsample, nullptr, &renderPassUpsample), "Falha ao criar renderPassUpsample.");

    // -------------------------------------------------------------
    // RP 4: Composite (Passe 10: saída para a Swapchain)
    // -------------------------------------------------------------
    VkAttachmentDescription anexoComp{};
    anexoComp.format = formatoSwapchain;
    anexoComp.samples = VK_SAMPLE_COUNT_1_BIT;
    anexoComp.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    anexoComp.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    anexoComp.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    anexoComp.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference refComp{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpassComp{};
    subpassComp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassComp.colorAttachmentCount = 1;
    subpassComp.pColorAttachments = &refComp;

    VkRenderPassCreateInfo infoRPComp{};
    infoRPComp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    infoRPComp.attachmentCount = 1;
    infoRPComp.pAttachments = &anexoComp;
    infoRPComp.subpassCount = 1;
    infoRPComp.pSubpasses = &subpassComp;
    verificarResultadoVulkan(vkCreateRenderPass(dev, &infoRPComp, nullptr, &renderPassComposite), "Falha ao criar renderPassComposite.");
}

void PipelineBloom::criarLayoutsDescritores(VkDevice dev) {
    // 1. Cena (UBO Camera + Sampler Material)
    VkDescriptorSetLayoutBinding vinculacaoCenaUBO{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacaoCenaTex{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacoesCena[] = { vinculacaoCenaUBO, vinculacaoCenaTex };

    VkDescriptorSetLayoutCreateInfo infoLayoutCena{};
    infoLayoutCena.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    infoLayoutCena.bindingCount = 2;
    infoLayoutCena.pBindings = vinculacoesCena;
    verificarResultadoVulkan(vkCreateDescriptorSetLayout(dev, &infoLayoutCena, nullptr, &layoutDescCena), "Erro layoutDescCena.");

    // 2. Bright Pass (UBO BloomParams + Sampler sceneHDR)
    VkDescriptorSetLayoutBinding vinculacaoBrightUBO{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacaoBrightTex{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacoesBright[] = { vinculacaoBrightUBO, vinculacaoBrightTex };

    VkDescriptorSetLayoutCreateInfo infoLayoutBright{};
    infoLayoutBright.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    infoLayoutBright.bindingCount = 2;
    infoLayoutBright.pBindings = vinculacoesBright;
    verificarResultadoVulkan(vkCreateDescriptorSetLayout(dev, &infoLayoutBright, nullptr, &layoutDescBright), "Erro layoutDescBright.");

    // 3. Downsample & Upsample (Sampler srcTexture)
    VkDescriptorSetLayoutBinding vinculacaoDownTex{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutCreateInfo infoLayoutDown{};
    infoLayoutDown.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    infoLayoutDown.bindingCount = 1;
    infoLayoutDown.pBindings = &vinculacaoDownTex;
    verificarResultadoVulkan(vkCreateDescriptorSetLayout(dev, &infoLayoutDown, nullptr, &layoutDescDownsampleUpsample), "Erro layoutDescDown.");

    // 4. Composite (UBO BloomParams + sceneHDR + bloomMip[0])
    VkDescriptorSetLayoutBinding vinculacaoCompUBO{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacaoCompScene{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacaoCompBloom{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding vinculacoesComp[] = { vinculacaoCompUBO, vinculacaoCompScene, vinculacaoCompBloom };

    VkDescriptorSetLayoutCreateInfo infoLayoutComp{};
    infoLayoutComp.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    infoLayoutComp.bindingCount = 3;
    infoLayoutComp.pBindings = vinculacoesComp;
    verificarResultadoVulkan(vkCreateDescriptorSetLayout(dev, &infoLayoutComp, nullptr, &layoutDescComposite), "Erro layoutDescComposite.");
}

void PipelineBloom::criarPipelines(VkDevice dev, const CarregadorShader& carregador) {
    // -------------------------------------------------------------
    // 1. Pipeline da Cena 3D (scene.vert + scene.frag)
    // -------------------------------------------------------------
    VkPipelineLayoutCreateInfo infoLayoutCena{};
    infoLayoutCena.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    infoLayoutCena.setLayoutCount = 1;
    infoLayoutCena.pSetLayouts = &layoutDescCena;
    verificarResultadoVulkan(vkCreatePipelineLayout(dev, &infoLayoutCena, nullptr, &layoutCena), "Erro layoutCena.");

    VkPipelineShaderStageCreateInfo stgCenaV{};
    stgCenaV.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgCenaV.stage = VK_SHADER_STAGE_VERTEX_BIT;
    stgCenaV.module = carregador.obterModulo("scene.vert.spv");
    stgCenaV.pName = "main";

    VkPipelineShaderStageCreateInfo stgCenaF{};
    stgCenaF.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgCenaF.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stgCenaF.module = carregador.obterModulo("scene.frag.spv");
    stgCenaF.pName = "main";

    VkPipelineShaderStageCreateInfo stgsCena[] = { stgCenaV, stgCenaF };

    auto bindDesc = VerticeCena::obterDescricaoVinculacao();
    auto attrDesc = VerticeCena::obterDescricaoAtributos();

    VkPipelineVertexInputStateCreateInfo vertInputCena{};
    vertInputCena.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputCena.vertexBindingDescriptionCount = 1;
    vertInputCena.pVertexBindingDescriptions = &bindDesc;
    vertInputCena.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
    vertInputCena.pVertexAttributeDescriptions = attrDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    std::vector<VkDynamicState> dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynInfo{};
    dynInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynInfo.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynInfo.pDynamicStates = dynStates.data();

    VkPipelineViewportStateCreateInfo vpInfo{};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.viewportCount = 1;
    vpInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rastCena{};
    rastCena.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastCena.polygonMode = VK_POLYGON_MODE_FILL;
    rastCena.lineWidth = 1.0f;
    rastCena.cullMode = VK_CULL_MODE_BACK_BIT;
    rastCena.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineDepthStencilStateCreateInfo dsCena{};
    dsCena.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsCena.depthTestEnable = VK_TRUE;
    dsCena.depthWriteEnable = VK_TRUE;
    dsCena.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineMultisampleStateCreateInfo msInfo{};
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttCena{};
    blendAttCena.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttCena.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blendCena{};
    blendCena.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendCena.attachmentCount = 1;
    blendCena.pAttachments = &blendAttCena;

    VkGraphicsPipelineCreateInfo infoPipeCena{};
    infoPipeCena.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    infoPipeCena.stageCount = 2;
    infoPipeCena.pStages = stgsCena;
    infoPipeCena.pVertexInputState = &vertInputCena;
    infoPipeCena.pInputAssemblyState = &inputAssembly;
    infoPipeCena.pViewportState = &vpInfo;
    infoPipeCena.pRasterizationState = &rastCena;
    infoPipeCena.pMultisampleState = &msInfo;
    infoPipeCena.pDepthStencilState = &dsCena;
    infoPipeCena.pColorBlendState = &blendCena;
    infoPipeCena.pDynamicState = &dynInfo;
    infoPipeCena.layout = layoutCena;
    infoPipeCena.renderPass = renderPassCena;
    verificarResultadoVulkan(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &infoPipeCena, nullptr, &pipelineCena), "Erro pipelineCena.");

    // Configurações comuns aos passes de pós-processamento (fullscreen.vert)
    VkPipelineVertexInputStateCreateInfo vertInputVazio{};
    vertInputVazio.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineRasterizationStateCreateInfo rastPost{};
    rastPost.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastPost.polygonMode = VK_POLYGON_MODE_FILL;
    rastPost.lineWidth = 1.0f;
    rastPost.cullMode = VK_CULL_MODE_NONE;
    rastPost.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineShaderStageCreateInfo stgFullV{};
    stgFullV.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgFullV.stage = VK_SHADER_STAGE_VERTEX_BIT;
    stgFullV.module = carregador.obterModulo("fullscreen.vert.spv");
    stgFullV.pName = "main";

    // -------------------------------------------------------------
    // 2. Pipeline Bright Pass (fullscreen.vert + bright.frag)
    // -------------------------------------------------------------
    VkPipelineLayoutCreateInfo infoLayoutBright{};
    infoLayoutBright.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    infoLayoutBright.setLayoutCount = 1;
    infoLayoutBright.pSetLayouts = &layoutDescBright;
    verificarResultadoVulkan(vkCreatePipelineLayout(dev, &infoLayoutBright, nullptr, &layoutBright), "Erro layoutBright.");

    VkPipelineShaderStageCreateInfo stgBrightF{};
    stgBrightF.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgBrightF.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stgBrightF.module = carregador.obterModulo("bright.frag.spv");
    stgBrightF.pName = "main";

    VkPipelineShaderStageCreateInfo stgsBright[] = { stgFullV, stgBrightF };

    VkGraphicsPipelineCreateInfo infoPipeBright = infoPipeCena;
    infoPipeBright.stageCount = 2;
    infoPipeBright.pStages = stgsBright;
    infoPipeBright.pVertexInputState = &vertInputVazio;
    infoPipeBright.pRasterizationState = &rastPost;
    infoPipeBright.pDepthStencilState = nullptr;
    infoPipeBright.layout = layoutBright;
    infoPipeBright.renderPass = renderPassBright;
    verificarResultadoVulkan(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &infoPipeBright, nullptr, &pipelineBright), "Erro pipelineBright.");

    // -------------------------------------------------------------
    // 3 e 4. Pipelines Downsample e Upsample
    // -------------------------------------------------------------
    VkPushConstantRange rangePush{};
    rangePush.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    rangePush.offset = 0;
    rangePush.size = sizeof(PushConstantsBloom);

    VkPipelineLayoutCreateInfo infoLayoutDownUp{};
    infoLayoutDownUp.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    infoLayoutDownUp.setLayoutCount = 1;
    infoLayoutDownUp.pSetLayouts = &layoutDescDownsampleUpsample;
    infoLayoutDownUp.pushConstantRangeCount = 1;
    infoLayoutDownUp.pPushConstantRanges = &rangePush;
    verificarResultadoVulkan(vkCreatePipelineLayout(dev, &infoLayoutDownUp, nullptr, &layoutDownsampleUpsample), "Erro layoutDownUp.");

    // Downsample
    VkPipelineShaderStageCreateInfo stgDownF{};
    stgDownF.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgDownF.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stgDownF.module = carregador.obterModulo("downsample.frag.spv");
    stgDownF.pName = "main";

    VkPipelineShaderStageCreateInfo stgsDown[] = { stgFullV, stgDownF };
    VkGraphicsPipelineCreateInfo infoPipeDown = infoPipeBright;
    infoPipeDown.pStages = stgsDown;
    infoPipeDown.layout = layoutDownsampleUpsample;
    infoPipeDown.renderPass = renderPassDownsample;
    verificarResultadoVulkan(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &infoPipeDown, nullptr, &pipelineDownsample), "Erro pipelineDown.");

    // Upsample com Blend Aditivo (ONE + ONE)
    VkPipelineShaderStageCreateInfo stgUpF{};
    stgUpF.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgUpF.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stgUpF.module = carregador.obterModulo("upsample.frag.spv");
    stgUpF.pName = "main";

    VkPipelineShaderStageCreateInfo stgsUp[] = { stgFullV, stgUpF };

    VkPipelineColorBlendAttachmentState blendAttUpsample{};
    blendAttUpsample.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttUpsample.blendEnable = VK_TRUE;
    blendAttUpsample.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttUpsample.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttUpsample.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttUpsample.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttUpsample.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttUpsample.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blendUpsampleInfo{};
    blendUpsampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendUpsampleInfo.attachmentCount = 1;
    blendUpsampleInfo.pAttachments = &blendAttUpsample;

    VkGraphicsPipelineCreateInfo infoPipeUp = infoPipeDown;
    infoPipeUp.pStages = stgsUp;
    infoPipeUp.pColorBlendState = &blendUpsampleInfo;
    infoPipeUp.renderPass = renderPassUpsample;
    verificarResultadoVulkan(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &infoPipeUp, nullptr, &pipelineUpsample), "Erro pipelineUp.");

    // -------------------------------------------------------------
    // 5. Pipeline Composite (fullscreen.vert + composite.frag)
    // -------------------------------------------------------------
    VkPipelineLayoutCreateInfo infoLayoutComp{};
    infoLayoutComp.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    infoLayoutComp.setLayoutCount = 1;
    infoLayoutComp.pSetLayouts = &layoutDescComposite;
    verificarResultadoVulkan(vkCreatePipelineLayout(dev, &infoLayoutComp, nullptr, &layoutComposite), "Erro layoutComposite.");

    VkPipelineShaderStageCreateInfo stgCompF{};
    stgCompF.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stgCompF.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stgCompF.module = carregador.obterModulo("composite.frag.spv");
    stgCompF.pName = "main";

    VkPipelineShaderStageCreateInfo stgsComp[] = { stgFullV, stgCompF };
    VkGraphicsPipelineCreateInfo infoPipeComp = infoPipeBright;
    infoPipeComp.pStages = stgsComp;
    infoPipeComp.layout = layoutComposite;
    infoPipeComp.renderPass = renderPassComposite;
    verificarResultadoVulkan(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &infoPipeComp, nullptr, &pipelineComposite), "Erro pipelineComp.");
}

void PipelineBloom::criarPoolEDescritores(VkDevice dev) {
    std::vector<VkDescriptorPoolSize> tamanhosPool = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 }
    };

    VkDescriptorPoolCreateInfo infoPool{};
    infoPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    infoPool.maxSets = 20;
    infoPool.poolSizeCount = static_cast<uint32_t>(tamanhosPool.size());
    infoPool.pPoolSizes = tamanhosPool.data();
    verificarResultadoVulkan(vkCreateDescriptorPool(dev, &infoPool, nullptr, &poolDescritores), "Erro poolDescritores.");

    // 1. Descritor da Cena
    VkDescriptorSetAllocateInfo allocCena{};
    allocCena.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocCena.descriptorPool = poolDescritores;
    allocCena.descriptorSetCount = 1;
    allocCena.pSetLayouts = &layoutDescCena;
    vkAllocateDescriptorSets(dev, &allocCena, &descSetCena);

    VkDescriptorBufferInfo bufCamInfo{ bufferCamera.buffer, 0, sizeof(CameraUBO) };
    VkDescriptorImageInfo imgMatInfo{ amostradorMaterial, texturaMaterialCena.visualizacao, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    VkWriteDescriptorSet writesCena[2]{};
    writesCena[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesCena[0].dstSet = descSetCena;
    writesCena[0].dstBinding = 0;
    writesCena[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writesCena[0].descriptorCount = 1;
    writesCena[0].pBufferInfo = &bufCamInfo;

    writesCena[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesCena[1].dstSet = descSetCena;
    writesCena[1].dstBinding = 1;
    writesCena[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writesCena[1].descriptorCount = 1;
    writesCena[1].pImageInfo = &imgMatInfo;
    vkUpdateDescriptorSets(dev, 2, writesCena, 0, nullptr);

    // 2. Descritor Bright Pass
    VkDescriptorSetAllocateInfo allocBright = allocCena;
    allocBright.pSetLayouts = &layoutDescBright;
    vkAllocateDescriptorSets(dev, &allocBright, &descSetBright);

    VkDescriptorBufferInfo bufBloomInfo{ bufferBloomParams.buffer, 0, sizeof(ParametrosBloomUBO) };
    VkDescriptorImageInfo imgSceneInfo{ amostradorLinear, imagemSceneHDR.visualizacao, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    VkWriteDescriptorSet writesBright[2]{};
    writesBright[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesBright[0].dstSet = descSetBright;
    writesBright[0].dstBinding = 0;
    writesBright[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writesBright[0].descriptorCount = 1;
    writesBright[0].pBufferInfo = &bufBloomInfo;

    writesBright[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesBright[1].dstSet = descSetBright;
    writesBright[1].dstBinding = 1;
    writesBright[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writesBright[1].descriptorCount = 1;
    writesBright[1].pImageInfo = &imgSceneInfo;
    vkUpdateDescriptorSets(dev, 2, writesBright, 0, nullptr);

    // 3. Descritores Downsample (4 conjuntos: lendo mip 0, 1, 2, 3)
    descSetsDownsample.resize(4);
    std::vector<VkDescriptorSetLayout> layoutsDown(4, layoutDescDownsampleUpsample);
    VkDescriptorSetAllocateInfo allocDown{};
    allocDown.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocDown.descriptorPool = poolDescritores;
    allocDown.descriptorSetCount = 4;
    allocDown.pSetLayouts = layoutsDown.data();
    vkAllocateDescriptorSets(dev, &allocDown, descSetsDownsample.data());

    std::vector<VkDescriptorImageInfo> imgDownInfos(4);
    std::vector<VkWriteDescriptorSet> writesDown(4);
    for (int i = 0; i < 4; i++) {
        imgDownInfos[i] = { amostradorLinear, visualizacoesMipsIndividuais[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        writesDown[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writesDown[i].dstSet = descSetsDownsample[i];
        writesDown[i].dstBinding = 0;
        writesDown[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writesDown[i].descriptorCount = 1;
        writesDown[i].pImageInfo = &imgDownInfos[i];
    }
    vkUpdateDescriptorSets(dev, 4, writesDown.data(), 0, nullptr);

    // 4. Descritores Upsample (4 conjuntos: lendo mip 4, 3, 2, 1)
    descSetsUpsample.resize(4);
    vkAllocateDescriptorSets(dev, &allocDown, descSetsUpsample.data());

    std::vector<VkDescriptorImageInfo> imgUpInfos(4);
    std::vector<VkWriteDescriptorSet> writesUp(4);
    for (int i = 0; i < 4; i++) {
        // Upsample lê mip i+1 (nível menor) para somar no nível i
        imgUpInfos[i] = { amostradorLinear, visualizacoesMipsIndividuais[i + 1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        writesUp[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writesUp[i].dstSet = descSetsUpsample[i];
        writesUp[i].dstBinding = 0;
        writesUp[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writesUp[i].descriptorCount = 1;
        writesUp[i].pImageInfo = &imgUpInfos[i];
    }
    vkUpdateDescriptorSets(dev, 4, writesUp.data(), 0, nullptr);

    // 5. Descritor Composite (UBO BloomParams + sceneHDR + bloomMip[0])
    VkDescriptorSetAllocateInfo allocComp = allocCena;
    allocComp.pSetLayouts = &layoutDescComposite;
    vkAllocateDescriptorSets(dev, &allocComp, &descSetComposite);

    VkDescriptorImageInfo imgBloom0Info{ amostradorLinear, visualizacoesMipsIndividuais[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    VkWriteDescriptorSet writesComp[3]{};
    writesComp[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesComp[0].dstSet = descSetComposite;
    writesComp[0].dstBinding = 0;
    writesComp[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writesComp[0].descriptorCount = 1;
    writesComp[0].pBufferInfo = &bufBloomInfo;

    writesComp[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesComp[1].dstSet = descSetComposite;
    writesComp[1].dstBinding = 1;
    writesComp[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writesComp[1].descriptorCount = 1;
    writesComp[1].pImageInfo = &imgSceneInfo;

    writesComp[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesComp[2].dstSet = descSetComposite;
    writesComp[2].dstBinding = 2;
    writesComp[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writesComp[2].descriptorCount = 1;
    writesComp[2].pImageInfo = &imgBloom0Info;
    vkUpdateDescriptorSets(dev, 3, writesComp, 0, nullptr);
}

void PipelineBloom::criarFramebuffers(VkDevice dev, const GerenciadorSwapchain& swapchain) {
    // 1. Framebuffer da Cena (sceneHDR + Depth)
    VkImageView anexosCena[] = { imagemSceneHDR.visualizacao, imagemProfundidade.visualizacao };
    VkFramebufferCreateInfo fbCenaInfo{};
    fbCenaInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCenaInfo.renderPass = renderPassCena;
    fbCenaInfo.attachmentCount = 2;
    fbCenaInfo.pAttachments = anexosCena;
    fbCenaInfo.width = extensaoAtual.width;
    fbCenaInfo.height = extensaoAtual.height;
    fbCenaInfo.layers = 1;
    verificarResultadoVulkan(vkCreateFramebuffer(dev, &fbCenaInfo, nullptr, &framebufferCena), "Erro fbCena.");

    // 2. Framebuffer Bright Pass (bloomMip[0])
    VkImageView anexoBright[] = { visualizacoesMipsIndividuais[0] };
    VkFramebufferCreateInfo fbBrightInfo{};
    fbBrightInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbBrightInfo.renderPass = renderPassBright;
    fbBrightInfo.attachmentCount = 1;
    fbBrightInfo.pAttachments = anexoBright;
    fbBrightInfo.width = dimensoesMips[0].width;
    fbBrightInfo.height = dimensoesMips[0].height;
    fbBrightInfo.layers = 1;
    verificarResultadoVulkan(vkCreateFramebuffer(dev, &fbBrightInfo, nullptr, &framebufferBright), "Erro fbBright.");

    // 3. Framebuffers Downsample (escrevendo em mip 1 a 4)
    framebuffersDownsample.resize(4);
    for (int i = 0; i < 4; i++) {
        uint32_t nivelDestino = i + 1;
        VkImageView anexoDown[] = { visualizacoesMipsIndividuais[nivelDestino] };
        VkFramebufferCreateInfo fbDownInfo{};
        fbDownInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbDownInfo.renderPass = renderPassDownsample;
        fbDownInfo.attachmentCount = 1;
        fbDownInfo.pAttachments = anexoDown;
        fbDownInfo.width = dimensoesMips[nivelDestino].width;
        fbDownInfo.height = dimensoesMips[nivelDestino].height;
        fbDownInfo.layers = 1;
        verificarResultadoVulkan(vkCreateFramebuffer(dev, &fbDownInfo, nullptr, &framebuffersDownsample[i]), "Erro fbDown.");
    }

    // 4. Framebuffers Upsample (escrevendo em mip 3, 2, 1, 0)
    framebuffersUpsample.resize(4);
    for (int i = 0; i < 4; i++) {
        uint32_t nivelDestino = 3 - i; // 3, 2, 1, 0
        VkImageView anexoUp[] = { visualizacoesMipsIndividuais[nivelDestino] };
        VkFramebufferCreateInfo fbUpInfo{};
        fbUpInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbUpInfo.renderPass = renderPassUpsample;
        fbUpInfo.attachmentCount = 1;
        fbUpInfo.pAttachments = anexoUp;
        fbUpInfo.width = dimensoesMips[nivelDestino].width;
        fbUpInfo.height = dimensoesMips[nivelDestino].height;
        fbUpInfo.layers = 1;
        verificarResultadoVulkan(vkCreateFramebuffer(dev, &fbUpInfo, nullptr, &framebuffersUpsample[i]), "Erro fbUp.");
    }

    // 5. Framebuffers Composite (imagens da Swapchain)
    const auto& visSwap = swapchain.obterVisualizacoesImagens();
    framebuffersComposite.resize(visSwap.size());
    for (size_t i = 0; i < visSwap.size(); i++) {
        VkImageView anexoComp[] = { visSwap[i] };
        VkFramebufferCreateInfo fbCompInfo{};
        fbCompInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCompInfo.renderPass = renderPassComposite;
        fbCompInfo.attachmentCount = 1;
        fbCompInfo.pAttachments = anexoComp;
        fbCompInfo.width = extensaoAtual.width;
        fbCompInfo.height = extensaoAtual.height;
        fbCompInfo.layers = 1;
        verificarResultadoVulkan(vkCreateFramebuffer(dev, &fbCompInfo, nullptr, &framebuffersComposite[i]), "Erro fbComp.");
    }
}

void PipelineBloom::atualizarUBOs(const CameraUBO& camera, const ParametrosBloomUBO& bloom) {
    void* dados = nullptr;
    vkMapMemory(dispositivoLogicoReferencia, bufferCamera.memoria, 0, sizeof(CameraUBO), 0, &dados);
    memcpy(dados, &camera, sizeof(CameraUBO));
    vkUnmapMemory(dispositivoLogicoReferencia, bufferCamera.memoria);

    vkMapMemory(dispositivoLogicoReferencia, bufferBloomParams.memoria, 0, sizeof(ParametrosBloomUBO), 0, &dados);
    memcpy(dados, &bloom, sizeof(ParametrosBloomUBO));
    vkUnmapMemory(dispositivoLogicoReferencia, bufferBloomParams.memoria);
}

void PipelineBloom::gravarComandosCompletos(
    VkCommandBuffer cmd,
    uint32_t indiceSwapchain,
    const CenaGeometrica& cena) {

    // -------------------------------------------------------------
    // PASSE 0: Renderização da Cena 3D em HDR (sceneHDR)
    // -------------------------------------------------------------
    VkClearValue valoresLimpezaCena[2];
    valoresLimpezaCena[0].color = {{ 0.02f, 0.03f, 0.06f, 1.0f }}; // Fundo escuro
    valoresLimpezaCena[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo infoCena{};
    infoCena.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    infoCena.renderPass = renderPassCena;
    infoCena.framebuffer = framebufferCena;
    infoCena.renderArea.extent = extensaoAtual;
    infoCena.clearValueCount = 2;
    infoCena.pClearValues = valoresLimpezaCena;

    vkCmdBeginRenderPass(cmd, &infoCena, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineCena);

    VkViewport vpCena{ 0.0f, 0.0f, (float)extensaoAtual.width, (float)extensaoAtual.height, 0.0f, 1.0f };
    VkRect2D scCena{ {0, 0}, extensaoAtual };
    vkCmdSetViewport(cmd, 0, 1, &vpCena);
    vkCmdSetScissor(cmd, 0, 1, &scCena);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutCena, 0, 1, &descSetCena, 0, nullptr);
    cena.desenhar(cmd);
    vkCmdEndRenderPass(cmd);

    // BARREIRA B1: sceneHDR de COLOR_ATTACHMENT_OPTIMAL para SHADER_READ_ONLY_OPTIMAL
    RecursosGraficos::transicionarLayoutImagemComando(
        cmd, imagemSceneHDR.imagem,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 1
    );

    // -------------------------------------------------------------
    // PASSE 1: Bright Pass (Filtra altas luzes para bloomMip[0])
    // -------------------------------------------------------------
    VkRenderPassBeginInfo infoBright{};
    infoBright.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    infoBright.renderPass = renderPassBright;
    infoBright.framebuffer = framebufferBright;
    infoBright.renderArea.extent = dimensoesMips[0];
    infoBright.clearValueCount = 0;

    vkCmdBeginRenderPass(cmd, &infoBright, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineBright);

    VkViewport vpBright{ 0.0f, 0.0f, (float)dimensoesMips[0].width, (float)dimensoesMips[0].height, 0.0f, 1.0f };
    VkRect2D scBright{ {0, 0}, dimensoesMips[0] };
    vkCmdSetViewport(cmd, 0, 1, &vpBright);
    vkCmdSetScissor(cmd, 0, 1, &scBright);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutBright, 0, 1, &descSetBright, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0); // Triângulo tela cheia
    vkCmdEndRenderPass(cmd);

    // BARREIRA B2: bloomMip[0] de COLOR_ATTACHMENT para SHADER_READ_ONLY
    RecursosGraficos::transicionarLayoutImagemComando(
        cmd, imagemBloomMips.imagem,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 1
    );

    // -------------------------------------------------------------
    // PASSES 2 A 5: Downsample Piramidal (4 iterações com filtro de 13 amostras)
    // -------------------------------------------------------------
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineDownsample);

    for (int i = 0; i < 4; i++) {
        uint32_t nivelOrigem = i;
        uint32_t nivelDestino = i + 1;

        VkRenderPassBeginInfo infoDown{};
        infoDown.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        infoDown.renderPass = renderPassDownsample;
        infoDown.framebuffer = framebuffersDownsample[i];
        infoDown.renderArea.extent = dimensoesMips[nivelDestino];

        vkCmdBeginRenderPass(cmd, &infoDown, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vpDown{ 0.0f, 0.0f, (float)dimensoesMips[nivelDestino].width, (float)dimensoesMips[nivelDestino].height, 0.0f, 1.0f };
        VkRect2D scDown{ {0, 0}, dimensoesMips[nivelDestino] };
        vkCmdSetViewport(cmd, 0, 1, &vpDown);
        vkCmdSetScissor(cmd, 0, 1, &scDown);

        PushConstantsBloom pc{};
        pc.texelSize = { 1.0f / dimensoesMips[nivelOrigem].width, 1.0f / dimensoesMips[nivelOrigem].height };
        pc.mipLevel = nivelDestino;
        vkCmdPushConstants(cmd, layoutDownsampleUpsample, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsBloom), &pc);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutDownsampleUpsample, 0, 1, &descSetsDownsample[i], 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);

        // BARREIRAS B3 a B6: bloomMip[nivelDestino] de COLOR_ATTACHMENT para SHADER_READ_ONLY
        RecursosGraficos::transicionarLayoutImagemComando(
            cmd, imagemBloomMips.imagem,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            nivelDestino, 1
        );
    }

    // -------------------------------------------------------------
    // PASSES 6 A 9: Upsample Piramidal (4 iterações com filtro tenda e blend aditivo)
    // -------------------------------------------------------------
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineUpsample);

    for (int i = 0; i < 4; i++) {
        uint32_t nivelOrigem = 4 - i;   // Lê de 4, 3, 2, 1
        uint32_t nivelDestino = 3 - i;  // Escreve e acumula em 3, 2, 1, 0

        VkRenderPassBeginInfo infoUp{};
        infoUp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        infoUp.renderPass = renderPassUpsample;
        infoUp.framebuffer = framebuffersUpsample[i];
        infoUp.renderArea.extent = dimensoesMips[nivelDestino];

        vkCmdBeginRenderPass(cmd, &infoUp, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vpUp{ 0.0f, 0.0f, (float)dimensoesMips[nivelDestino].width, (float)dimensoesMips[nivelDestino].height, 0.0f, 1.0f };
        VkRect2D scUp{ {0, 0}, dimensoesMips[nivelDestino] };
        vkCmdSetViewport(cmd, 0, 1, &vpUp);
        vkCmdSetScissor(cmd, 0, 1, &scUp);

        PushConstantsBloom pc{};
        pc.texelSize = { 1.0f / dimensoesMips[nivelOrigem].width, 1.0f / dimensoesMips[nivelOrigem].height };
        pc.mipLevel = nivelDestino;
        vkCmdPushConstants(cmd, layoutDownsampleUpsample, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsBloom), &pc);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutDownsampleUpsample, 0, 1, &descSetsUpsample[nivelDestino], 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);

        // BARREIRAS B7 a B10: transiciona o nível destino para SHADER_READ_ONLY para o próximo passo ler
        RecursosGraficos::transicionarLayoutImagemComando(
            cmd, imagemBloomMips.imagem,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            nivelDestino, 1
        );
    }

    // -------------------------------------------------------------
    // PASSE 10: Composição Final e Tone Mapping de Reinhard (Saída para a Swapchain)
    // -------------------------------------------------------------
    VkRenderPassBeginInfo infoComp{};
    infoComp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    infoComp.renderPass = renderPassComposite;
    infoComp.framebuffer = framebuffersComposite[indiceSwapchain];
    infoComp.renderArea.extent = extensaoAtual;

    vkCmdBeginRenderPass(cmd, &infoComp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineComposite);

    VkViewport vpComp{ 0.0f, 0.0f, (float)extensaoAtual.width, (float)extensaoAtual.height, 0.0f, 1.0f };
    VkRect2D scComp{ {0, 0}, extensaoAtual };
    vkCmdSetViewport(cmd, 0, 1, &vpComp);
    vkCmdSetScissor(cmd, 0, 1, &scComp);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layoutComposite, 0, 1, &descSetComposite, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd);
}

void PipelineBloom::recriarRecursosDimensionais(
    const DispositivoVulkan& dispositivo,
    const GerenciadorSwapchain& swapchain) {

    VkDevice dev = dispositivo.obterDispositivoLogico();
    extensaoAtual = swapchain.obterExtensao();

    limparRecursosDependentesSwapchain(dev);
    criarRecursosImagens(dispositivo, extensaoAtual);
    criarFramebuffers(dev, swapchain);

    // Atualiza os descritores com as novas visualizações de imagem
    VkDescriptorImageInfo imgSceneInfo{ amostradorLinear, imagemSceneHDR.visualizacao, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    VkWriteDescriptorSet writeBright{};
    writeBright.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeBright.dstSet = descSetBright;
    writeBright.dstBinding = 1;
    writeBright.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeBright.descriptorCount = 1;
    writeBright.pImageInfo = &imgSceneInfo;
    vkUpdateDescriptorSets(dev, 1, &writeBright, 0, nullptr);

    for (int i = 0; i < 4; i++) {
        VkDescriptorImageInfo imgDown{ amostradorLinear, visualizacoesMipsIndividuais[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkWriteDescriptorSet writeDown{};
        writeDown.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDown.dstSet = descSetsDownsample[i];
        writeDown.dstBinding = 0;
        writeDown.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDown.descriptorCount = 1;
        writeDown.pImageInfo = &imgDown;
        vkUpdateDescriptorSets(dev, 1, &writeDown, 0, nullptr);

        VkDescriptorImageInfo imgUp{ amostradorLinear, visualizacoesMipsIndividuais[i + 1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkWriteDescriptorSet writeUp{};
        writeUp.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeUp.dstSet = descSetsUpsample[i];
        writeUp.dstBinding = 0;
        writeUp.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeUp.descriptorCount = 1;
        writeUp.pImageInfo = &imgUp;
        vkUpdateDescriptorSets(dev, 1, &writeUp, 0, nullptr);
    }

    VkDescriptorImageInfo imgBloom0Info{ amostradorLinear, visualizacoesMipsIndividuais[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkWriteDescriptorSet writesComp[2]{};
    writesComp[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesComp[0].dstSet = descSetComposite;
    writesComp[0].dstBinding = 1;
    writesComp[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writesComp[0].descriptorCount = 1;
    writesComp[0].pImageInfo = &imgSceneInfo;

    writesComp[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writesComp[1].dstSet = descSetComposite;
    writesComp[1].dstBinding = 2;
    writesComp[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writesComp[1].descriptorCount = 1;
    writesComp[1].pImageInfo = &imgBloom0Info;
    vkUpdateDescriptorSets(dev, 2, writesComp, 0, nullptr);
}
