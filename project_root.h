#include <filesystem>
#include <string>

inline std::string project_root() {
  std::filesystem::path path = std::filesystem::current_path();
  return path.string();
}
