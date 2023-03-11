// This may look like C code, but it's really -*- C++ -*-
/*
 */
#pragma once

#include <sstream>
#include <ostream>

#include "WebRequest.h"
#include "WtReply.h"
#include "Wt/Http/Message.h"

/* An app is a convenience wrapper of some of the most used fuctionalities and allows a
 * builder-pattern kind of init. Apps operate on the implicit thread local Loop */
#include "uws/App.h"
#include "uws/HttpContext.h"
#include "uws/HttpResponse.h"
#include "uws/HttpParser.h"
#include "uws/WebSocket.h"

namespace http
{
  namespace server
  {
    class uWebSocket;
  }
}

struct PerSocketData {
    /* Define your user data */
    int something;
    http::server::uWebSocket *webresponse = 0;
};


namespace http {
namespace server {


//template<bool SSL>
class uWebSocket final : public Wt::WebResponse
{
public:
  uWebSocket(uWS::HttpResponse<false> *reply,
             uWS::HttpRequest *request,
             http::server::Configuration *serverConfiguration,
             const Wt::EntryPoint *entryPoint,
             uWS::Loop *loop);

  void reset(uWS::WebSocket<false, true, PerSocketData> *ws);

  bool done() const;

  virtual void flush(ResponseState state, const WriteCallback& callback) override;
  virtual void readWebSocketMessage(const ReadCallback& callback) override;
  virtual bool webSocketMessagePending() const override;
  virtual bool detectDisconnect(const DisconnectCallback& callback) override;

  virtual std::istream& in() override { return *in_; }
  virtual std::ostream& out() override { return out_; }
  virtual std::ostream& err() override { return std::cerr; }

  /*! \brief Appends a string.
   *
   * Appends \p length bytes from the given string.
   */
  virtual void append(const char *s, int length) override 
  {
    //uwsreply_->write(std::string_view(s, length));
  }

  virtual void setStatus(int status) override;
  virtual void setContentLength(::int64_t length) override;

  virtual void addHeader(const std::string& name, std::string_view value) override;
  virtual void setContentType(const std::string& value) override;
  virtual void setRedirect(const std::string& url) override;

  virtual std::string_view contentType() const override;
  virtual ::int64_t contentLength() const override;

  virtual std::string_view envValue(const char *name) const override;
  virtual std::string_view headerValue(const char *name) const override;
  virtual std::vector<Wt::Http::Message::Header> headers() const override;
  virtual std::string_view serverName() const override;
  virtual std::string_view serverPort() const override;
  virtual std::string_view scriptName() const override;
  virtual std::string_view requestMethod() const override;
  virtual std::string_view queryString() const override;
  virtual std::string_view pathInfo() const override;
  virtual std::string_view remoteAddr() const override;
  virtual const char *urlScheme() const override;
  bool isSynchronous() const;
  virtual std::unique_ptr<Wt::WSslInfo> sslInfo(const Wt::Configuration & conf) const override;
  virtual const std::vector<std::pair<std::string, std::string> > &urlParams() const override;

  bool consumeWebSocketMessage(uWS::OpCode opcode, std::string_view message);

  void send() 
  {
    const char *data = asio::buffer_cast<const char *>(out_buf_.data());
    int size = asio::buffer_size(out_buf_.data());
        //std::cout << "buffer : ------------------ " << size << std::endl << std::string_view(data, size);
    auto [ok, hasResponded] = uwsreply_->tryEnd(std::string_view(data, size)); //out_buf_.str()
    //std::cout << "tryEND : ------------------" << ok << " " << hasResponded  << std::endl;
  }
  void write(std::ostream& os)
  {

  }
  void setmessage(std::string_view os)
  {
    setContentLength(os.length());
    msg_ = os;

    in_mem_ << os;

  }
  virtual Wt::WebResponse &operator<<(std::string_view toclient) override
  {
    //uwsreply_->end(toclient);
    out_ << toclient;
    return *this;
  }


private:
  std::string_view msg_;
  WtReplyPtr reply_; 
  uWS::Loop *loop_;
  uWS::WebSocket<false, true, PerSocketData> *ws_ = nullptr;
  http::server::Configuration *serverConfiguration_;
  uWS::HttpResponse<false> *uwsreply_;
  uWS::HttpRequest *request_;
  Reply::status_type status_;
  ::int64_t contentLength_ = 0, bodyReceived_ = 0;

  //std::stringstream out_;  
  asio::streambuf out_buf_;
  std::ostream out_;
  std::iostream *in_; std::stringstream in_mem_;

  Wt::WebRequest::WriteCallback fetchMoreDataCallback_ = nullptr;
  Wt::WebRequest::ReadCallback readMessageCallback_ = nullptr;
  std::unordered_map<std::string, std::string> headers_;

  bool isWebSocket_;

  mutable std::string serverPort_;
  mutable std::vector<std::string> s_;

#ifdef HTTP_WITH_SSL
  // Extracts SSL info from internal Wt-specific base64-encoded JSON implementation,
  // used for Wt's own reverse proxy (dedicated session processes).
  std::unique_ptr<Wt::WSslInfo> sslInfoFromJson() const;
#endif // HTTP_WITH_SSL
  // Extract SSL info from X-SSL-Client-* headers. Can be used when Wt is behind an SSL-terminating
  // proxy like nginx or Apache (HAProxy's headers are not currently supported).
  std::unique_ptr<Wt::WSslInfo> sslInfoFromHeaders() const;

  const char *cstr(const buffer_string& bs) const;

  static const std::string empty_;
};


}
}
