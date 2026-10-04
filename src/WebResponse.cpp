#include "WebResponse.h"

void WebResponse::sendJson(const char* body, uint16_t statusCode) {
  send(statusCode, "application/json", body);
}

void WebResponse::sendText(const char* body, uint16_t statusCode) {
  send(statusCode, "text/plain; charset=utf-8", body);
}

void WebResponse::send(uint16_t statusCode, const char* contentType, const char* body) {
  if (sent_ || pageOpen_ || !sender_) return;
  sender_(senderContext_, statusCode, contentType, body ? body : "");
  sent_ = true;
}

bool WebResponse::beginPage(const char* title) {
  if (sent_ || pageOpen_ || !pageBegin_) return false;
  pageBegin_(senderContext_, title ? title : "");
  pageOpen_ = true;
  return true;
}

void WebResponse::write(const char* html) {
  if (!pageOpen_ || !pageWrite_) return;
  pageWrite_(senderContext_, html ? html : "");
}

void WebResponse::endPage() {
  if (!pageOpen_ || !pageEnd_) return;
  pageEnd_(senderContext_);
  pageOpen_ = false;
  sent_ = true;
}
