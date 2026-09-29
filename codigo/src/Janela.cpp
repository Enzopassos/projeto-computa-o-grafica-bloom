#include "Janela.hpp"

Janela::Janela(uint32_t largura, uint32_t altura, const std::string& titulo)
    : larguraAtual(largura), alturaAtual(altura), tituloJanela(titulo) {
    if (!glfwInit()) {
        throw std::runtime_error("Falha ao inicializar a biblioteca GLFW.");
    }

    // Configura GLFW para não criar contexto OpenGL e permitir redimensionamento
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    janelaGlfw = glfwCreateWindow(static_cast<int>(larguraAtual),
                                  static_cast<int>(alturaAtual),
                                  tituloJanela.c_str(),
                                  nullptr,
                                  nullptr);
    if (!janelaGlfw) {
        glfwTerminate();
        throw std::runtime_error("Falha ao criar janela GLFW.");
    }

    glfwShowWindow(janelaGlfw);
    glfwFocusWindow(janelaGlfw);

    // Salva o ponteiro desta classe na janela para uso nos callbacks
    glfwSetWindowUserPointer(janelaGlfw, this);
    glfwSetFramebufferSizeCallback(janelaGlfw, callbackRedimensionamento);
}

Janela::~Janela() {
    if (janelaGlfw) {
        glfwDestroyWindow(janelaGlfw);
        janelaGlfw = nullptr;
    }
    glfwTerminate();
}

bool Janela::deveFechar() const {
    return glfwWindowShouldClose(janelaGlfw);
}

void Janela::processarEventos() const {
    glfwPollEvents();
}

void Janela::criarSuperficie(VkInstance instancia, VkSurfaceKHR* superficie) const {
    verificarResultadoVulkan(
        glfwCreateWindowSurface(instancia, janelaGlfw, nullptr, superficie),
        "Falha ao criar a superfície de janela Vulkan (VkSurfaceKHR)."
    );
}

void Janela::callbackRedimensionamento(GLFWwindow* janela, int novaLargura, int novaAltura) {
    auto aplicacaoJanela = reinterpret_cast<Janela*>(glfwGetWindowUserPointer(janela));
    if (aplicacaoJanela) {
        aplicacaoJanela->larguraAtual = static_cast<uint32_t>(novaLargura);
        aplicacaoJanela->alturaAtual = static_cast<uint32_t>(novaAltura);
        aplicacaoJanela->janelaFoiRedimensionada = true;
    }
}
