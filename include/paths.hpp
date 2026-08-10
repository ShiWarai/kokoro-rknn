#pragma once

#include <filesystem>
#include <string>

namespace kokoro::paths {

// Model repo root passed via --models-dir or KOKORO_MODELS_DIR.
// Pack layout: <root>/<pack>/ or <root>/packs/<pack>/ (HF clone root).
void setModelsDir(std::filesystem::path dir);
std::filesystem::path modelsDir();

std::filesystem::path packDir(const char* pack);
std::string packFile(const char* pack, const char* rel);

std::filesystem::path exeDir();
std::filesystem::path projectRoot();

// Absolute path, or relative to CWD.
std::string resolveUserPath(const std::string& path);

} // namespace kokoro::paths
