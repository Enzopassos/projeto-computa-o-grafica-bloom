// scene.frag - fragment shader do passe 0 (cena)
// entrada: normal, uv e textura do material / saida: cor HDR na sceneHDR
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

// textura do material: sampler trilinear (com mipmaps), REPEAT.
// o canal alfa marca o que emite luz (1 = emite)
layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;   // sceneHDR, pode passar de 1.0

const vec3 luzDir = normalize(vec3(0.3, 1.0, 0.5));   // direcao para a luz
const vec3 luzCor = vec3(4.0, 3.8, 3.5);              // luz forte, em HDR
const float ambiente = 0.05;
const float emissivo = 10.0;

void main() {
    vec4 texel = texture(texSampler, fragTexCoord);   // nivel do mipmap automatico
    vec3 albedo = texel.rgb;

    vec3 n = normalize(fragNormal);   // a interpolacao muda o tamanho

    // difusa de Lambert; max evita valor negativo nas faces de costas
    float difusa = max(dot(n, luzDir), 0.0);

    vec3 cor = albedo * ambiente + albedo * luzCor * difusa;
    cor += albedo * texel.a * emissivo;   // partes que emitem luz

    outColor = vec4(cor, 1.0);   // sem clamp, e HDR
}
