#pragma once

#include "Comum.hpp"
#include "DispositivoVulkan.hpp"
#include "GerenciadorSwapchain.hpp"
#include "CarregadorShader.hpp"
#include "RecursosGraficos.hpp"
#include "CenaGeometrica.hpp"

// Classe central responsável pela orquestração do Pipeline de Bloom completo (11 passes e 5 render passes)
class PipelineBloom {
public:
    static constexpr uint32_t TOTAL_NIVEIS_MIP = 5;

    PipelineBloom(
        const DispositivoVulkan& dispositivo,
        const GerenciadorSwapchain& swapchain,
        const CarregadorShader& carregadorShader,
        VkCommandPool poolComandos,
        VkQueue fila
    );
    ~PipelineBloom();

    PipelineBloom(const PipelineBloom&) = delete;
    PipelineBloom& operator=(const PipelineBloom&) = delete;

    void atualizarUBOs(const CameraUBO& camera, const ParametrosBloomUBO& bloom);

    void gravarComandosCompletos(
        VkCommandBuffer cmd,
        uint32_t indiceSwapchain,
        const CenaGeometrica& cena
    );

    void recriarRecursosDimensionais(
        const DispositivoVulkan& dispositivo,
        const GerenciadorSwapchain& swapchain
    );

private:
    void criarRecursosImagens(const DispositivoVulkan& dispositivo, VkExtent2D dimensao);
    void criarAmostradores(VkDevice dev);
    void criarRenderPasses(const DispositivoVulkan& dispositivo, VkFormat formatoSwapchain);
    void criarLayoutsDescritores(VkDevice dev);
    void criarPipelines(VkDevice dev, const CarregadorShader& carregador);
    void criarBuffersUBO(const DispositivoVulkan& dispositivo);
    void criarPoolEDescritores(VkDevice dev);
    void criarFramebuffers(VkDevice dev, const GerenciadorSwapchain& swapchain);
    void limparRecursosDependentesSwapchain(VkDevice dev);

    VkDevice dispositivoLogicoReferencia = VK_NULL_HANDLE;

    // Recursos de textura
    ImagemVulkan imagemSceneHDR;
    ImagemVulkan imagemProfundidade;
    ImagemVulkan imagemBloomMips;
    std::vector<VkImageView> visualizacoesMipsIndividuais;
    std::vector<VkExtent2D> dimensoesMips;

    ImagemVulkan texturaMaterialCena;
    VkSampler amostradorLinear = VK_NULL_HANDLE;
    VkSampler amostradorMaterial = VK_NULL_HANDLE;

    // Render Passes (5 passes descritos na Tabela 2 da Entrega 2)
    VkRenderPass renderPassCena = VK_NULL_HANDLE;
    VkRenderPass renderPassBright = VK_NULL_HANDLE;
    VkRenderPass renderPassDownsample = VK_NULL_HANDLE;
    VkRenderPass renderPassUpsample = VK_NULL_HANDLE;
    VkRenderPass renderPassComposite = VK_NULL_HANDLE;

    // Layouts de Pipeline e Pipelines
    VkPipelineLayout layoutCena = VK_NULL_HANDLE;
    VkPipeline pipelineCena = VK_NULL_HANDLE;

    VkPipelineLayout layoutBright = VK_NULL_HANDLE;
    VkPipeline pipelineBright = VK_NULL_HANDLE;

    VkPipelineLayout layoutDownsampleUpsample = VK_NULL_HANDLE;
    VkPipeline pipelineDownsample = VK_NULL_HANDLE;
    VkPipeline pipelineUpsample = VK_NULL_HANDLE;

    VkPipelineLayout layoutComposite = VK_NULL_HANDLE;
    VkPipeline pipelineComposite = VK_NULL_HANDLE;

    // Framebuffers
    VkFramebuffer framebufferCena = VK_NULL_HANDLE;
    VkFramebuffer framebufferBright = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffersDownsample; // Mip 1 a 4
    std::vector<VkFramebuffer> framebuffersUpsample;   // Mip 3 a 0
    std::vector<VkFramebuffer> framebuffersComposite;  // Swapchain

    // Descriptor Sets e Layouts
    VkDescriptorSetLayout layoutDescCena = VK_NULL_HANDLE;
    VkDescriptorSetLayout layoutDescBright = VK_NULL_HANDLE;
    VkDescriptorSetLayout layoutDescDownsampleUpsample = VK_NULL_HANDLE;
    VkDescriptorSetLayout layoutDescComposite = VK_NULL_HANDLE;

    VkDescriptorPool poolDescritores = VK_NULL_HANDLE;
    VkDescriptorSet descSetCena = VK_NULL_HANDLE;
    VkDescriptorSet descSetBright = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descSetsDownsample; // Lendo mip[i-1]
    std::vector<VkDescriptorSet> descSetsUpsample;   // Lendo mip[i+1]
    VkDescriptorSet descSetComposite = VK_NULL_HANDLE;

    // Buffers de Uniformes (UBOs)
    BufferVulkan bufferCamera;
    BufferVulkan bufferBloomParams;

    VkExtent2D extensaoAtual{};
};
