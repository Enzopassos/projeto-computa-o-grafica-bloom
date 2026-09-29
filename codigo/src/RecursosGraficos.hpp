#pragma once

#include "Comum.hpp"
#include "DispositivoVulkan.hpp"
#include <glm/glm.hpp>

// Estrutura auxiliar para empacotar uma imagem e sua memória alocada
struct ImagemVulkan {
    VkImage imagem = VK_NULL_HANDLE;
    VkDeviceMemory memoria = VK_NULL_HANDLE;
    VkImageView visualizacao = VK_NULL_HANDLE;
    VkFormat formato = VK_FORMAT_UNDEFINED;
    VkExtent2D extensao{};
    uint32_t totalNiveisMip = 1;
};

// Estrutura auxiliar para empacotar um buffer e sua memória alocada
struct BufferVulkan {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memoria = VK_NULL_HANDLE;
    VkDeviceSize tamanho = 0;
};

// Classe utilitária responsável pela alocação e gerenciamento de recursos de baixo nível da GPU (Princípio da Responsabilidade Única)
class RecursosGraficos {
public:
    static void criarBuffer(
        const DispositivoVulkan& dispositivo,
        VkDeviceSize tamanho,
        VkBufferUsageFlags uso,
        VkMemoryPropertyFlags propriedades,
        VkBuffer& buffer,
        VkDeviceMemory& memoriaBuffer
    );

    static void criarImagem(
        const DispositivoVulkan& dispositivo,
        uint32_t largura,
        uint32_t altura,
        uint32_t niveisMip,
        VkFormat formato,
        VkImageTiling arranjo,
        VkImageUsageFlags uso,
        VkMemoryPropertyFlags propriedades,
        VkImage& imagem,
        VkDeviceMemory& memoriaImagem
    );

    static VkImageView criarVisualizacaoImagem(
        VkDevice dispositivoLogico,
        VkImage imagem,
        VkFormat formato,
        VkImageAspectFlags mascaraAspecto,
        uint32_t nivelMipBase = 0,
        uint32_t totalNiveisMip = 1
    );

    static void transicionarLayoutImagem(
        VkDevice dispositivoLogico,
        VkCommandPool poolComandos,
        VkQueue fila,
        VkImage imagem,
        VkFormat formato,
        VkImageLayout layoutAntigo,
        VkImageLayout layoutNovo,
        uint32_t nivelMipBase = 0,
        uint32_t totalNiveisMip = 1
    );

    static void transicionarLayoutImagemComando(
        VkCommandBuffer bufferComando,
        VkImage imagem,
        VkImageLayout layoutAntigo,
        VkImageLayout layoutNovo,
        VkAccessFlags acessoOrigem,
        VkAccessFlags acessoDestino,
        VkPipelineStageFlags estagioOrigem,
        VkPipelineStageFlags estagioDestino,
        uint32_t nivelMipBase = 0,
        uint32_t totalNiveisMip = 1
    );

    static VkFormat encontrarFormatoProfundidade(const DispositivoVulkan& dispositivo);

    static VkSampler criarAmostradorLinear(VkDevice dispositivoLogico);
    static VkSampler criarAmostradorTextura(VkDevice dispositivoLogico);

    static ImagemVulkan criarTexturaMaterialProcedural(
        const DispositivoVulkan& dispositivo,
        VkCommandPool poolComandos,
        VkQueue fila
    );
};
