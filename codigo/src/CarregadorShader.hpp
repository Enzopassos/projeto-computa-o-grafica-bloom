#pragma once

#include "Comum.hpp"
#include <map>

// Classe responsável pelo carregamento dos arquivos binários SPIR-V e instanciação dos VkShaderModule
class CarregadorShader {
public:
    explicit CarregadorShader(VkDevice dispositivoLogico);
    ~CarregadorShader();

    CarregadorShader(const CarregadorShader&) = delete;
    CarregadorShader& operator=(const CarregadorShader&) = delete;

    // Carrega um arquivo SPIR-V (.spv) e cria o respectivo VkShaderModule
    VkShaderModule carregarModuloShader(const std::string& caminhoArquivo);

    // Carrega e valida todos os shaders do projeto desenvolvidos nas entregas anteriores
    void carregarTodosShadersDoProjeto(const std::string& diretorioSpv = "shaders/spv");

    VkShaderModule obterModulo(const std::string& identificador) const;

    void liberarModulo(VkShaderModule modulo);

private:
    static std::vector<char> lerArquivoBinario(const std::string& caminhoArquivo);

    VkDevice dispositivoLogicoReferencia = VK_NULL_HANDLE;
    std::map<std::string, VkShaderModule> modulosCarregados;
};
