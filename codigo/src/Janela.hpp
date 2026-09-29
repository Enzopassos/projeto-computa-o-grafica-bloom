#pragma once

#include "Comum.hpp"

// Classe responsável exclusivamente pelo gerenciamento da janela do sistema operacional (Princípio da Responsabilidade Única)
class Janela {
public:
    Janela(uint32_t largura, uint32_t altura, const std::string& titulo);
    ~Janela();

    // Impede cópia para proteger o ponteiro da janela GLFW
    Janela(const Janela&) = delete;
    Janela& operator=(const Janela&) = delete;

    bool deveFechar() const;
    void processarEventos() const;
    void criarSuperficie(VkInstance instancia, VkSurfaceKHR* superficie) const;

    uint32_t obterLargura() const { return larguraAtual; }
    uint32_t obterAltura() const { return alturaAtual; }
    GLFWwindow* obterInstanciaGlfw() const { return janelaGlfw; }
    bool foiRedimensionada() const { return janelaFoiRedimensionada; }
    void redefinirSinalRedimensionamento() { janelaFoiRedimensionada = false; }

private:
    static void callbackRedimensionamento(GLFWwindow* janela, int novaLargura, int novaAltura);

    uint32_t larguraAtual;
    uint32_t alturaAtual;
    std::string tituloJanela;
    GLFWwindow* janelaGlfw = nullptr;
    bool janelaFoiRedimensionada = false;
};
