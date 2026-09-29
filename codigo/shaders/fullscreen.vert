// fullscreen.vert - vertex shader dos passes 1 a 10
// sem vertex buffer: 3 vertices fixos, escolhidos pelo gl_VertexIndex
#version 450

layout(location = 0) out vec2 outUV;   // uv para o fragment shader

// triangulo maior que a tela, o hardware corta o que sobra
vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);   // z = 0, nao tem teste de profundidade
    outUV = pos * 0.5 + 0.5;             // de [-1,1] para [0,1], sem inverter o y
}
