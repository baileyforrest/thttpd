#include "main/content-type.h"

#include "absl/container/flat_hash_map.h"

namespace {

constexpr char kOctetStream[] = "application/octet-stream";

// TODO(bcf): This is just a subset.
const absl::flat_hash_map<absl::string_view, absl::string_view>&
ExtensionToContetType() {
  static auto* map =
      new absl::flat_hash_map<absl::string_view, absl::string_view>({
          {"txt", "text/plain"},
          {"css", "text/css"},
          {"html", "text/html"},
          {"js", "text/javascript"},
          {"gif", "image/gif"},
          {"jpg", "image/jpeg"},
          {"jpeg", "image/jpeg"},
          {"png", "image/png"},
          {"png", "image/png"},
          {"webp", "image/webp"},
          {"svg", "image/svg+xml"},
          {"ico", "image/x-icon"},
          {"wav", "audio/wav"},
      });

  return *map;
}

}  // namespace

// static
absl::string_view ContentType::ForFilename(absl::string_view filename) {
  if (filename.empty()) {
    return kOctetStream;
  }
  size_t path_sep = filename.rfind("/");
  if (path_sep == filename.size() - 1) {
    return kOctetStream;
  }
  absl::string_view basename =
      path_sep == filename.npos ? filename : filename.substr(path_sep + 1);

  size_t extension_sep = basename.rfind(".");
  if (extension_sep == basename.npos || extension_sep == basename.size() - 1) {
    return kOctetStream;
  }

  absl::string_view extension = basename.substr(extension_sep + 1);

  const auto& ext_to_type = ExtensionToContetType();
  auto it = ext_to_type.find(extension);
  if (it != ext_to_type.end()) {
    return it->second;
  }
  return kOctetStream;
}
