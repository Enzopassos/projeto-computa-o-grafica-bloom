#include "DispositivoVulkan.hpp"
#include <cstring>
#include <set>

// Funções auxiliares para carregar dinamicamente as extensões de depuração do Vulkan
static VkResult CriarDebugUtilsMessengerEXT(
    VkInstance instancia,
    const VkDebugUtilsMessengerCreateInfoEXT* pInfoCriacao,
    const VkAllocationCallbacks* pAlocador,
    VkDebugUtilsMessengerEXT* pMensageiro) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instancia, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instancia, pInfoCriacao, pAlocador, pMensageiro);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestruirDebugUtilsMessengerEXT(
    VkInstance instancia,
    VkDebugUtilsMessengerEXT mensageiro,
    const VkAllocationCallbacks* pAlocador) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instancia, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instancia, mensageiro, pAlocador);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL callbackDepuracaoVulkan(
    VkDebugUtilsMessageSeverityFlagBitsEXT severidadeMensagem,
    VkDebugUtilsMessageTypeFlagsEXT /* tiposMensagem */,
    const VkDebugUtilsMessengerCallbackDataEXT* dadosCallback,
    void* /* dadosUsuario */) {
    
    if (severidadeMensagem >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[Camada de Validação Vulkan]: " << dadosCallback->pMessage << std::endl;
    }
    return VK_FALSE;
}

// ============================================================================
// Implementação de InstanciaVulkan
// ============================================================================

InstanciaVulkan::InstanciaVulkan(const std::string& nomeAplicacao, bool habilitarValidacao)
    : validacaoHabilitada(habilitarValidacao) {
    criarInstancia(nomeAplicacao);
    configurarMensageiroDepuracao();
}

InstanciaVulkan::~InstanciaVulkan() {
    if (validacaoHabilitada && mensageiroDepuracao != VK_NULL_HANDLE) {
        DestruirDebugUtilsMessengerEXT(instancia, mensageiroDepuracao, nullptr);
        mensageiroDepuracao = VK_NULL_HANDLE;
    }
    if (instancia != VK_NULL_HANDLE) {
        vkDestroyInstance(instancia, nullptr);
        instancia = VK_NULL_HANDLE;
    }
}

bool InstanciaVulkan::verificarSuporteCamadasValidacao() const {
    uint32_t totalCamadas = 0;
    vkEnumerateInstanceLayerProperties(&totalCamadas, nullptr);

    std::vector<VkLayerProperties> camadasDisponiveis(totalCamadas);
    vkEnumerateInstanceLayerProperties(&totalCamadas, camadasDisponiveis.data());

    for (const char* nomeCamada : obterCamadasValidacao()) {
        bool camadaEncontrada = false;
        for (const auto& propriedadesCamada : camadasDisponiveis) {
            if (strcmp(nomeCamada, propriedadesCamada.layerName) == 0) {
                camadaEncontrada = true;
                break;
            }
        }
        if (!camadaEncontrada) {
            return false;
        }
    }
    return true;
}

std::vector<const char*> InstanciaVulkan::obterExtensoesNecessarias() const {
    uint32_t totalExtensoesGlfw = 0;
    const char** extensoesGlfw = glfwGetRequiredInstanceExtensions(&totalExtensoesGlfw);

    std::vector<const char*> extensoes(extensoesGlfw, extensoesGlfw + totalExtensoesGlfw);

    if (validacaoHabilitada) {
        extensoes.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensoes;
}

void InstanciaVulkan::criarInstancia(const std::string& nomeAplicacao) {
    if (validacaoHabilitada && !verificarSuporteCamadasValidacao()) {
        std::cerr << "Aviso: Camadas de validação solicitadas, mas não disponíveis no sistema. Desativando..." << std::endl;
        validacaoHabilitada = false;
    }

    VkApplicationInfo infoAplicacao{};
    infoAplicacao.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    infoAplicacao.pApplicationName = nomeAplicacao.c_str();
    infoAplicacao.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    infoAplicacao.pEngineName = "MotorBloomVulkan";
    infoAplicacao.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    infoAplicacao.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo infoInstancia{};
    infoInstancia.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    infoInstancia.pApplicationInfo = &infoAplicacao;

    auto extensoes = obterExtensoesNecessarias();
    infoInstancia.enabledExtensionCount = static_cast<uint32_t>(extensoes.size());
    infoInstancia.ppEnabledExtensionNames = extensoes.data();

    VkDebugUtilsMessengerCreateInfoEXT infoDepuracaoInstancia{};
    if (validacaoHabilitada) {
        const auto& camadas = obterCamadasValidacao();
        infoInstancia.enabledLayerCount = static_cast<uint32_t>(camadas.size());
        infoInstancia.ppEnabledLayerNames = camadas.data();

        infoDepuracaoInstancia.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        infoDepuracaoInstancia.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        infoDepuracaoInstancia.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        infoDepuracaoInstancia.pfnUserCallback = callbackDepuracaoVulkan;
        infoInstancia.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&infoDepuracaoInstancia;
    } else {
        infoInstancia.enabledLayerCount = 0;
        infoInstancia.pNext = nullptr;
    }

    verificarResultadoVulkan(
        vkCreateInstance(&infoInstancia, nullptr, &instancia),
        "Falha ao criar a instância Vulkan (VkInstance)."
    );
}

void InstanciaVulkan::configurarMensageiroDepuracao() {
    if (!validacaoHabilitada) return;

    VkDebugUtilsMessengerCreateInfoEXT infoCriacao{};
    infoCriacao.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    infoCriacao.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    infoCriacao.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    infoCriacao.pfnUserCallback = callbackDepuracaoVulkan;

    verificarResultadoVulkan(
        CriarDebugUtilsMessengerEXT(instancia, &infoCriacao, nullptr, &mensageiroDepuracao),
        "Falha ao configurar o mensageiro de depuração Vulkan."
    );
}

// ============================================================================
// Implementação de DispositivoVulkan
// ============================================================================

DispositivoVulkan::DispositivoVulkan(VkInstance instancia, VkSurfaceKHR superficie)
    : instanciaReferencia(instancia), superficieJanela(superficie) {
    selecionarGpuFisica();
    criarDispositivoLogico();
}

DispositivoVulkan::~DispositivoVulkan() {
    if (dispositivoLogico != VK_NULL_HANDLE) {
        vkDestroyDevice(dispositivoLogico, nullptr);
        dispositivoLogico = VK_NULL_HANDLE;
    }
}

void DispositivoVulkan::selecionarGpuFisica() {
    uint32_t totalDispositivos = 0;
    vkEnumeratePhysicalDevices(instanciaReferencia, &totalDispositivos, nullptr);

    if (totalDispositivos == 0) {
        throw std::runtime_error("Nenhuma GPU com suporte ao Vulkan foi detectada.");
    }

    std::vector<VkPhysicalDevice> dispositivos(totalDispositivos);
    vkEnumeratePhysicalDevices(instanciaReferencia, &totalDispositivos, dispositivos.data());

    // Procura prioritariamente por GPU discreta (dedicada)
    for (const auto& disp : dispositivos) {
        if (verificarAdequacaoDispositivo(disp)) {
            VkPhysicalDeviceProperties propriedades;
            vkGetPhysicalDeviceProperties(disp, &propriedades);
            if (propriedades.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                dispositivoFisico = disp;
                std::cout << "GPU Dedicada Selecionada: " << propriedades.deviceName << std::endl;
                break;
            }
        }
    }

    // Caso não haja GPU discreta adequada, aceita a primeira GPU que atenda aos requisitos
    if (dispositivoFisico == VK_NULL_HANDLE) {
        for (const auto& disp : dispositivos) {
            if (verificarAdequacaoDispositivo(disp)) {
                dispositivoFisico = disp;
                VkPhysicalDeviceProperties propriedades;
                vkGetPhysicalDeviceProperties(disp, &propriedades);
                std::cout << "GPU Integrada Selecionada: " << propriedades.deviceName << std::endl;
                break;
            }
        }
    }

    if (dispositivoFisico == VK_NULL_HANDLE) {
        throw std::runtime_error("Nenhuma GPU atendeu aos requisitos mínimos para o pipeline gráfico.");
    }

    familiasFilas = encontrarFamiliasDeFilas(dispositivoFisico);
}

bool DispositivoVulkan::verificarAdequacaoDispositivo(VkPhysicalDevice dispositivo) {
    FamiliasDeFilas indices = encontrarFamiliasDeFilas(dispositivo);
    bool suportaExtensoes = verificarSuporteExtensoesDispositivo(dispositivo);

    bool swapchainAdequada = false;
    if (suportaExtensoes) {
        DetalhesSuporteSwapchain suporte = consultarSuporteSwapchain(dispositivo);
        swapchainAdequada = !suporte.formatos.empty() && !suporte.modosApresentacao.empty();
    }

    return indices.estaCompleto() && suportaExtensoes && swapchainAdequada;
}

FamiliasDeFilas DispositivoVulkan::encontrarFamiliasDeFilas(VkPhysicalDevice dispositivo) const {
    FamiliasDeFilas indices;

    uint32_t totalFamilias = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dispositivo, &totalFamilias, nullptr);

    std::vector<VkQueueFamilyProperties> familias(totalFamilias);
    vkGetPhysicalDeviceQueueFamilyProperties(dispositivo, &totalFamilias, familias.data());

    uint32_t i = 0;
    for (const auto& familia : familias) {
        if (familia.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.familiaGrafica = i;
        }

        VkBool32 suportaApresentacao = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dispositivo, i, superficieJanela, &suportaApresentacao);

        if (suportaApresentacao) {
            indices.familiaApresentacao = i;
        }

        if (indices.estaCompleto()) {
            break;
        }
        i++;
    }

    return indices;
}

bool DispositivoVulkan::verificarSuporteExtensoesDispositivo(VkPhysicalDevice dispositivo) const {
    uint32_t totalExtensoes = 0;
    vkEnumerateDeviceExtensionProperties(dispositivo, nullptr, &totalExtensoes, nullptr);

    std::vector<VkExtensionProperties> extensoesDisponiveis(totalExtensoes);
    vkEnumerateDeviceExtensionProperties(dispositivo, nullptr, &totalExtensoes, extensoesDisponiveis.data());

    std::set<std::string> extensoesNecessarias(extensoesDispositivoObrigatorias.begin(),
                                               extensoesDispositivoObrigatorias.end());

    for (const auto& ext : extensoesDisponiveis) {
        extensoesNecessarias.erase(ext.extensionName);
    }

    return extensoesNecessarias.empty();
}

DetalhesSuporteSwapchain DispositivoVulkan::consultarSuporteSwapchain(VkPhysicalDevice dispositivo) const {
    DetalhesSuporteSwapchain suporte;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dispositivo, superficieJanela, &suporte.capacidades);

    uint32_t totalFormatos = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dispositivo, superficieJanela, &totalFormatos, nullptr);

    if (totalFormatos != 0) {
        suporte.formatos.resize(totalFormatos);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dispositivo, superficieJanela, &totalFormatos, suporte.formatos.data());
    }

    uint32_t totalModos = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dispositivo, superficieJanela, &totalModos, nullptr);

    if (totalModos != 0) {
        suporte.modosApresentacao.resize(totalModos);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dispositivo, superficieJanela, &totalModos, suporte.modosApresentacao.data());
    }

    return suporte;
}

void DispositivoVulkan::criarDispositivoLogico() {
    std::vector<VkDeviceQueueCreateInfo> infosFilas;
    std::set<uint32_t> familiasUnicas = {
        familiasFilas.familiaGrafica.value(),
        familiasFilas.familiaApresentacao.value()
    };

    float prioridadeFila = 1.0f;
    for (uint32_t familia : familiasUnicas) {
        VkDeviceQueueCreateInfo infoFila{};
        infoFila.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        infoFila.queueFamilyIndex = familia;
        infoFila.queueCount = 1;
        infoFila.pQueuePriorities = &prioridadeFila;
        infosFilas.push_back(infoFila);
    }

    VkPhysicalDeviceFeatures recursosDispositivo{};
    recursosDispositivo.samplerAnisotropy = VK_FALSE;

    VkDeviceCreateInfo infoCriacaoDispositivo{};
    infoCriacaoDispositivo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    infoCriacaoDispositivo.queueCreateInfoCount = static_cast<uint32_t>(infosFilas.size());
    infoCriacaoDispositivo.pQueueCreateInfos = infosFilas.data();
    infoCriacaoDispositivo.pEnabledFeatures = &recursosDispositivo;
    infoCriacaoDispositivo.enabledExtensionCount = static_cast<uint32_t>(extensoesDispositivoObrigatorias.size());
    infoCriacaoDispositivo.ppEnabledExtensionNames = extensoesDispositivoObrigatorias.data();

    // Compatibilidade com versões mais antigas do Vulkan
    const auto& camadas = InstanciaVulkan::obterCamadasValidacao();
    infoCriacaoDispositivo.enabledLayerCount = static_cast<uint32_t>(camadas.size());
    infoCriacaoDispositivo.ppEnabledLayerNames = camadas.data();

    verificarResultadoVulkan(
        vkCreateDevice(dispositivoFisico, &infoCriacaoDispositivo, nullptr, &dispositivoLogico),
        "Falha ao criar o dispositivo lógico Vulkan (VkDevice)."
    );

    vkGetDeviceQueue(dispositivoLogico, familiasFilas.familiaGrafica.value(), 0, &filaGrafica);
    vkGetDeviceQueue(dispositivoLogico, familiasFilas.familiaApresentacao.value(), 0, &filaApresentacao);
}

uint32_t DispositivoVulkan::encontrarTipoMemoria(uint32_t filtroTipo, VkMemoryPropertyFlags propriedades) const {
    VkPhysicalDeviceMemoryProperties propriedadesMemoria;
    vkGetPhysicalDeviceMemoryProperties(dispositivoFisico, &propriedadesMemoria);

    for (uint32_t i = 0; i < propriedadesMemoria.memoryTypeCount; i++) {
        if ((filtroTipo & (1 << i)) &&
            (propriedadesMemoria.memoryTypes[i].propertyFlags & propriedades) == propriedades) {
            return i;
        }
    }

    throw std::runtime_error("Falha ao encontrar o tipo de memória compatível na GPU.");
}

void DispositivoVulkan::esperarOcioso() const {
    if (dispositivoLogico != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(dispositivoLogico);
    }
}
