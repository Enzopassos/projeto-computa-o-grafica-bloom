#include "CarregadorShader.hpp"

CarregadorShader::CarregadorShader(VkDevice dispositivoLogico)
    : dispositivoLogicoReferencia(dispositivoLogico) {
}

CarregadorShader::~CarregadorShader() {
    for (auto const& [nome, modulo] : modulosCarregados) {
        if (modulo != VK_NULL_HANDLE) {
            vkDestroyShaderModule(dispositivoLogicoReferencia, modulo, nullptr);
        }
    }
    modulosCarregados.clear();
}

std::vector<char> CarregadorShader::lerArquivoBinario(const std::string& caminhoArquivo) {
    std::ifstream arquivo(caminhoArquivo, std::ios::ate | std::ios::binary);

    if (!arquivo.is_open()) {
        throw std::runtime_error("Falha ao abrir arquivo de shader SPIR-V: " + caminhoArquivo);
    }

    size_t tamanhoArquivo = static_cast<size_t>(arquivo.tellg());
    std::vector<char> buffer(tamanhoArquivo);

    arquivo.seekg(0);
    arquivo.read(buffer.data(), tamanhoArquivo);
    arquivo.close();

    return buffer;
}

VkShaderModule CarregadorShader::carregarModuloShader(const std::string& caminhoArquivo) {
    auto codigoSpv = lerArquivoBinario(caminhoArquivo);

    VkShaderModuleCreateInfo infoCriacao{};
    infoCriacao.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    infoCriacao.codeSize = codigoSpv.size();
    infoCriacao.pCode = reinterpret_cast<const uint32_t*>(codigoSpv.data());

    VkShaderModule moduloShader = VK_NULL_HANDLE;
    verificarResultadoVulkan(
        vkCreateShaderModule(dispositivoLogicoReferencia, &infoCriacao, nullptr, &moduloShader),
        "Falha ao criar o VkShaderModule para: " + caminhoArquivo
    );

    return moduloShader;
}

void CarregadorShader::carregarTodosShadersDoProjeto(const std::string& diretorioSpv) {
    std::string dir = diretorioSpv;

    // Detecta automaticamente se o programa foi executado da raiz ou de dentro de codigo/
    std::ifstream teste(dir + "/fullscreen.vert.spv");
    if (!teste.is_open()) {
        std::ifstream testeAlt("codigo/" + dir + "/fullscreen.vert.spv");
        if (testeAlt.is_open()) {
            dir = "codigo/" + dir;
        }
    }

    std::cout << "\n========================================================" << std::endl;
    std::cout << "Integrando e Carregando Shaders SPIR-V da Entrega 3..." << std::endl;
    std::cout << "Diretório de Shaders: " << dir << std::endl;
    std::cout << "========================================================" << std::endl;

    const std::vector<std::string> listaShaders = {
        "fullscreen.vert.spv",
        "scene.vert.spv",
        "scene.frag.spv",
        "bright.frag.spv",
        "downsample.frag.spv",
        "upsample.frag.spv",
        "composite.frag.spv",
        "teste.frag.spv"
    };

    for (const auto& nomeArquivo : listaShaders) {
        std::string caminhoCompleto = dir + "/" + nomeArquivo;
        try {
            VkShaderModule modulo = carregarModuloShader(caminhoCompleto);
            modulosCarregados[nomeArquivo] = modulo;
            std::cout << "[OK] Shader carregado com sucesso: " << nomeArquivo << std::endl;
        } catch (const std::exception& erro) {
            std::cerr << "[FALHA] Erro ao carregar " << nomeArquivo << ": " << erro.what() << std::endl;
            throw;
        }
    }
    std::cout << "Total de shaders integrados com sucesso: " << modulosCarregados.size() << std::endl;
    std::cout << "========================================================\n" << std::endl;
}

VkShaderModule CarregadorShader::obterModulo(const std::string& identificador) const {
    auto iterador = modulosCarregados.find(identificador);
    if (iterador == modulosCarregados.end()) {
        throw std::runtime_error("Shader solicitado não encontrado no carregador: " + identificador);
    }
    return iterador->second;
}

void CarregadorShader::liberarModulo(VkShaderModule modulo) {
    if (modulo != VK_NULL_HANDLE) {
        vkDestroyShaderModule(dispositivoLogicoReferencia, modulo, nullptr);
    }
}
