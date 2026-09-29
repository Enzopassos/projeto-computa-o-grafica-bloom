#include "RecursosGraficos.hpp"
#include <cstring>
#include <cmath>

void RecursosGraficos::criarBuffer(
    const DispositivoVulkan& dispositivo,
    VkDeviceSize tamanho,
    VkBufferUsageFlags uso,
    VkMemoryPropertyFlags propriedades,
    VkBuffer& buffer,
    VkDeviceMemory& memoriaBuffer) {

    VkDevice dev = dispositivo.obterDispositivoLogico();

    VkBufferCreateInfo infoBuffer{};
    infoBuffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    infoBuffer.size = tamanho;
    infoBuffer.usage = uso;
    infoBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    verificarResultadoVulkan(
        vkCreateBuffer(dev, &infoBuffer, nullptr, &buffer),
        "Falha ao criar buffer Vulkan."
    );

    VkMemoryRequirements requisitos;
    vkGetBufferMemoryRequirements(dev, buffer, &requisitos);

    VkMemoryAllocateInfo infoAlocacao{};
    infoAlocacao.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    infoAlocacao.allocationSize = requisitos.size;
    infoAlocacao.memoryTypeIndex = dispositivo.encontrarTipoMemoria(requisitos.memoryTypeBits, propriedades);

    verificarResultadoVulkan(
        vkAllocateMemory(dev, &infoAlocacao, nullptr, &memoriaBuffer),
        "Falha ao alocar memória para o buffer Vulkan."
    );

    vkBindBufferMemory(dev, buffer, memoriaBuffer, 0);
}

void RecursosGraficos::criarImagem(
    const DispositivoVulkan& dispositivo,
    uint32_t largura,
    uint32_t altura,
    uint32_t niveisMip,
    VkFormat formato,
    VkImageTiling arranjo,
    VkImageUsageFlags uso,
    VkMemoryPropertyFlags propriedades,
    VkImage& imagem,
    VkDeviceMemory& memoriaImagem) {

    VkDevice dev = dispositivo.obterDispositivoLogico();

    VkImageCreateInfo infoImagem{};
    infoImagem.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    infoImagem.imageType = VK_IMAGE_TYPE_2D;
    infoImagem.extent.width = largura;
    infoImagem.extent.height = altura;
    infoImagem.extent.depth = 1;
    infoImagem.mipLevels = niveisMip;
    infoImagem.arrayLayers = 1;
    infoImagem.format = formato;
    infoImagem.tiling = arranjo;
    infoImagem.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    infoImagem.usage = uso;
    infoImagem.samples = VK_SAMPLE_COUNT_1_BIT;
    infoImagem.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    verificarResultadoVulkan(
        vkCreateImage(dev, &infoImagem, nullptr, &imagem),
        "Falha ao criar imagem Vulkan (VkImage)."
    );

    VkMemoryRequirements requisitos;
    vkGetImageMemoryRequirements(dev, imagem, &requisitos);

    VkMemoryAllocateInfo infoAlocacao{};
    infoAlocacao.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    infoAlocacao.allocationSize = requisitos.size;
    infoAlocacao.memoryTypeIndex = dispositivo.encontrarTipoMemoria(requisitos.memoryTypeBits, propriedades);

    verificarResultadoVulkan(
        vkAllocateMemory(dev, &infoAlocacao, nullptr, &memoriaImagem),
        "Falha ao alocar memória para a imagem Vulkan."
    );

    vkBindImageMemory(dev, imagem, memoriaImagem, 0);
}

VkImageView RecursosGraficos::criarVisualizacaoImagem(
    VkDevice dispositivoLogico,
    VkImage imagem,
    VkFormat formato,
    VkImageAspectFlags mascaraAspecto,
    uint32_t nivelMipBase,
    uint32_t totalNiveisMip) {

    VkImageViewCreateInfo infoVisualizacao{};
    infoVisualizacao.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    infoVisualizacao.image = imagem;
    infoVisualizacao.viewType = VK_IMAGE_VIEW_TYPE_2D;
    infoVisualizacao.format = formato;
    infoVisualizacao.subresourceRange.aspectMask = mascaraAspecto;
    infoVisualizacao.subresourceRange.baseMipLevel = nivelMipBase;
    infoVisualizacao.subresourceRange.levelCount = totalNiveisMip;
    infoVisualizacao.subresourceRange.baseArrayLayer = 0;
    infoVisualizacao.subresourceRange.layerCount = 1;

    VkImageView visualizacao = VK_NULL_HANDLE;
    verificarResultadoVulkan(
        vkCreateImageView(dispositivoLogico, &infoVisualizacao, nullptr, &visualizacao),
        "Falha ao criar visualização de imagem (VkImageView)."
    );

    return visualizacao;
}

void RecursosGraficos::transicionarLayoutImagemComando(
    VkCommandBuffer bufferComando,
    VkImage imagem,
    VkImageLayout layoutAntigo,
    VkImageLayout layoutNovo,
    VkAccessFlags acessoOrigem,
    VkAccessFlags acessoDestino,
    VkPipelineStageFlags estagioOrigem,
    VkPipelineStageFlags estagioDestino,
    uint32_t nivelMipBase,
    uint32_t totalNiveisMip) {

    VkImageMemoryBarrier barreira{};
    barreira.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barreira.oldLayout = layoutAntigo;
    barreira.newLayout = layoutNovo;
    barreira.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barreira.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barreira.image = imagem;
    barreira.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barreira.subresourceRange.baseMipLevel = nivelMipBase;
    barreira.subresourceRange.levelCount = totalNiveisMip;
    barreira.subresourceRange.baseArrayLayer = 0;
    barreira.subresourceRange.layerCount = 1;
    barreira.srcAccessMask = acessoOrigem;
    barreira.dstAccessMask = acessoDestino;

    vkCmdPipelineBarrier(
        bufferComando,
        estagioOrigem,
        estagioDestino,
        0,
        0, nullptr,
        0, nullptr,
        1, &barreira
    );
}

void RecursosGraficos::transicionarLayoutImagem(
    VkDevice dispositivoLogico,
    VkCommandPool poolComandos,
    VkQueue fila,
    VkImage imagem,
    VkFormat /* formato */,
    VkImageLayout layoutAntigo,
    VkImageLayout layoutNovo,
    uint32_t nivelMipBase,
    uint32_t totalNiveisMip) {

    VkCommandBufferAllocateInfo infoAlocacao{};
    infoAlocacao.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    infoAlocacao.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    infoAlocacao.commandPool = poolComandos;
    infoAlocacao.commandBufferCount = 1;

    VkCommandBuffer bufferComando;
    vkAllocateCommandBuffers(dispositivoLogico, &infoAlocacao, &bufferComando);

    VkCommandBufferBeginInfo infoInicio{};
    infoInicio.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    infoInicio.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(bufferComando, &infoInicio);

    VkImageMemoryBarrier barreira{};
    barreira.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barreira.oldLayout = layoutAntigo;
    barreira.newLayout = layoutNovo;
    barreira.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barreira.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barreira.image = imagem;
    barreira.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barreira.subresourceRange.baseMipLevel = nivelMipBase;
    barreira.subresourceRange.levelCount = totalNiveisMip;
    barreira.subresourceRange.baseArrayLayer = 0;
    barreira.subresourceRange.layerCount = 1;

    VkPipelineStageFlags estagioOrigem;
    VkPipelineStageFlags estagioDestino;

    if (layoutAntigo == VK_IMAGE_LAYOUT_UNDEFINED && layoutNovo == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barreira.srcAccessMask = 0;
        barreira.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        estagioOrigem = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        estagioDestino = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (layoutAntigo == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layoutNovo == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barreira.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barreira.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        estagioOrigem = VK_PIPELINE_STAGE_TRANSFER_BIT;
        estagioDestino = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        barreira.srcAccessMask = 0;
        barreira.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        estagioOrigem = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        estagioDestino = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(bufferComando, estagioOrigem, estagioDestino, 0, 0, nullptr, 0, nullptr, 1, &barreira);

    vkEndCommandBuffer(bufferComando);

    VkSubmitInfo submissao{};
    submissao.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submissao.commandBufferCount = 1;
    submissao.pCommandBuffers = &bufferComando;

    vkQueueSubmit(fila, 1, &submissao, VK_NULL_HANDLE);
    vkQueueWaitIdle(fila);

    vkFreeCommandBuffers(dispositivoLogico, poolComandos, 1, &bufferComando);
}

VkFormat RecursosGraficos::encontrarFormatoProfundidade(const DispositivoVulkan& dispositivo) {
    std::vector<VkFormat> formatosCandidatos = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat formato : formatosCandidatos) {
        VkFormatProperties propriedades;
        vkGetPhysicalDeviceFormatProperties(dispositivo.obterDispositivoFisico(), formato, &propriedades);

        if ((propriedades.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return formato;
        }
    }

    throw std::runtime_error("Falha ao encontrar formato de profundidade compatível na GPU.");
}

VkSampler RecursosGraficos::criarAmostradorLinear(VkDevice dispositivoLogico) {
    VkSamplerCreateInfo infoAmostrador{};
    infoAmostrador.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    infoAmostrador.magFilter = VK_FILTER_LINEAR;
    infoAmostrador.minFilter = VK_FILTER_LINEAR;
    infoAmostrador.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    infoAmostrador.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    infoAmostrador.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    infoAmostrador.anisotropyEnable = VK_FALSE;
    infoAmostrador.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    infoAmostrador.unnormalizedCoordinates = VK_FALSE;
    infoAmostrador.compareEnable = VK_FALSE;
    infoAmostrador.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    infoAmostrador.mipLodBias = 0.0f;
    infoAmostrador.minLod = 0.0f;
    infoAmostrador.maxLod = 0.0f;

    VkSampler amostrador = VK_NULL_HANDLE;
    verificarResultadoVulkan(
        vkCreateSampler(dispositivoLogico, &infoAmostrador, nullptr, &amostrador),
        "Falha ao criar amostrador linear com clamp to edge."
    );

    return amostrador;
}

VkSampler RecursosGraficos::criarAmostradorTextura(VkDevice dispositivoLogico) {
    VkSamplerCreateInfo infoAmostrador{};
    infoAmostrador.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    infoAmostrador.magFilter = VK_FILTER_LINEAR;
    infoAmostrador.minFilter = VK_FILTER_LINEAR;
    infoAmostrador.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    infoAmostrador.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    infoAmostrador.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    infoAmostrador.anisotropyEnable = VK_FALSE;
    infoAmostrador.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    infoAmostrador.unnormalizedCoordinates = VK_FALSE;
    infoAmostrador.compareEnable = VK_FALSE;
    infoAmostrador.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler amostrador = VK_NULL_HANDLE;
    verificarResultadoVulkan(
        vkCreateSampler(dispositivoLogico, &infoAmostrador, nullptr, &amostrador),
        "Falha ao criar amostrador de textura com repetição."
    );

    return amostrador;
}

ImagemVulkan RecursosGraficos::criarTexturaMaterialProcedural(
    const DispositivoVulkan& dispositivo,
    VkCommandPool poolComandos,
    VkQueue fila) {

    VkDevice dev = dispositivo.obterDispositivoLogico();
    const uint32_t LARGURA = 128;
    const uint32_t ALTURA = 128;
    VkDeviceSize tamanhoImagem = LARGURA * ALTURA * 4;

    std::vector<uint8_t> pixels(tamanhoImagem);

    // Gera textura com linhas de neon emissivas de alta intensidade:
    // O canal Alpha é o mapa de emissão (1.0 = emite luz, 0.0 = superfície opaca)
    for (uint32_t y = 0; y < ALTURA; y++) {
        for (uint32_t x = 0; x < LARGURA; x++) {
            uint32_t idx = (y * LARGURA + x) * 4;

            bool linhaNeonHorizontal = (y >= 54 && y <= 74);
            bool linhaNeonVertical   = (x >= 54 && x <= 74);
            bool ehNeon = linhaNeonHorizontal || linhaNeonVertical;

            if (ehNeon) {
                // Núcleo néon brilhante (ciano/turquesa intenso, canal Alpha = 255)
                pixels[idx + 0] = 60;   // R
                pixels[idx + 1] = 230;  // G
                pixels[idx + 2] = 255;  // B
                pixels[idx + 3] = 255;  // A (Emissivo = 1.0)
            } else {
                // Superfície escura de metal/polímero fosco com grade sutil
                bool grade = ((x / 16) + (y / 16)) % 2 == 0;
                uint8_t tom = grade ? 45 : 30;
                pixels[idx + 0] = tom;
                pixels[idx + 1] = tom;
                pixels[idx + 2] = tom + 5;
                pixels[idx + 3] = 0;    // A (Não emissivo = 0.0)
            }
        }
    }

    VkBuffer bufferStaging;
    VkDeviceMemory memoriaStaging;
    criarBuffer(
        dispositivo,
        tamanhoImagem,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferStaging,
        memoriaStaging
    );

    void* dados = nullptr;
    vkMapMemory(dev, memoriaStaging, 0, tamanhoImagem, 0, &dados);
    memcpy(dados, pixels.data(), static_cast<size_t>(tamanhoImagem));
    vkUnmapMemory(dev, memoriaStaging);

    ImagemVulkan resultado;
    resultado.extensao = { LARGURA, ALTURA };
    resultado.formato = VK_FORMAT_R8G8B8A8_UNORM;
    resultado.totalNiveisMip = 1;

    criarImagem(
        dispositivo,
        LARGURA,
        ALTURA,
        1,
        resultado.formato,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        resultado.imagem,
        resultado.memoria
    );

    transicionarLayoutImagem(
        dev, poolComandos, fila, resultado.imagem, resultado.formato,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // Cópia do buffer para a imagem
    VkCommandBufferAllocateInfo infoAlocacao{};
    infoAlocacao.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    infoAlocacao.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    infoAlocacao.commandPool = poolComandos;
    infoAlocacao.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(dev, &infoAlocacao, &cmd);

    VkCommandBufferBeginInfo infoInicio{};
    infoInicio.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    infoInicio.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &infoInicio);

    VkBufferImageCopy regiao{};
    regiao.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    regiao.imageSubresource.mipLevel = 0;
    regiao.imageSubresource.baseArrayLayer = 0;
    regiao.imageSubresource.layerCount = 1;
    regiao.imageExtent = { LARGURA, ALTURA, 1 };

    vkCmdCopyBufferToImage(cmd, bufferStaging, resultado.imagem, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regiao);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submissao{};
    submissao.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submissao.commandBufferCount = 1;
    submissao.pCommandBuffers = &cmd;
    vkQueueSubmit(fila, 1, &submissao, VK_NULL_HANDLE);
    vkQueueWaitIdle(fila);

    vkFreeCommandBuffers(dev, poolComandos, 1, &cmd);
    vkDestroyBuffer(dev, bufferStaging, nullptr);
    vkFreeMemory(dev, memoriaStaging, nullptr);

    transicionarLayoutImagem(
        dev, poolComandos, fila, resultado.imagem, resultado.formato,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    resultado.visualizacao = criarVisualizacaoImagem(dev, resultado.imagem, resultado.formato, VK_IMAGE_ASPECT_COLOR_BIT);

    return resultado;
}
