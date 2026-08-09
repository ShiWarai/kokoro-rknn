#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace kokoro::paths {

inline std::filesystem::path exeDir() {
  return std::filesystem::canonical("/proc/self/exe").parent_path();
}

// Locate repo root by looking for models/base/config.json near the binary or CWD.
inline std::filesystem::path projectRoot() {
  std::vector<std::filesystem::path> candidates = {
      exeDir().parent_path(),
      exeDir(),
      std::filesystem::current_path(),
      std::filesystem::current_path().parent_path(),
  };
  for (const auto& root : candidates) {
    if (std::filesystem::exists(root / "models" / "base" / "config.json"))
      return std::filesystem::absolute(root);
  }
  return std::filesystem::absolute(exeDir().parent_path());
}

inline std::string resolve(const std::filesystem::path& root,
                           const std::string& path) {
  std::filesystem::path p(path);
  if (p.is_absolute() || std::filesystem::exists(p))
    return std::filesystem::absolute(p).string();
  return std::filesystem::absolute(root / p).string();
}

inline std::string defaultModelPath(const char* rel) {
  return resolve(projectRoot(), rel);
}

} // namespace kokoro::paths
