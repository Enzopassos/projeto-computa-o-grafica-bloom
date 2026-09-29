// scene.vert - vertex shader do passe 0 (cena)
// entrada: vertex buffer + UBO da camera / saida: gl_Position, normal e uv
#version 450

layout(location = 0) in vec3 inPosition;   // posicao do vertice
layout(location = 1) in vec3 inNormal;     // normal do vertice
layout(location = 2) in vec2 inTexCoord;   // coordenada de textura

layout(set = 0, binding = 0) uniform CameraUBO {   // std140, 192 bytes
    mat4 model;   // objeto -> mundo
    mat4 view;    // mundo -> camera
    mat4 proj;    // camera -> tela
} ubo;

layout(location = 0) out vec3 fragNormal;    // vai para o scene.frag
layout(location = 1) out vec2 fragTexCoord;

void main() {
    // posicao na tela (o hardware divide por w e usa z como profundidade)
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // normal so gira (parte 3x3 da model) e volta a ter tamanho 1
    fragNormal = normalize(mat3(ubo.model) * inNormal);

    fragTexCoord = inTexCoord;   // passa direto
}
