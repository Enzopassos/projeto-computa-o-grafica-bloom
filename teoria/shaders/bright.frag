// bright.frag - fragment shader do passe 1 (bright pass)
// entrada: sceneHDR + UBO BloomParams / saida: bloomMip[0] (960x540)
#version 450

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform BloomParams {   // std140
    float threshold;   // T da eq. 2
    float knee;        // k da eq. 2
    float intensity;   // usado so no composite
    vec2 texelSize;    // (1/1920, 1/1080)
} params;

layout(set = 0, binding = 1) uniform sampler2D sceneHDR;   // linear, clamp to edge

layout(location = 0) out vec4 outColor;

const float EPS = 0.0001;   // epsilon da eq. 2

void main() {
    // cada pixel daqui = bloco 2x2 da cena, entao faz a media dos 4
    vec2 offsets[4] = vec2[](vec2(-0.5, -0.5), vec2(0.5, -0.5),
                             vec2(-0.5,  0.5), vec2(0.5,  0.5));
    vec3 cor = vec3(0.0);
    for (int i = 0; i < 4; i++) {   // 4 vezes, uma por pixel do bloco
        cor += texture(sceneHDR, inUV + offsets[i] * params.texelSize).rgb;
    }
    cor = cor / 4.0;
    cor = max(cor, vec3(0.0));   // nao existe luz negativa

    // luminancia (eq. 1)
    float lum = dot(cor, vec3(0.2126, 0.7152, 0.0722));

    // soft knee (eq. 2): peso vai de 0 (em T - k) ate k (em T + k)
    float T = params.threshold;
    float k = params.knee;
    float x = clamp(lum - T + k, 0.0, 2.0 * k);
    float peso = (x * x) / (4.0 * k + EPS);

    outColor = vec4(cor * peso, 1.0);   // so o que e claro sobra
}
