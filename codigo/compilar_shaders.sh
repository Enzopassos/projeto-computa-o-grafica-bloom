#!/usr/bin/env bash
# ==============================================================================
# Script de Compilação de Shaders GLSL para SPIR-V
# Projeto: Bloom e Efeitos de Pós-Processamento em Tempo Real
# ==============================================================================

set -e

DIRETORIO_SHADERS="$(cd "$(dirname "${BASH_SOURCE[0]}")/shaders" && pwd)"
DIRETORIO_SAIDA="${DIRETORIO_SHADERS}/spv"

mkdir -p "${DIRETORIO_SAIDA}"

echo "========================================================"
echo "Compilando shaders GLSL para SPIR-V usando glslc..."
echo "Diretório de entrada: ${DIRETORIO_SHADERS}"
echo "Diretório de saída:   ${DIRETORIO_SAIDA}"
echo "========================================================"

TOTAL=0
SUCESSO=0

for ARQUIVO_SHADER in "${DIRETORIO_SHADERS}"/*.vert "${DIRETORIO_SHADERS}"/*.frag; do
    if [ -f "${ARQUIVO_SHADER}" ]; then
        NOME_BASE="$(basename "${ARQUIVO_SHADER}")"
        ARQUIVO_SPV="${DIRETORIO_SAIDA}/${NOME_BASE}.spv"
        
        echo -n "Compilando ${NOME_BASE} -> ${NOME_BASE}.spv ... "
        glslc "${ARQUIVO_SHADER}" -o "${ARQUIVO_SPV}"
        echo "OK"
        
        SUCESSO=$((SUCESSO + 1))
        TOTAL=$((TOTAL + 1))
    fi
done

echo "========================================================"
echo "Sucesso: ${SUCESSO} de ${TOTAL} shaders compilados para SPIR-V!"
echo "========================================================"
