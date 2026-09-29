// composite.frag - fragment shader do passe 10 (cena + bloom + tone mapping)
// entrada: sceneHDR, bloomMip[0] e UBO BloomParams / saida: imagem do swapchain
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform BloomParams {
    float threshold;
    float knee;
    float intensity;   // alfa da eq. 5
    vec2 texelSize;
} params;

layout(set = 0, binding = 1) uniform sampler2D sceneHDR;   // 1920x1080
layout(set = 0, binding = 2) uniform sampler2D bloomTex;   // bloomMip[0], 960x540

layout(location = 0) out vec4 outColor;   // swapchain (B8G8R8A8_SRGB)

void main() {
    vec3 cena = texture(sceneHDR, inUV).rgb;
    vec3 bloom = texture(bloomTex, inUV).rgb;   // o filtro linear amplia o halo

    vec3 total = cena + params.intensity * bloom;   // C_total da eq. 5

    vec3 cor = total / (1.0 + total);   // Reinhard, fica entre 0 e 1

    outColor = vec4(clamp(cor, 0.0, 1.0), 1.0);   // gama e feita pelo formato _SRGB
}
