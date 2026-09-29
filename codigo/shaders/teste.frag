#version 450
// teste.frag - Shader de fragmento de teste para validação da Entrega 4
// Entrada: coordenada UV vinda do fullscreen.vert
// Saída: imagem renderizada na tela com gradiente e núcleo luminoso central

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    // Vetor deslocado para o centro da tela (origem em 0,0)
    vec2 deslocamento = inUV - vec2(0.5);
    float distanciaAoCentro = length(deslocamento);

    // Gradiente de fundo suave
    vec3 corFundo = mix(vec3(0.05, 0.07, 0.12), vec3(0.02, 0.03, 0.05), inUV.y);

    // Fonte emissiva central (núcleo incandescente de alta energia)
    // Esse padrão antecipa visualmente o comportamento da fonte emissiva do Bloom
    float intensidadeEmissiva = 0.035 / (distanciaAoCentro * distanciaAoCentro + 0.015);
    vec3 corEmissiva = vec3(1.0, 0.7, 0.3) * intensidadeEmissiva;

    // Disco delimitador central com suavização de borda
    float mascaraCirculo = smoothstep(0.12, 0.08, distanciaAoCentro);
    vec3 corNucleo = vec3(1.0, 0.95, 0.85) * mascaraCirculo * 2.0;

    vec3 corFinal = corFundo + corEmissiva + corNucleo;

    outColor = vec4(corFinal, 1.0);
}
