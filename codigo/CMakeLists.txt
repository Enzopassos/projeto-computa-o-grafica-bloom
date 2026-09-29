cmake_minimum_required(VERSION 3.20)
project(BloomVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Otimizações e avisos de compilação
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Caso os headers do Vulkan estejam no repositório local em third_party
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan-headers/include/vulkan/vulkan.h")
    set(Vulkan_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan-headers/include" CACHE PATH "Vulkan Include Dir" FORCE)
endif()

# Encontrar pacotes externos obrigatórios
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# GLM (cabeçalho matemático)
find_path(GLM_INCLUDE_DIR "glm/glm.hpp")
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "Biblioteca GLM não foi encontrada no sistema (/usr/include/glm).")
endif()

# Executa script de compilação dos shaders GLSL para SPIR-V antes do build
add_custom_target(compilar_shaders ALL
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/compilar_shaders.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compilando shaders GLSL para SPIR-V (.spv)..."
)

# Arquivos fonte C++ organizados de forma modular
set(FONTES_CPP
    src/main.cpp
    src/Janela.cpp
    src/DispositivoVulkan.cpp
    src/GerenciadorSwapchain.cpp
    src/CarregadorShader.cpp
    src/PipelineGrafico.cpp
    src/RecursosGraficos.cpp
    src/CenaGeometrica.cpp
    src/PipelineBloom.cpp
    src/AplicacaoVulkan.cpp
)

add_executable(BloomVulkan ${FONTES_CPP})

add_dependencies(BloomVulkan compilar_shaders)

target_include_directories(BloomVulkan PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
)

target_link_libraries(BloomVulkan PRIVATE
    Vulkan::Vulkan
    glfw
)

message(STATUS "Configuração CMake concluída para o projeto BloomVulkan.")
