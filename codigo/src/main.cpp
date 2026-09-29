#include "AplicacaoVulkan.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout << "========================================================" << std::endl;
    std::cout << "Projeto: Bloom e Efeitos de Pós-Processamento em Tempo Real" << std::endl;
    std::cout << "Entrega 4 - Integração do Código e Boilerplate Vulkan" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    uint32_t limiteQuadros = 0;
    for (int i = 1; i < argc; i++) {
        std::string argumento = argv[i];
        if (argumento == "--teste") {
            limiteQuadros = 60; // 60 quadros para teste automatizado
        } else if ((argumento == "--quadros" || argumento == "-q") && i + 1 < argc) {
            limiteQuadros = static_cast<uint32_t>(std::atoi(argv[++i]));
        }
    }

    try {
        AplicacaoVulkan aplicacao(1280, 720, "Vulkan - Efeito Bloom e Pós-Processamento (Entrega 4)");
        aplicacao.executar(limiteQuadros);
    } catch (const std::exception& erro) {
        std::cerr << "\n[ERRO CRÍTICO]: " << erro.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nAplicação encerrada com sucesso." << std::endl;
    return EXIT_SUCCESS;
}
