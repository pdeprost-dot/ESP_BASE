#include "WebRequest.h"

bool WebRequest::hasArg(const char* name) const {
  return name && hasArgReader_ && hasArgReader_(context_, name);
}

String WebRequest::arg(const char* name) const {
  return name && argReader_ ? argReader_(context_, name) : String();
}
