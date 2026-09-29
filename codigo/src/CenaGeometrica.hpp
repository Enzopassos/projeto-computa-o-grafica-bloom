#pragma once

#include "Comum.hpp"
#include "DispositivoVulkan.hpp"
#include "RecursosGraficos.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Vértice da cena 3D correspondente estritamente ao scene.vert da Entrega 3
struct VerticeCena {
    glm::vec3 posicao;
    glm::vec3 normal;
    glm::vec2 uv;

    static VkVertexInputBindingDescription obterDescricaoVinculacao();
    static std::vector<VkVertexInputAttributeDescription> obterDescricaoAtributos();
};

// UBO da câmera (std140, 192 bytes) correspondente ao scene.vert
struct CameraUBO {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

// UBO do Bloom (std140, 32 bytes) correspondente ao bright.frag e composite.frag
struct ParametrosBloomUBO {
    alignas(4) float threshold = 1.0f;  // Limiar de corte
    alignas(4) float knee = 0.5f;       // Joelho suave
    alignas(4) float intensity = 1.2f;  // Intensidade do efeito de Bloom
    alignas(4) float espacamento = 0.0f;
    alignas(8) glm::vec2 texelSize{1.0f / 1280.0f, 1.0f / 720.0f};
    alignas(8) glm::vec2 espacamentoFinal{0.0f, 0.0f};
};

// Push Constants para downsample.frag e upsample.frag (12 bytes)
struct PushConstantsBloom {
    glm::vec2 texelSize;
    int32_t mipLevel;
};

// Classe responsável por gerenciar a geometria 3D, vértices e índices da cena
class CenaGeometrica {
public:
    CenaGeometrica(const DispositivoVulkan& dispositivo, VkCommandPool poolComandos, VkQueue fila);
    ~CenaGeometrica();

    CenaGeometrica(const CenaGeometrica&) = delete;
    CenaGeometrica& operator=(const CenaGeometrica&) = delete;

    void desenhar(VkCommandBuffer bufferComando) const;

private:
    void criarGeometriaCubo(const DispositivoVulkan& dispositivo, VkCommandPool poolComandos, VkQueue fila);

    VkDevice dispositivoLogicoReferencia = VK_NULL_HANDLE;
    BufferVulkan bufferVertices;
    BufferVulkan bufferIndices;
    uint32_t totalIndices = 0;
};
