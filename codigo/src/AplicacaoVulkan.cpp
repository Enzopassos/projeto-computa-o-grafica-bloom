#include "AplicacaoVulkan.hpp"
#include <iostream>

AplicacaoVulkan::AplicacaoVulkan(uint32_t largura, uint32_t altura, const std::string& titulo)
    : janela(largura, altura, titulo) {
    inicializarVulkan();
}

AplicacaoVulkan::~AplicacaoVulkan() {
    limparRecursos();
}

void AplicacaoVulkan::inicializarVulkan() {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "Inicializando Protótipo Completo de Bloom em Vulkan..." << std::endl;
    std::cout << "========================================================" << std::endl;

    // 1. Instância e Superfície
    instancia = std::make_unique<InstanciaVulkan>("Prototipo Bloom Vulkan", true);
    janela.criarSuperficie(instancia->obterInstancia(), &superficie);

    // 2. Dispositivo e Swapchain
    dispositivo = std::make_unique<DispositivoVulkan>(instancia->obterInstancia(), superficie);
    swapchain = std::make_unique<GerenciadorSwapchain>(*dispositivo, janela, superficie);

    // 3. Shaders
    carregadorShader = std::make_unique<CarregadorShader>(dispositivo->obterDispositivoLogico());
    carregadorShader->carregarTodosShadersDoProjeto("shaders/spv");

    // 4. Filas e Comandos
    criarPoolDeComandos();
    criarBuffersDeComando();
    criarObjetosDeSincronizacao();

    // 5. Geometria 3D e Texturas do Material (Passe 0)
    std::cout << "Configurando geometria 3D e texturas com mapas de emissão..." << std::endl;
    cenaGeometrica = std::make_unique<CenaGeometrica>(*dispositivo, poolDeComandos, dispositivo->obterFilaGrafica());

    // 6. Pipeline do Bloom Completo (11 Passes)
    std::cout << "Configurando a cadeia completa do Pipeline de Bloom (11 Passes)..." << std::endl;
    pipelineBloom = std::make_unique<PipelineBloom>(
        *dispositivo, *swapchain, *carregadorShader, poolDeComandos, dispositivo->obterFilaGrafica()
    );

    // 7. Parâmetros Iniciais
    parametrosBloom.threshold = 1.0f;
    parametrosBloom.knee = 0.5f;
    parametrosBloom.intensity = intensidadePadrao;

    // 8. Callbacks de Entrada do Usuário
    ultimoTempo = glfwGetTime();
    glfwSetWindowUserPointer(janela.obterInstanciaGlfw(), this);
    glfwSetKeyCallback(janela.obterInstanciaGlfw(), callbackTeclado);

    std::cout << "\n[CONTROLES INTERATIVOS]:" << std::endl;
    std::cout << "  - Tecla [ESPAÇO]: Liga / Desliga o efeito de Bloom (Comparativo)" << std::endl;
    std::cout << "  - Tecla [P]:      Pausa / Retoma a rotação do cubo" << std::endl;
    std::cout << "  - Tecla [+]:      Aumenta a intensidade do brilho" << std::endl;
    std::cout << "  - Tecla [-]:      Diminui a intensidade do brilho" << std::endl;
    std::cout << "========================================================\n" << std::endl;
}

void AplicacaoVulkan::callbackTeclado(GLFWwindow* janelaGlfw, int tecla, int /* scancode */, int acao, int /* mods */) {
    if (acao != GLFW_PRESS && acao != GLFW_REPEAT) return;

    auto app = reinterpret_cast<AplicacaoVulkan*>(glfwGetWindowUserPointer(janelaGlfw));
    if (!app) return;

    if (tecla == GLFW_KEY_SPACE) {
        app->bloomAtivado = !app->bloomAtivado;
        app->parametrosBloom.intensity = app->bloomAtivado ? app->intensidadePadrao : 0.0f;
        std::cout << "\n>>> [ESTADO]: Bloom " << (app->bloomAtivado ? "ATIVADO" : "DESATIVADO")
                  << " (Intensidade: " << app->parametrosBloom.intensity << ")" << std::endl;
    } else if (tecla == GLFW_KEY_P) {
        app->rotacaoPausada = !app->rotacaoPausada;
        std::cout << "\n>>> [ESTADO]: Rotação do cubo " << (app->rotacaoPausada ? "PAUSADA" : "RETOMADA") << std::endl;
    } else if (tecla == GLFW_KEY_EQUAL || tecla == GLFW_KEY_KP_ADD) {
        app->intensidadePadrao += 0.25f;
        if (app->bloomAtivado) app->parametrosBloom.intensity = app->intensidadePadrao;
        std::cout << ">>> [INTENSIDADE]: " << app->intensidadePadrao << std::endl;
    } else if (tecla == GLFW_KEY_MINUS || tecla == GLFW_KEY_KP_SUBTRACT) {
        app->intensidadePadrao = std::max(0.0f, app->intensidadePadrao - 0.25f);
        if (app->bloomAtivado) app->parametrosBloom.intensity = app->intensidadePadrao;
        std::cout << ">>> [INTENSIDADE]: " << app->intensidadePadrao << std::endl;
    } else if (tecla == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(janelaGlfw, GLFW_TRUE);
    }
}

void AplicacaoVulkan::criarPoolDeComandos() {
    FamiliasDeFilas indices = dispositivo->obterFamiliasFilas();

    VkCommandPoolCreateInfo infoPool{};
    infoPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    infoPool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    infoPool.queueFamilyIndex = indices.familiaGrafica.value();

    verificarResultadoVulkan(
        vkCreateCommandPool(dispositivo->obterDispositivoLogico(), &infoPool, nullptr, &poolDeComandos),
        "Falha ao criar o pool de comandos do Vulkan."
    );
}

void AplicacaoVulkan::criarBuffersDeComando() {
    buffersDeComando.resize(LIMITE_QUADROS_EM_VOO);

    VkCommandBufferAllocateInfo infoAlocacao{};
    infoAlocacao.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    infoAlocacao.commandPool = poolDeComandos;
    infoAlocacao.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    infoAlocacao.commandBufferCount = static_cast<uint32_t>(buffersDeComando.size());

    verificarResultadoVulkan(
        vkAllocateCommandBuffers(dispositivo->obterDispositivoLogico(), &infoAlocacao, buffersDeComando.data()),
        "Falha ao alocar buffers de comando Vulkan."
    );
}

void AplicacaoVulkan::criarObjetosDeSincronizacao() {
    semaforosImagemDisponivel.resize(LIMITE_QUADROS_EM_VOO);
    semaforosRenderizacaoConcluida.resize(LIMITE_QUADROS_EM_VOO);
    cercasEmVoo.resize(LIMITE_QUADROS_EM_VOO);

    VkSemaphoreCreateInfo infoSemaforo{};
    infoSemaforo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo infoCerca{};
    infoCerca.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    infoCerca.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < LIMITE_QUADROS_EM_VOO; i++) {
        verificarResultadoVulkan(
            vkCreateSemaphore(dispositivo->obterDispositivoLogico(), &infoSemaforo, nullptr, &semaforosImagemDisponivel[i]),
            "Falha ao criar semáforo de imagem disponível."
        );
        verificarResultadoVulkan(
            vkCreateSemaphore(dispositivo->obterDispositivoLogico(), &infoSemaforo, nullptr, &semaforosRenderizacaoConcluida[i]),
            "Falha ao criar semáforo de renderização concluída."
        );
        verificarResultadoVulkan(
            vkCreateFence(dispositivo->obterDispositivoLogico(), &infoCerca, nullptr, &cercasEmVoo[i]),
            "Falha ao criar cerca (VkFence) de sincronização de quadros."
        );
    }
}

void AplicacaoVulkan::atualizarCenaEParametros() {
    double tempoAtual = glfwGetTime();
    float deltaTempo = static_cast<float>(tempoAtual - ultimoTempo);
    ultimoTempo = tempoAtual;

    if (!rotacaoPausada) {
        anguloRotacao += velocidadeRotacao * deltaTempo;
    }

    VkExtent2D ext = swapchain->obterExtensao();

    CameraUBO cam{};
    cam.model = glm::rotate(glm::mat4(1.0f), anguloRotacao, glm::vec3(0.4f, 1.0f, 0.2f));
    cam.view = glm::lookAt(glm::vec3(0.0f, 1.5f, 3.6f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cam.proj = glm::perspective(glm::radians(45.0f), (float)ext.width / (float)ext.height, 0.1f, 100.0f);
    cam.proj[1][1] *= -1.0f; // Inverte coordenadas Y para o Vulkan

    parametrosBloom.texelSize = { 1.0f / (float)ext.width, 1.0f / (float)ext.height };

    pipelineBloom->atualizarUBOs(cam, parametrosBloom);
}

void AplicacaoVulkan::gravarBufferComando(VkCommandBuffer bufferComando, uint32_t indiceImagem) {
    VkCommandBufferBeginInfo infoInicio{};
    infoInicio.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    verificarResultadoVulkan(
        vkBeginCommandBuffer(bufferComando, &infoInicio),
        "Falha ao iniciar gravação do buffer de comando."
    );

    // Grava os 11 passes e 11 barreiras de memória do pipeline de Bloom
    pipelineBloom->gravarComandosCompletos(bufferComando, indiceImagem, *cenaGeometrica);

    verificarResultadoVulkan(
        vkEndCommandBuffer(bufferComando),
        "Falha ao finalizar gravação do buffer de comando."
    );
}

void AplicacaoVulkan::recriarSwapchainEFila() {
    dispositivo->esperarOcioso();
    swapchain->recriarSwapchain(*dispositivo, janela, superficie);
    pipelineBloom->recriarRecursosDimensionais(*dispositivo, *swapchain);
}

void AplicacaoVulkan::desenharQuadro() {
    VkDevice devLogico = dispositivo->obterDispositivoLogico();

    vkWaitForFences(devLogico, 1, &cercasEmVoo[quadroAtual], VK_TRUE, UINT64_MAX);

    uint32_t indiceImagem = 0;
    VkResult resultadoAquisicao = vkAcquireNextImageKHR(
        devLogico,
        swapchain->obterSwapchain(),
        UINT64_MAX,
        semaforosImagemDisponivel[quadroAtual],
        VK_NULL_HANDLE,
        &indiceImagem
    );

    if (resultadoAquisicao == VK_ERROR_OUT_OF_DATE_KHR) {
        recriarSwapchainEFila();
        return;
    } else if (resultadoAquisicao != VK_SUCCESS && resultadoAquisicao != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Falha ao adquirir imagem da swapchain.");
    }

    vkResetFences(devLogico, 1, &cercasEmVoo[quadroAtual]);

    atualizarCenaEParametros();

    vkResetCommandBuffer(buffersDeComando[quadroAtual], 0);
    gravarBufferComando(buffersDeComando[quadroAtual], indiceImagem);

    VkSubmitInfo infoSubmissao{};
    infoSubmissao.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore semaforosEspera[] = { semaforosImagemDisponivel[quadroAtual] };
    VkPipelineStageFlags estagiosEspera[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    infoSubmissao.waitSemaphoreCount = 1;
    infoSubmissao.pWaitSemaphores = semaforosEspera;
    infoSubmissao.pWaitDstStageMask = estagiosEspera;

    infoSubmissao.commandBufferCount = 1;
    infoSubmissao.pCommandBuffers = &buffersDeComando[quadroAtual];

    VkSemaphore semaforosSinalizacao[] = { semaforosRenderizacaoConcluida[quadroAtual] };
    infoSubmissao.signalSemaphoreCount = 1;
    infoSubmissao.pSignalSemaphores = semaforosSinalizacao;

    verificarResultadoVulkan(
        vkQueueSubmit(dispositivo->obterFilaGrafica(), 1, &infoSubmissao, cercasEmVoo[quadroAtual]),
        "Falha ao submeter o buffer de comandos à fila gráfica."
    );

    VkPresentInfoKHR infoApresentacao{};
    infoApresentacao.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    infoApresentacao.waitSemaphoreCount = 1;
    infoApresentacao.pWaitSemaphores = semaforosSinalizacao;

    VkSwapchainKHR swapchains[] = { swapchain->obterSwapchain() };
    infoApresentacao.swapchainCount = 1;
    infoApresentacao.pSwapchains = swapchains;
    infoApresentacao.pImageIndices = &indiceImagem;

    VkResult resultadoApresentacao = vkQueuePresentKHR(dispositivo->obterFilaApresentacao(), &infoApresentacao);

    if (resultadoApresentacao == VK_ERROR_OUT_OF_DATE_KHR || resultadoApresentacao == VK_SUBOPTIMAL_KHR || janela.foiRedimensionada()) {
        janela.redefinirSinalRedimensionamento();
        recriarSwapchainEFila();
    } else if (resultadoApresentacao != VK_SUCCESS) {
        throw std::runtime_error("Falha ao apresentar imagem da swapchain na tela.");
    }

    quadroAtual = (quadroAtual + 1) % LIMITE_QUADROS_EM_VOO;
}

void AplicacaoVulkan::salvarCapturaDeQuadro(const std::string& caminhoArquivoPpm, uint32_t indiceImagem) {
    dispositivo->esperarOcioso();

    VkExtent2D dimensao = swapchain->obterExtensao();
    VkDeviceSize tamanhoBytes = dimensao.width * dimensao.height * 4;

    VkBuffer bufferDestino = VK_NULL_HANDLE;
    VkDeviceMemory memoriaBuffer = VK_NULL_HANDLE;

    RecursosGraficos::criarBuffer(
        *dispositivo, tamanhoBytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferDestino, memoriaBuffer
    );

    VkCommandBufferAllocateInfo infoAlocCmd{};
    infoAlocCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    infoAlocCmd.commandPool = poolDeComandos;
    infoAlocCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    infoAlocCmd.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(dispositivo->obterDispositivoLogico(), &infoAlocCmd, &cmd);

    VkCommandBufferBeginInfo infoInicio{};
    infoInicio.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &infoInicio);

    VkImage imgSwap = swapchain->obterImagem(indiceImagem);

    // Transição de PRESENT_SRC_KHR para TRANSFER_SRC_OPTIMAL
    RecursosGraficos::transicionarLayoutImagemComando(
        cmd, imgSwap,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy regiao{};
    regiao.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    regiao.imageSubresource.mipLevel = 0;
    regiao.imageSubresource.baseArrayLayer = 0;
    regiao.imageSubresource.layerCount = 1;
    regiao.imageExtent = { dimensao.width, dimensao.height, 1 };

    vkCmdCopyImageToBuffer(cmd, imgSwap, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, bufferDestino, 1, &regiao);

    // Restaura para PRESENT_SRC_KHR
    RecursosGraficos::transicionarLayoutImagemComando(
        cmd, imgSwap,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    );

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submissao{};
    submissao.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submissao.commandBufferCount = 1;
    submissao.pCommandBuffers = &cmd;

    vkQueueSubmit(dispositivo->obterFilaGrafica(), 1, &submissao, VK_NULL_HANDLE);
    dispositivo->esperarOcioso();

    vkFreeCommandBuffers(dispositivo->obterDispositivoLogico(), poolDeComandos, 1, &cmd);

    void* dados = nullptr;
    vkMapMemory(dispositivo->obterDispositivoLogico(), memoriaBuffer, 0, tamanhoBytes, 0, &dados);
    const uint8_t* pixels = reinterpret_cast<const uint8_t*>(dados);

    std::ofstream arquivoPpm(caminhoArquivoPpm, std::ios::binary);
    arquivoPpm << "P6\n" << dimensao.width << " " << dimensao.height << "\n255\n";

    for (uint32_t y = 0; y < dimensao.height; y++) {
        for (uint32_t x = 0; x < dimensao.width; x++) {
            uint32_t idx = (y * dimensao.width + x) * 4;
            uint8_t b = pixels[idx + 0];
            uint8_t g = pixels[idx + 1];
            uint8_t r = pixels[idx + 2];
            arquivoPpm.put(static_cast<char>(r));
            arquivoPpm.put(static_cast<char>(g));
            arquivoPpm.put(static_cast<char>(b));
        }
    }
    arquivoPpm.close();
    vkUnmapMemory(dispositivo->obterDispositivoLogico(), memoriaBuffer);

    vkDestroyBuffer(dispositivo->obterDispositivoLogico(), bufferDestino, nullptr);
    vkFreeMemory(dispositivo->obterDispositivoLogico(), memoriaBuffer, nullptr);

    std::cout << "[OK] Captura de quadro salva em: " << caminhoArquivoPpm << std::endl;
}

void AplicacaoVulkan::executar(uint32_t limiteQuadros) {
    std::cout << "Entrando no Loop Principal de Renderização...\n" << std::endl;

    uint32_t quadrosProcessados = 0;
    while (!janela.deveFechar()) {
        janela.processarEventos();
        desenharQuadro();
        quadrosProcessados++;

        // No modo teste, grava capturas de tela com Bloom ativo e Bloom desativado para comprovação
        if (limiteQuadros > 0) {
            if (quadrosProcessados == 15) {
                salvarCapturaDeQuadro("captura_bloom_ativo.ppm", 0);
                parametrosBloom.intensity = 0.0f; // Desativa para testar sem Bloom
                std::cout << "[TESTE] Alternando para Bloom DESATIVADO para captura comparativa..." << std::endl;
            } else if (quadrosProcessados == 25) {
                salvarCapturaDeQuadro("captura_bloom_desativado.ppm", 0);
                parametrosBloom.intensity = intensidadePadrao; // Reativa
                std::cout << "[TESTE] Reativando Bloom..." << std::endl;
            }
        }

        if (limiteQuadros > 0 && quadrosProcessados >= limiteQuadros) {
            std::cout << "Limite de " << limiteQuadros << " quadros atingido no modo teste. Finalizando..." << std::endl;
            break;
        }
    }

    dispositivo->esperarOcioso();
    std::cout << "\nLoop de renderização encerrado. Limpando recursos..." << std::endl;
}

void AplicacaoVulkan::limparRecursos() {
    if (dispositivo) {
        dispositivo->esperarOcioso();
        VkDevice devLogico = dispositivo->obterDispositivoLogico();

        for (int i = 0; i < LIMITE_QUADROS_EM_VOO; i++) {
            if (i < static_cast<int>(semaforosImagemDisponivel.size()) && semaforosImagemDisponivel[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(devLogico, semaforosImagemDisponivel[i], nullptr);
            }
            if (i < static_cast<int>(semaforosRenderizacaoConcluida.size()) && semaforosRenderizacaoConcluida[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(devLogico, semaforosRenderizacaoConcluida[i], nullptr);
            }
            if (i < static_cast<int>(cercasEmVoo.size()) && cercasEmVoo[i] != VK_NULL_HANDLE) {
                vkDestroyFence(devLogico, cercasEmVoo[i], nullptr);
            }
        }

        if (poolDeComandos != VK_NULL_HANDLE) {
            vkDestroyCommandPool(devLogico, poolDeComandos, nullptr);
            poolDeComandos = VK_NULL_HANDLE;
        }

        pipelineBloom.reset();
        cenaGeometrica.reset();
        carregadorShader.reset();
        swapchain.reset();
        dispositivo.reset();
    }

    if (superficie != VK_NULL_HANDLE && instancia) {
        vkDestroySurfaceKHR(instancia->obterInstancia(), superficie, nullptr);
        superficie = VK_NULL_HANDLE;
    }

    instancia.reset();
    std::cout << "[OK] Todos os recursos do Vulkan foram liberados corretamente!" << std::endl;
}
