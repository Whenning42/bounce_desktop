#include <memory>

#include "third_party/status/status_or.h"

class Backend {
 public:
  virtual int port() = 0;
  virtual ~Backend() = default;
};
