#pragma once

#include "Comum.hpp"
#include "Janela.hpp"
#include "DispositivoVulkan.hpp"
#include "GerenciadorSwapchain.hpp"
#include "CarregadorShader.hpp"
#include "RecursosGraficos.hpp"
#include "CenaGeometrica.hpp"
#include "PipelineBloom.hpp"

// Classe principal que orquestra a aplicação Vulkan com o Pipeline completo de Bloom
class AplicacaoVulkan {
public:
    AplicacaoVulkan(uint32_t largura = 1280, uint32_t altura = 720, const std::string& titulo = "Vulkan - Efeito Bloom e Pós-Processamento em Tempo Real");
    ~AplicacaoVulkan();

    AplicacaoVulkan(const AplicacaoVulkan&) = delete;
    AplicacaoVulkan& operator=(const AplicacaoVulkan&) = delete;

    void executar(uint32_t limiteQuadros = 0);
    void salvarCapturaDeQuadro(const std::string& caminhoArquivoPpm, uint32_t indiceImagem);

private:
    void inicializarVulkan();
    void criarPoolDeComandos();
    void criarBuffersDeComando();
    void criarObjetosDeSincronizacao();
    void gravarBufferComando(VkCommandBuffer bufferComando, uint32_t indiceImagem);
    void desenharQuadro();
    void recriarSwapchainEFila();
    void atualizarCenaEParametros();
    void limparRecursos();

    static void callbackTeclado(GLFWwindow* janela, int tecla, int scancode, int acao, int mods);

    static constexpr int LIMITE_QUADROS_EM_VOO = 2;

    Janela janela;
    std::unique_ptr<InstanciaVulkan> instancia;
    VkSurfaceKHR superficie = VK_NULL_HANDLE;
    std::unique_ptr<DispositivoVulkan> dispositivo;
    std::unique_ptr<GerenciadorSwapchain> swapchain;
    std::unique_ptr<CarregadorShader> carregadorShader;
    std::unique_ptr<CenaGeometrica> cenaGeometrica;
    std::unique_ptr<PipelineBloom> pipelineBloom;

    VkCommandPool poolDeComandos = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> buffersDeComando;

    std::vector<VkSemaphore> semaforosImagemDisponivel;
    std::vector<VkSemaphore> semaforosRenderizacaoConcluida;
    std::vector<VkFence> cercasEmVoo;

    uint32_t quadroAtual = 0;
    float anguloRotacao = 0.0f;
    double ultimoTempo = 0.0;
    bool rotacaoPausada = false;
    float velocidadeRotacao = 0.35f; // Rotação suave baseada em tempo (radianos por segundo)

    // Parâmetros do Bloom e controle interativo
    ParametrosBloomUBO parametrosBloom;
    bool bloomAtivado = true;
    float intensidadePadrao = 1.4f;
};
