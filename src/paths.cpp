#include "paths.hpp"

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

namespace kokoro::paths {
namespace {

std::optional<std::filesystem::path> g_modelsDir;

std::filesystem::path findPackDir(const std::filesystem::path& root,
                                  const char* pack) {
  const std::filesystem::path candidates[] = {
      root / pack,
      root / "packs" / pack,
  };
  for (const auto& dir : candidates) {
    if (std::filesystem::exists(dir / "config.json"))
      return dir;
  }
  throw std::runtime_error(
      "Model pack '" + std::string(pack) + "' not found under " + root.string() +
      " (tried " + pack + "/ and packs/" + pack + "/)");
}

} // namespace

void setModelsDir(std::filesystem::path dir) {
  g_modelsDir = std::filesystem::absolute(std::move(dir));
}

std::filesystem::path modelsDir() {
  if (g_modelsDir) return *g_modelsDir;
  if (const char* dir = std::getenv("KOKORO_MODELS_DIR")) {
    if (*dir) return std::filesystem::absolute(dir);
  }
  throw std::runtime_error(
      "models path not set; pass --models-dir or set KOKORO_MODELS_DIR");
}

std::filesystem::path packDir(const char* pack) {
  return findPackDir(modelsDir(), pack);
}

std::string packFile(const char* pack, const char* rel) {
  return (packDir(pack) / rel).string();
}

std::filesystem::path exeDir() {
  return std::filesystem::canonical("/proc/self/exe").parent_path();
}

std::filesystem::path projectRoot() {
  const std::filesystem::path candidates[] = {
      exeDir().parent_path(),
      exeDir(),
      std::filesystem::current_path(),
      std::filesystem::current_path().parent_path(),
  };
  for (const auto& root : candidates) {
    if (std::filesystem::exists(root / "server" / "web-content") ||
        std::filesystem::exists(root / "CMakeLists.txt"))
      return std::filesystem::absolute(root);
  }
  return std::filesystem::absolute(exeDir().parent_path());
}

std::string resolveUserPath(const std::string& path) {
  std::filesystem::path p(path);
  if (p.is_absolute() || std::filesystem::exists(p))
    return std::filesystem::absolute(p).string();
  return std::filesystem::absolute(std::filesystem::current_path() / p).string();
}

} // namespace kokoro::paths
