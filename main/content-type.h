#ifndef MAIN_CONTENT_TYPE_H_
#define MAIN_CONTENT_TYPE_H_

#include "absl/strings/string_view.h"

class ContentType {
 public:
  static absl::string_view ForFilename(absl::string_view filename);
};

#endif  // MAIN_CONTENT_TYPE_H_
