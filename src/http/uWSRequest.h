// This may look like C code, but it's really -*- C++ -*-
/*
 */
#pragma once

#include <sstream>
#include <ostream>

#include "WebRequest.h"
//#include "WtReply.h"
#include <seastar/http/request.hh>
#include <seastar/http/reply.hh>
#include <seastar/core/iostream.hh>

#include "http/Configuration.h"
#include "Wt/Http/Message.h"

#include <boost/asio.hpp>

// namespace uWS {
//     /* Type queued up when publishing */
//     struct TopicTreeMessage {
//         std::string message;
//         /*OpCode*/ int opCode;
//         bool compress;
//     };
//     struct TopicTreeBigMessage {
//         std::string_view message;
//         /*OpCode*/ int opCode;
//         bool compress;
//     };
// }

/* An app is a convenience wrapper of some of the most used fuctionalities and allows a
 * builder-pattern kind of init. Apps operate on the implicit thread local Loop */

// namespace http {
// namespace server {
// class uWSRequest;
// }}

// struct PerSocketData {
//     /* Define your user data */
//     int something;
//     http::server::uWSRequest *webresponse = 0;
// };


namespace http {

class uWSRequest final : public Wt::WebResponse
{
public:
  uWSRequest(seastar::http::reply *reply,
             seastar::http::request *request,
             //uWS::WebSocket<false, true, PerSocketData> *ws,
             http::server::Configuration *serverConfiguration,
             const Wt::EntryPoint *entryPoint);

  void reset(seastar::http::reply *reply,
             seastar::http::request *request,
             const Wt::EntryPoint *entryPoint);

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
  virtual void append(const char *s, int length) //override
  {
    //uwsreply_->write(std::string_view(s, length));
  }

  virtual void setStatus(int status) override;
  virtual void setContentLength(::int64_t length) override;

  virtual void addHeader(const std::string &name, const std::string &value) override;
  virtual void setContentType(const std::string& value) override;
  virtual void setRedirect(const std::string& url) override;

  virtual std::vector<Wt::Http::Message::Header> headers() const override;


  virtual ::int64_t contentLength() const override;
  virtual std::string_view contentType() const override;
  virtual std::string_view envValue(const char *name) const override;
  virtual std::string_view headerValue(const char *name) const override;
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

  void send() 
  {
//    const char *data = boost::asio::buffer_cast<const char *>(out_buf_.data());
//    int size = boost::asio::buffer_size(out_buf_.data());
//    uwsreply_->write_body(contenttype_, seastar::sstring(data, size));
//        //std::cout << "buffer : ------------------ " << size << std::endl << std::string_view(data, size);
//    auto [ok, hasResponded] = uwsreply_->tryEnd(std::string_view(data, size)); //out_buf_.str()
//    //std::cout << "tryEND : ------------------" << ok << " " << hasResponded  << std::endl;
  }

  virtual void write_body(std::string_view response) override
  {
    //std::cout << "test " << response << std::endl;
    uwsreply_->write_body("text/html", seastar::sstring(response));
  }

  void write(std::ostream& os)
  {
    //seastar::output_stream<char> out(uwsreply_->_content);
    //std::ostream c(&uwsreply_->_content.back());
  }
  void setmessage(std::string_view os)
  {
    // in_mem_.str("");
    // in_mem_.clear();
    //std::cout << "message post : " << os << std::endl;
    setContentLength(os.length());
    msg_ = os;

    in_mem_ << os;

    auto str = in_mem_.str();
    //std::cout << "in_mem :  --------------------------" << str << std::endl;
  }
  virtual http::uWSRequest &operator<<(std::string_view toclient) override
  {

    out_ << toclient;
    //uwsreply_->_content.append(toclient.data(), toclient.size());
    //uwsreply_->end(toclient);
    //out_buf_.write(toclient.data(), toclient.size()).get();// << "toclient";
    return *this;
  }

  void saveRequest()
  {
    //auto req = new uWS::HttpRequest;
    savedrequest_ = request_;


    headers_.clear();
//    for(auto it = request_->begin(); it != request_->end(); ++it)
//    {
//      auto [key, value] = *it;
//      //std::cout << "key, value : " << key << ", " << value << std::endl;
//      auto test = std::string(value);
//      headers_[std::string(key)] = test;
//    }
//    method_ = std::string(request_->getMethod());
//    query_ = std::string(request_->getQuery());

    request_ = 0;
    // for(auto [key, value] : headers_)
    // {
    //   std::cout << "key, value : " << key << ", " << value << std::endl;
    // }

    //std::cout << "content-type header: " << request_->getHeader("content-type") << std::endl;
  }


private:
  std::string_view msg_;
  //WtReplyPtr reply_;
  http::server::Configuration *serverConfiguration_;

  seastar::http::reply *uwsreply_;
  seastar::http::request *request_;
  seastar::http::request *savedrequest_;
  //std::unique_ptr<uWS::HttpRequest> request_;
  //uWS::WebSocket<false, true, PerSocketData> *ws_;
  seastar::http::reply::status_type status_;
  ::int64_t contentLength_, bodyReceived_;

  //std::stringstream out_;
  boost::asio::streambuf out_buf_;
  //seastar::output_stream<char> out_buf_;
  //seastar::temporary_buffer<char> out_buf2_;
  //seastar::memory_output_stream<char> cd
  //std::ostringstream out_buf2_;
  std::ostream out_;
  std::iostream *in_; std::stringstream in_mem_;

  bool isWebSocket_;

  mutable std::string serverPort_;
  mutable std::vector<std::string> s_;
  std::string scriptname_;
  std::unordered_map<std::string, std::string> headers_;
  std::string method_, query_;
  std::string contenttype_;

#ifdef HTTP_WITH_SSL
  // Extracts SSL info from internal Wt-specific base64-encoded JSON implementation,
  // used for Wt's own reverse proxy (dedicated session processes).
  std::unique_ptr<Wt::WSslInfo> sslInfoFromJson() const;
#endif // HTTP_WITH_SSL
  // Extract SSL info from X-SSL-Client-* headers. Can be used when Wt is behind an SSL-terminating
  // proxy like nginx or Apache (HAProxy's headers are not currently supported).
  std::unique_ptr<Wt::WSslInfo> sslInfoFromHeaders() const;

  //const char *cstr(const buffer_string& bs) const;

  static const std::string empty_;
};


}
