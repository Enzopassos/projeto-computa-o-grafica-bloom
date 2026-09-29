#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <memory>

// Macro utilitária para verificar retornos da API Vulkan seguindo a diretriz Fail-Fast
inline void verificarResultadoVulkan(VkResult resultado, const std::string& mensagem) {
    if (resultado != VK_SUCCESS) {
        throw std::runtime_error(mensagem + " [Erro Vulkan: " + std::to_string(resultado) + "]");
    }
}

// Estrutura que armazena os índices das famílias de filas da GPU
struct FamiliasDeFilas {
    std::optional<uint32_t> familiaGrafica;
    std::optional<uint32_t> familiaApresentacao;

    bool estaCompleto() const {
        return familiaGrafica.has_value() && familiaApresentacao.has_value();
    }
};

// Estrutura com detalhes sobre o suporte de apresentação na janela
struct DetalhesSuporteSwapchain {
    VkSurfaceCapabilitiesKHR capacidades{};
    std::vector<VkSurfaceFormatKHR> formatos;
    std::vector<VkPresentModeKHR> modosApresentacao;
};
