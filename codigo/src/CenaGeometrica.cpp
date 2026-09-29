#include "CenaGeometrica.hpp"
#include <cstring>

VkVertexInputBindingDescription VerticeCena::obterDescricaoVinculacao() {
    VkVertexInputBindingDescription descricao{};
    descricao.binding = 0;
    descricao.stride = sizeof(VerticeCena);
    descricao.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return descricao;
}

std::vector<VkVertexInputAttributeDescription> VerticeCena::obterDescricaoAtributos() {
    std::vector<VkVertexInputAttributeDescription> atributos(3);

    // layout(location = 0) in vec3 inPosition
    atributos[0].binding = 0;
    atributos[0].location = 0;
    atributos[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    atributos[0].offset = offsetof(VerticeCena, posicao);

    // layout(location = 1) in vec3 inNormal
    atributos[1].binding = 0;
    atributos[1].location = 1;
    atributos[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    atributos[1].offset = offsetof(VerticeCena, normal);

    // layout(location = 2) in vec2 inTexCoord
    atributos[2].binding = 0;
    atributos[2].location = 2;
    atributos[2].format = VK_FORMAT_R32G32_SFLOAT;
    atributos[2].offset = offsetof(VerticeCena, uv);

    return atributos;
}

CenaGeometrica::CenaGeometrica(const DispositivoVulkan& dispositivo, VkCommandPool poolComandos, VkQueue fila) {
    dispositivoLogicoReferencia = dispositivo.obterDispositivoLogico();
    criarGeometriaCubo(dispositivo, poolComandos, fila);
}

CenaGeometrica::~CenaGeometrica() {
    if (bufferVertices.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(dispositivoLogicoReferencia, bufferVertices.buffer, nullptr);
        vkFreeMemory(dispositivoLogicoReferencia, bufferVertices.memoria, nullptr);
    }
    if (bufferIndices.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(dispositivoLogicoReferencia, bufferIndices.buffer, nullptr);
        vkFreeMemory(dispositivoLogicoReferencia, bufferIndices.memoria, nullptr);
    }
}

void CenaGeometrica::criarGeometriaCubo(const DispositivoVulkan& dispositivo, VkCommandPool poolComandos, VkQueue fila) {
    VkDevice dev = dispositivo.obterDispositivoLogico();

    // 24 vértices para as 6 faces do cubo com normais e coordenadas UV
    const std::vector<VerticeCena> vertices = {
        // Face frontal (+Z)
        { {-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
        { { 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
        { { 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
        { {-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },

        // Face traseira (-Z)
        { { 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} },
        { {-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} },
        { {-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} },
        { { 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} },

        // Face superior (+Y)
        { {-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },

        // Face inferior (-Y)
        { {-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },

        // Face lateral direita (+X)
        { { 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { { 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        { { 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
        { { 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },

        // Face lateral esquerda (-X)
        { {-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { {-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        { {-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
        { {-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
    };

    const std::vector<uint32_t> indices = {
         0,  1,  2,   2,  3,  0, // Frente
         4,  5,  6,   6,  7,  4, // Trás
         8,  9, 10,  10, 11,  8, // Cima
        12, 13, 14,  14, 15, 12, // Baixo
        16, 17, 18,  18, 19, 16, // Direita
        20, 21, 22,  22, 23, 20  // Esquerda
    };

    totalIndices = static_cast<uint32_t>(indices.size());

    VkDeviceSize tamanhoVertices = sizeof(vertices[0]) * vertices.size();
    VkDeviceSize tamanhoIndices = sizeof(indices[0]) * indices.size();

    // Staging buffer para os vértices
    VkBuffer stagingVertices;
    VkDeviceMemory stagingMemVertices;
    RecursosGraficos::criarBuffer(
        dispositivo, tamanhoVertices,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingVertices, stagingMemVertices
    );

    void* dados = nullptr;
    vkMapMemory(dev, stagingMemVertices, 0, tamanhoVertices, 0, &dados);
    memcpy(dados, vertices.data(), static_cast<size_t>(tamanhoVertices));
    vkUnmapMemory(dev, stagingMemVertices);

    // Buffer de vértices na memória local rápida da GPU (DEVICE_LOCAL)
    RecursosGraficos::criarBuffer(
        dispositivo, tamanhoVertices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferVertices.buffer, bufferVertices.memoria
    );
    bufferVertices.tamanho = tamanhoVertices;

    // Staging buffer para os índices
    VkBuffer stagingIndices;
    VkDeviceMemory stagingMemIndices;
    RecursosGraficos::criarBuffer(
        dispositivo, tamanhoIndices,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingIndices, stagingMemIndices
    );

    vkMapMemory(dev, stagingMemIndices, 0, tamanhoIndices, 0, &dados);
    memcpy(dados, indices.data(), static_cast<size_t>(tamanhoIndices));
    vkUnmapMemory(dev, stagingMemIndices);

    // Buffer de índices na GPU
    RecursosGraficos::criarBuffer(
        dispositivo, tamanhoIndices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferIndices.buffer, bufferIndices.memoria
    );
    bufferIndices.tamanho = tamanhoIndices;

    // Comando para transferir dados dos buffers staging para os buffers locais da GPU
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

    VkBufferCopy copiaVertices{};
    copiaVertices.size = tamanhoVertices;
    vkCmdCopyBuffer(cmd, stagingVertices, bufferVertices.buffer, 1, &copiaVertices);

    VkBufferCopy copiaIndices{};
    copiaIndices.size = tamanhoIndices;
    vkCmdCopyBuffer(cmd, stagingIndices, bufferIndices.buffer, 1, &copiaIndices);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submissao{};
    submissao.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submissao.commandBufferCount = 1;
    submissao.pCommandBuffers = &cmd;
    vkQueueSubmit(fila, 1, &submissao, VK_NULL_HANDLE);
    vkQueueWaitIdle(fila);

    vkFreeCommandBuffers(dev, poolComandos, 1, &cmd);
    vkDestroyBuffer(dev, stagingVertices, nullptr);
    vkFreeMemory(dev, stagingMemVertices, nullptr);
    vkDestroyBuffer(dev, stagingIndices, nullptr);
    vkFreeMemory(dev, stagingMemIndices, nullptr);
}

void CenaGeometrica::desenhar(VkCommandBuffer bufferComando) const {
    VkBuffer buffers[] = { bufferVertices.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(bufferComando, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(bufferComando, bufferIndices.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(bufferComando, totalIndices, 1, 0, 0, 0);
}
