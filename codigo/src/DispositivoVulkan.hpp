#pragma once

#include "Comum.hpp"

// Callback oficial de depuração do Vulkan para as camadas de validação
VKAPI_ATTR VkBool32 VKAPI_CALL callbackDepuracaoVulkan(
    VkDebugUtilsMessageSeverityFlagBitsEXT severidadeMensagem,
    VkDebugUtilsMessageTypeFlagsEXT tiposMensagem,
    const VkDebugUtilsMessengerCallbackDataEXT* dadosCallback,
    void* dadosUsuario);

// Classe que gerencia a Instância Vulkan e Camadas de Validação (Princípio da Responsabilidade Única)
class InstanciaVulkan {
public:
    InstanciaVulkan(const std::string& nomeAplicacao, bool habilitarValidacao = true);
    ~InstanciaVulkan();

    InstanciaVulkan(const InstanciaVulkan&) = delete;
    InstanciaVulkan& operator=(const InstanciaVulkan&) = delete;

    VkInstance obterInstancia() const { return instancia; }
    bool validacaoAtiva() const { return validacaoHabilitada; }

    static const std::vector<const char*>& obterCamadasValidacao() {
        static const std::vector<const char*> camadas = { "VK_LAYER_KHRONOS_validation" };
        return camadas;
    }

private:
    void criarInstancia(const std::string& nomeAplicacao);
    void configurarMensageiroDepuracao();
    bool verificarSuporteCamadasValidacao() const;
    std::vector<const char*> obterExtensoesNecessarias() const;

    VkInstance instancia = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mensageiroDepuracao = VK_NULL_HANDLE;
    bool validacaoHabilitada = true;
};

// Classe responsável pela seleção da GPU e criação do Dispositivo Lógico
class DispositivoVulkan {
public:
    DispositivoVulkan(VkInstance instancia, VkSurfaceKHR superficie);
    ~DispositivoVulkan();

    DispositivoVulkan(const DispositivoVulkan&) = delete;
    DispositivoVulkan& operator=(const DispositivoVulkan&) = delete;

    VkDevice obterDispositivoLogico() const { return dispositivoLogico; }
    VkPhysicalDevice obterDispositivoFisico() const { return dispositivoFisico; }
    VkQueue obterFilaGrafica() const { return filaGrafica; }
    VkQueue obterFilaApresentacao() const { return filaApresentacao; }
    const FamiliasDeFilas& obterFamiliasFilas() const { return familiasFilas; }

    DetalhesSuporteSwapchain obterDetalhesSuporteSwapchain() const {
        return consultarSuporteSwapchain(dispositivoFisico);
    }

    uint32_t encontrarTipoMemoria(uint32_t filtroTipo, VkMemoryPropertyFlags propriedades) const;
    void esperarOcioso() const;

private:
    void selecionarGpuFisica();
    void criarDispositivoLogico();

    bool verificarAdequacaoDispositivo(VkPhysicalDevice dispositivo);
    FamiliasDeFilas encontrarFamiliasDeFilas(VkPhysicalDevice dispositivo) const;
    bool verificarSuporteExtensoesDispositivo(VkPhysicalDevice dispositivo) const;
    DetalhesSuporteSwapchain consultarSuporteSwapchain(VkPhysicalDevice dispositivo) const;

    VkInstance instanciaReferencia = VK_NULL_HANDLE;
    VkSurfaceKHR superficieJanela = VK_NULL_HANDLE;
    VkPhysicalDevice dispositivoFisico = VK_NULL_HANDLE;
    VkDevice dispositivoLogico = VK_NULL_HANDLE;
    VkQueue filaGrafica = VK_NULL_HANDLE;
    VkQueue filaApresentacao = VK_NULL_HANDLE;
    FamiliasDeFilas familiasFilas;

    static inline const std::vector<const char*> extensoesDispositivoObrigatorias = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};
