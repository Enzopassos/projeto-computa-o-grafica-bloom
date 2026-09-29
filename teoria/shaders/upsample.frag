// upsample.frag - fragment shader dos passes 6 a 9
// entrada: bloomMip[i+1] + push constants / saida: bloomMip[i]
// a soma com o que ja esta em bloomMip[i] e feita pelo blend aditivo (ONE + ONE)
#version 450

layout(location = 0) in vec2 inUV;

layout(push_constant) uniform PushConstants {
    vec2 texelSize;   // 1 pixel da imagem lida (a menor)
    int mipLevel;     // nivel que recebe a soma (3 a 0), so para debug
} pc;

layout(set = 0, binding = 0) uniform sampler2D srcTexture;   // linear, clamp to edge

layout(location = 0) out vec4 outColor;

float pesos[3] = float[](1.0, 2.0, 1.0);   // filtro tenda, /16 no total

void main() {
    vec3 cor = vec3(0.0);

    // 3x3 = 9 amostras, deslocadas de 1 pixel da imagem menor
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float peso = pesos[x + 1] * pesos[y + 1] / 16.0;
            vec2 uv = inUV + vec2(x, y) * pc.texelSize;
            cor += texture(srcTexture, uv).rgb * peso;
        }
    }

    outColor = vec4(cor, 1.0);   // o blend soma no bloomMip[i]
}
