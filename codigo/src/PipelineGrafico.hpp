#pragma once

#include "Comum.hpp"
#include "DispositivoVulkan.hpp"
#include "GerenciadorSwapchain.hpp"

// Classe responsável pela criação e gerenciamento do Render Pass, Framebuffers e Pipeline Gráfico
class PipelineGrafico {
public:
    PipelineGrafico(const DispositivoVulkan& dispositivo,
                    const GerenciadorSwapchain& swapchain,
                    VkShaderModule moduloVertexShader,
                    VkShaderModule moduloFragmentShader);
    ~PipelineGrafico();

    PipelineGrafico(const PipelineGrafico&) = delete;
    PipelineGrafico& operator=(const PipelineGrafico&) = delete;

    VkRenderPass obterRenderPass() const { return renderPass; }
    VkPipeline obterPipeline() const { return pipelineGrafico; }
    VkPipelineLayout obterLayoutPipeline() const { return layoutPipeline; }
    VkFramebuffer obterFramebuffer(size_t indice) const { return framebuffersSwapchain.at(indice); }
    size_t obterTotalFramebuffers() const { return framebuffersSwapchain.size(); }

    void recriarFramebuffers(const DispositivoVulkan& dispositivo, const GerenciadorSwapchain& swapchain);

private:
    void criarRenderPass(const DispositivoVulkan& dispositivo, VkFormat formatoCor);
    void criarLayoutPipeline(VkDevice dispositivoLogico);
    void criarPipelineGrafico(VkDevice dispositivoLogico,
                              VkShaderModule moduloVertexShader,
                              VkShaderModule moduloFragmentShader);
    void criarFramebuffers(VkDevice dispositivoLogico, const GerenciadorSwapchain& swapchain);
    void limparFramebuffers(VkDevice dispositivoLogico);

    VkDevice dispositivoLogicoReferencia = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout layoutPipeline = VK_NULL_HANDLE;
    VkPipeline pipelineGrafico = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffersSwapchain;
};
