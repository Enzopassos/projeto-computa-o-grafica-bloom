# Código-Fonte do Pipeline de Bloom em Vulkan (C++20)

Esta pasta contém todo o código-fonte, shaders GLSL, scripts de compilação e dependências para compilar e executar o protótipo.

### Como Compilar e Rodar

```bash
# 1. Configurar e Compilar com CMake
cmake -B build
cmake --build build

# 2. Executar
./build/BloomVulkan
```

### Controles no Teclado
- `[ESPAÇO]`: Alterna Bloom ON/OFF
- `[P]`: Pausa / Retoma rotação
- `[+]` / `[-]`: Ajusta intensidade do brilho
- `[ESC]`: Fecha a aplicação
