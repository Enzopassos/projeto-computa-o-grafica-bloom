// downsample.frag - fragment shader dos passes 2 a 5
// entrada: bloomMip[i-1] + push constants / saida: bloomMip[i] (metade do tamanho)
// filtro de 13 amostras (LearnOpenGL), faz o papel da gaussiana (eq. 4)
#version 450

layout(location = 0) in vec2 inUV;

layout(push_constant) uniform PushConstants {
    vec2 texelSize;   // 1 pixel da imagem lida
    int mipLevel;     // nivel escrito (1 a 4)
} pc;

layout(set = 0, binding = 0) uniform sampler2D srcTexture;   // linear, clamp to edge

layout(location = 0) out vec4 outColor;

void main() {
    float x = pc.texelSize.x;
    float y = pc.texelSize.y;

    // 13 amostras:   a - b - c
    //                - j - k -
    //                d - e - f    (e = pixel atual)
    //                - l - m -
    //                g - h - i
    vec3 a = texture(srcTexture, vec2(inUV.x - 2*x, inUV.y + 2*y)).rgb;
    vec3 b = texture(srcTexture, vec2(inUV.x,       inUV.y + 2*y)).rgb;
    vec3 c = texture(srcTexture, vec2(inUV.x + 2*x, inUV.y + 2*y)).rgb;

    vec3 d = texture(srcTexture, vec2(inUV.x - 2*x, inUV.y)).rgb;
    vec3 e = texture(srcTexture, vec2(inUV.x,       inUV.y)).rgb;
    vec3 f = texture(srcTexture, vec2(inUV.x + 2*x, inUV.y)).rgb;

    vec3 g = texture(srcTexture, vec2(inUV.x - 2*x, inUV.y - 2*y)).rgb;
    vec3 h = texture(srcTexture, vec2(inUV.x,       inUV.y - 2*y)).rgb;
    vec3 i = texture(srcTexture, vec2(inUV.x + 2*x, inUV.y - 2*y)).rgb;

    vec3 j = texture(srcTexture, vec2(inUV.x - x, inUV.y + y)).rgb;
    vec3 k = texture(srcTexture, vec2(inUV.x + x, inUV.y + y)).rgb;
    vec3 l = texture(srcTexture, vec2(inUV.x - x, inUV.y - y)).rgb;
    vec3 m = texture(srcTexture, vec2(inUV.x + x, inUV.y - y)).rgb;

    // media ponderada, os pesos somam 1
    vec3 cor = e * 0.125;
    cor += (a + c + g + i) * 0.03125;
    cor += (b + d + f + h) * 0.0625;
    cor += (j + k + l + m) * 0.125;

    // so no primeiro nivel: corta pixels muito fortes (ficam piscando)
    if (pc.mipLevel == 1) {
        cor = min(cor, vec3(10.0));
    }

    outColor = vec4(cor, 1.0);
}
