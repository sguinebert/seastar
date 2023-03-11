/*
 * Copyright (C) 2010 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include "Wt/WLogger.h"
#include "Wt/WSslInfo.h"

#include "WebSession.h"
#include "WebSocketMessage.h"

#include <cstring>

namespace Wt {

LOGGER("WebSocketMessage");

WebSocketMessage::WebSocketMessage(WebSession *session)
  : session_(session)
{
  queryString_ = "wtd=" + session_->sessionId() + "&request=jsupdate";
}

void WebSocketMessage::flush(ResponseState state,
                             const WriteCallback& callback)
{
  if (state != ResponseState::ResponseDone)
    error("flush(" + std::to_string(static_cast<unsigned int>(state))
          + ") expected");

  session_->pushUpdates();

  delete this;
}

void WebSocketMessage::setWebSocketMessageCallback(const ReadCallback& callback)
{
  error("setWebSocketMessageCallback() not supported");
}

bool WebSocketMessage::webSocketMessagePending() const
{
  error("webSocketMessagePending() not supported");
  return false;
}

std::istream& WebSocketMessage::in()
{
  return webSocket()->in();
}

std::ostream& WebSocketMessage::out()
{
  return webSocket()->out();
}

std::ostream& WebSocketMessage::err()
{
  return webSocket()->err();
}

void WebSocketMessage::setRedirect(const std::string& url)
{
  error("setRedirect() not supported");
}

void WebSocketMessage::setStatus(int status)
{
  error("setStatus() not supported");
}

void WebSocketMessage::setContentType(const std::string& value)
{
  if (value != "text/javascript; charset=UTF-8")
    error("setContentType(): text/javascript expected");
}

void WebSocketMessage::setContentLength(::int64_t length)
{
  // We have no use for it, web socket messages are framed
}

void WebSocketMessage::addHeader(const std::string& name,
                                 const std::string& value)
{
  error("addHeader(): not supported");
}

std::string_view WebSocketMessage::envValue(const char *name) const
{
  return webSocket()->envValue(name);
}

::int64_t WebSocketMessage::contentLength() const
{
  webSocket()->in().seekg(0, std::ios::end);
  int length = webSocket()->in().tellg();
  webSocket()->in().seekg(0, std::ios::beg);
  return length;
}

std::string_view WebSocketMessage::contentType() const
{
  return "application/x-www-form-urlencoded";
}

std::string_view WebSocketMessage::serverName() const
{
  return webSocket()->serverName();
}

std::string_view WebSocketMessage::serverPort() const
{
  return webSocket()->serverPort();
}

std::string_view WebSocketMessage::scriptName() const
{
  return webSocket()->scriptName();
}

std::string_view WebSocketMessage::requestMethod() const
{
  return "POST";
}

std::string_view WebSocketMessage::queryString() const
{
  return queryString_;
}

std::string_view WebSocketMessage::pathInfo() const
{
  return webSocket()->pathInfo();
}

std::string_view WebSocketMessage::remoteAddr() const
{
  return webSocket()->remoteAddr();
}

const char *WebSocketMessage::urlScheme() const
{
  const char *wsScheme = webSocket()->urlScheme();
  if (std::strcmp(wsScheme, "wss") == 0 ||
      std::strcmp(wsScheme, "https") == 0)
    return "https";
  else
    return "http";
}

std::unique_ptr<Wt::WSslInfo> WebSocketMessage::sslInfo(const Configuration & conf) const
{
  return webSocket()->sslInfo(conf);
}

std::string_view WebSocketMessage::headerValue(const char *name) const
{
  return webSocket()->headerValue(name);
}

std::vector<Wt::Http::Message::Header> WebSocketMessage::headers() const
{
  return webSocket()->headers();
}

void WebSocketMessage::error(const std::string& msg) const
{
  LOG_ERROR("WebSocketMessage error: " + msg);
}

WebRequest *WebSocketMessage::webSocket() const
{
  return session_->webSocket_;
}

}
