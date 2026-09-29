#pragma once

#include "Comum.hpp"
#include "DispositivoVulkan.hpp"
#include "Janela.hpp"

// Classe responsável pelo gerenciamento da Swapchain e suas Image Views
class GerenciadorSwapchain {
public:
    GerenciadorSwapchain(const DispositivoVulkan& dispositivo, const Janela& janela, VkSurfaceKHR superficie);
    ~GerenciadorSwapchain();

    GerenciadorSwapchain(const GerenciadorSwapchain&) = delete;
    GerenciadorSwapchain& operator=(const GerenciadorSwapchain&) = delete;

    VkSwapchainKHR obterSwapchain() const { return swapchain; }
    VkFormat obterFormatoImagem() const { return formatoImagemSwapchain; }
    VkExtent2D obterExtensao() const { return dimensaoSwapchain; }
    size_t obterTotalImagens() const { return imagensSwapchain.size(); }
    VkImage obterImagem(size_t indice) const { return imagensSwapchain.at(indice); }
    const std::vector<VkImageView>& obterVisualizacoesImagens() const { return visualizacoesImagensSwapchain; }
    VkImageView obterVisualizacaoImagem(size_t indice) const { return visualizacoesImagensSwapchain.at(indice); }

    void recriarSwapchain(const DispositivoVulkan& dispositivo, const Janela& janela, VkSurfaceKHR superficie);

private:
    void criarSwapchain(const DispositivoVulkan& dispositivo, const Janela& janela, VkSurfaceKHR superficie);
    void criarVisualizacoesDeImagem(VkDevice dispositivoLogico);
    void limparSwapchain(VkDevice dispositivoLogico);

    VkSurfaceFormatKHR escolherFormatoSuperficie(const std::vector<VkSurfaceFormatKHR>& formatosDisponiveis);
    VkPresentModeKHR escolherModoApresentacao(const std::vector<VkPresentModeKHR>& modosDisponiveis);
    VkExtent2D escolherDimensaoTroca(const VkSurfaceCapabilitiesKHR& capacidades, const Janela& janela);

    VkDevice dispositivoLogicoReferencia = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> imagensSwapchain;
    VkFormat formatoImagemSwapchain = VK_FORMAT_UNDEFINED;
    VkExtent2D dimensaoSwapchain{};
    std::vector<VkImageView> visualizacoesImagensSwapchain;
};
