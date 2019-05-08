#include "main/content-type.h"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

namespace {

constexpr char kOctetStream[] = "application/octet-stream";

// TODO(bcf): This is just a subset.
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Complete_list_of_MIME_types
const absl::flat_hash_map<absl::string_view, absl::string_view>&
ExtensionToContetType() {
  static auto* map =
      new absl::flat_hash_map<absl::string_view, absl::string_view>({
          {"aac", "audio/aac"},
          {"abw", "application/x-abiword"},
          {"arc", "application/x-freearc"},
          {"avi", "video/x-msvideo"},
          {"azw", "application/vnd.amazon.ebook"},
          {"bin", "application/octet-stream"},
          {"bmp", "image/bmp"},
          {"bz", "application/x-bzip"},
          {"bz2", "application/x-bzip2"},
          {"csh", "application/x-csh"},
          {"css", "text/css"},
          {"csv", "text/csv"},
          {"doc", "application/msword"},
          {"docx",
           "application/"
           "vnd.openxmlformats-officedocument.wordprocessingml.document"},
          {"eot", "application/vnd.ms-fontobject"},
          {"epub", "application/epub+zip"},
          {"gif", "image/gif"},
          {"html", "text/html"},
          {"ico", "image/vnd.microsoft.icon"},
          {"ics", "text/calendar"},
          {"jar", "application/java-archive"},
          {"jpeg", "image/jpeg"},
          {"js", "text/javascript"},
          {"json", "application/json"},
          {"jsonld", "application/ld+json"},
          {"mid", "audio/midi"},
          {"mjs", "text/javascript"},
          {"mp3", "audio/mpeg"},
          {"mpeg", "video/mpeg"},
          {"mpkg", "application/vnd.apple.installer+xml"},
          {"odp", "application/vnd.oasis.opendocument.presentation"},
          {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
          {"odt", "application/vnd.oasis.opendocument.text"},
          {"oga", "audio/ogg"},
          {"ogv", "video/ogg"},
          {"ogx", "application/ogg"},
          {"otf", "font/otf"},
          {"png", "image/png"},
          {"pdf", "application/pdf"},
          {"ppt", "application/vnd.ms-powerpoint"},
          {"pptx",
           "application/"
           "vnd.openxmlformats-officedocument.presentationml.presentation"},
          {"rar", "application/x-rar-compressed"},
          {"rtf", "application/rtf"},
          {"sh", "application/x-sh"},
          {"svg", "image/svg+xml"},
          {"swf", "application/x-shockwave-flash"},
          {"tar", "application/x-tar"},
          {"tiff", "image/tiff"},
          {"ttf", "font/ttf"},
          {"txt", "text/plain"},
          {"vsd", "application/vnd.visio"},
          {"wav", "audio/wav"},
          {"weba", "audio/webm"},
          {"webm", "video/webm"},
          {"webp", "image/webp"},
          {"woff", "font/woff"},
          {"woff2", "font/woff2"},
          {"xhtml", "application/xhtml+xml"},
          {"xls", "application/vnd.ms-excel"},
          {"xlsx",
           "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
          {"xml", "application/xml "},
          {"xul", "application/vnd.mozilla.xul+xml"},
          {"zip", "application/zip"},
          {"3gp", "video/3gpp"},
          {"3g2", "video/3gpp2"},
          {"7z", "application/x-7z-compressed"},
      });

  return *map;
}

const absl::flat_hash_set<absl::string_view>& ContentTypesToCompress() {
  static auto* set = new absl::flat_hash_set<absl::string_view>({
      {"text/html"},
      {"text/css"},
      {"text/javascript"},
      {"text/xml"},
      {"text/plain"},
      {"text/x-component"},
      {"application/javascript"},
      {"application/x-javascript"},
      {"application/json"},
      {"application/xml"},
      {"application/rss+xml"},
      {"application/atom+xml"},
      {"font/truetype"},
      {"font/opentype"},
      {"application/vnd.ms-fontobject"},
      {"image/svg+xml"},
  });

  return *set;
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

// static
bool ContentType::ShouldCompress(absl::string_view content_type) {
  return ContentTypesToCompress().count(content_type) > 0;
}
