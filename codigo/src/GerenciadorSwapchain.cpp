#include "GerenciadorSwapchain.hpp"

GerenciadorSwapchain::GerenciadorSwapchain(const DispositivoVulkan& dispositivo, const Janela& janela, VkSurfaceKHR superficie) {
    dispositivoLogicoReferencia = dispositivo.obterDispositivoLogico();
    criarSwapchain(dispositivo, janela, superficie);
    criarVisualizacoesDeImagem(dispositivoLogicoReferencia);
}

GerenciadorSwapchain::~GerenciadorSwapchain() {
    limparSwapchain(dispositivoLogicoReferencia);
}

void GerenciadorSwapchain::limparSwapchain(VkDevice dispositivoLogico) {
    for (auto visualizacao : visualizacoesImagensSwapchain) {
        vkDestroyImageView(dispositivoLogico, visualizacao, nullptr);
    }
    visualizacoesImagensSwapchain.clear();

    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(dispositivoLogico, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

void GerenciadorSwapchain::recriarSwapchain(const DispositivoVulkan& dispositivo, const Janela& janela, VkSurfaceKHR superficie) {
    // Tratamento de minimização de janela (espera até largura/altura > 0)
    int largura = 0, altura = 0;
    glfwGetFramebufferSize(janela.obterInstanciaGlfw(), &largura, &altura);
    while (largura == 0 || altura == 0) {
        glfwGetFramebufferSize(janela.obterInstanciaGlfw(), &largura, &altura);
        glfwWaitEvents();
    }

    dispositivo.esperarOcioso();

    limparSwapchain(dispositivo.obterDispositivoLogico());
    criarSwapchain(dispositivo, janela, superficie);
    criarVisualizacoesDeImagem(dispositivo.obterDispositivoLogico());
}

void GerenciadorSwapchain::criarSwapchain(const DispositivoVulkan& dispositivo, const Janela& janela, VkSurfaceKHR superficie) {
    DetalhesSuporteSwapchain suporte = dispositivo.obterDetalhesSuporteSwapchain();

    VkSurfaceFormatKHR formatoSuperficie = escolherFormatoSuperficie(suporte.formatos);
    VkPresentModeKHR modoApresentacao = escolherModoApresentacao(suporte.modosApresentacao);
    VkExtent2D dimensao = escolherDimensaoTroca(suporte.capacidades, janela);

    uint32_t totalImagens = suporte.capacidades.minImageCount + 1;
    if (suporte.capacidades.maxImageCount > 0 && totalImagens > suporte.capacidades.maxImageCount) {
        totalImagens = suporte.capacidades.maxImageCount;
    }

    VkSwapchainCreateInfoKHR infoCriacao{};
    infoCriacao.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    infoCriacao.surface = superficie;
    infoCriacao.minImageCount = totalImagens;
    infoCriacao.imageFormat = formatoSuperficie.format;
    infoCriacao.imageColorSpace = formatoSuperficie.colorSpace;
    infoCriacao.imageExtent = dimensao;
    infoCriacao.imageArrayLayers = 1;
    infoCriacao.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    FamiliasDeFilas indices = dispositivo.obterFamiliasFilas();
    uint32_t indicesFamiliasFilas[] = { indices.familiaGrafica.value(), indices.familiaApresentacao.value() };

    if (indices.familiaGrafica != indices.familiaApresentacao) {
        infoCriacao.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        infoCriacao.queueFamilyIndexCount = 2;
        infoCriacao.pQueueFamilyIndices = indicesFamiliasFilas;
    } else {
        infoCriacao.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        infoCriacao.queueFamilyIndexCount = 0;
        infoCriacao.pQueueFamilyIndices = nullptr;
    }

    infoCriacao.preTransform = suporte.capacidades.currentTransform;
    infoCriacao.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    infoCriacao.presentMode = modoApresentacao;
    infoCriacao.clipped = VK_TRUE;
    infoCriacao.oldSwapchain = VK_NULL_HANDLE;

    verificarResultadoVulkan(
        vkCreateSwapchainKHR(dispositivo.obterDispositivoLogico(), &infoCriacao, nullptr, &swapchain),
        "Falha ao criar a Swapchain Vulkan (VkSwapchainKHR)."
    );

    vkGetSwapchainImagesKHR(dispositivo.obterDispositivoLogico(), swapchain, &totalImagens, nullptr);
    imagensSwapchain.resize(totalImagens);
    vkGetSwapchainImagesKHR(dispositivo.obterDispositivoLogico(), swapchain, &totalImagens, imagensSwapchain.data());

    formatoImagemSwapchain = formatoSuperficie.format;
    dimensaoSwapchain = dimensao;
}

void GerenciadorSwapchain::criarVisualizacoesDeImagem(VkDevice dispositivoLogico) {
    visualizacoesImagensSwapchain.resize(imagensSwapchain.size());

    for (size_t i = 0; i < imagensSwapchain.size(); i++) {
        VkImageViewCreateInfo infoCriacao{};
        infoCriacao.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        infoCriacao.image = imagensSwapchain[i];
        infoCriacao.viewType = VK_IMAGE_VIEW_TYPE_2D;
        infoCriacao.format = formatoImagemSwapchain;

        infoCriacao.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        infoCriacao.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        infoCriacao.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        infoCriacao.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        infoCriacao.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        infoCriacao.subresourceRange.baseMipLevel = 0;
        infoCriacao.subresourceRange.levelCount = 1;
        infoCriacao.subresourceRange.baseArrayLayer = 0;
        infoCriacao.subresourceRange.layerCount = 1;

        verificarResultadoVulkan(
            vkCreateImageView(dispositivoLogico, &infoCriacao, nullptr, &visualizacoesImagensSwapchain[i]),
            "Falha ao criar a visualização de imagem (VkImageView) da swapchain."
        );
    }
}

VkSurfaceFormatKHR GerenciadorSwapchain::escolherFormatoSuperficie(const std::vector<VkSurfaceFormatKHR>& formatosDisponiveis) {
    for (const auto& formato : formatosDisponiveis) {
        if (formato.format == VK_FORMAT_B8G8R8A8_SRGB && formato.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formato;
        }
    }
    return formatosDisponiveis[0];
}

VkPresentModeKHR GerenciadorSwapchain::escolherModoApresentacao(const std::vector<VkPresentModeKHR>& modosDisponiveis) {
    for (const auto& modo : modosDisponiveis) {
        if (modo == VK_PRESENT_MODE_MAILBOX_KHR) {
            return modo; // Triple buffering
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR; // Garantido por padrão no Vulkan
}

VkExtent2D GerenciadorSwapchain::escolherDimensaoTroca(const VkSurfaceCapabilitiesKHR& capacidades, const Janela& janela) {
    if (capacidades.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capacidades.currentExtent;
    } else {
        int largura = 0, altura = 0;
        glfwGetFramebufferSize(janela.obterInstanciaGlfw(), &largura, &altura);

        VkExtent2D dimensaoReal = {
            static_cast<uint32_t>(largura),
            static_cast<uint32_t>(altura)
        };

        dimensaoReal.width = std::clamp(dimensaoReal.width, capacidades.minImageExtent.width, capacidades.maxImageExtent.width);
        dimensaoReal.height = std::clamp(dimensaoReal.height, capacidades.minImageExtent.height, capacidades.maxImageExtent.height);

        return dimensaoReal;
    }
}
