// This may look like C code, but it's really -*- C++ -*-
/*
 * Copyright (C) 2010 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#ifndef WEB_SOCKET_MESSAGE_H_
#define WEB_SOCKET_MESSAGE_H_

#include "WebRequest.h"

namespace Wt {

/*
 * Wraps a WebSocket message as a web request.
 */
class WT_API WebSocketMessage final : public WebResponse
{
public:
  WebSocketMessage(WebSession *session);

  virtual void flush(ResponseState state = ResponseState::ResponseDone,
                     const WriteCallback& callback = WriteCallback()) override;

  void setWebSocketMessageCallback(const ReadCallback& callback);
  virtual bool webSocketMessagePending() const override;

  virtual std::istream& in() override;
  virtual std::ostream& out() override;
  virtual std::ostream& err() override;

  virtual void setRedirect(const std::string& url) override;
  virtual void setStatus(int status) override;
  virtual void setContentType(const std::string& value) override;
  virtual void setContentLength(::int64_t length) override;

  virtual void addHeader(const std::string& name, const std::string& value) override;
  virtual std::string_view envValue(const char *name) const override;

  virtual std::string_view serverName() const override;
  virtual std::string_view serverPort() const override;
  virtual std::string_view scriptName() const override;
  virtual std::string_view requestMethod() const override;
  virtual std::string_view queryString() const override;
  virtual std::string_view pathInfo() const override;
  virtual std::string_view remoteAddr() const override;

  virtual const char *urlScheme() const override;

  virtual std::unique_ptr<Wt::WSslInfo> sslInfo(const Configuration & conf) const override;

  virtual std::string_view headerValue(const char *name) const override;

  virtual WebRequest &operator<<(std::string_view toclient) override
  {
      return *this;
  };

  virtual void write_body(std::string_view response) override
  {
      //uwsreply_->write_body(contenttype_, seastar::sstring(response));
  }

#ifndef WT_TARGET_JAVA
  virtual std::vector<Wt::Http::Message::Header> headers() const override;
#endif

  virtual bool isWebSocketMessage() const override { return true; }

  virtual std::string_view contentType() const override;
  virtual ::int64_t contentLength() const override;

private:
  WebSession *session_;
  std::string queryString_;

  WebRequest *webSocket() const;
  void error(const std::string& msg) const;
};

}

#endif // WEB_SOCKET_MESSAGE_H_
