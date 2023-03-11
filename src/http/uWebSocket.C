/*
 * Copyright (C) Sylvain Guinebert, Paris, France.
 *
 * See the LICENSE file for terms of use.
 */

#include "uWebSocket.h"
#include "Configuration.h"
#include "WtReply.h"

#include "Wt/Utils.h"
#include "Wt/WConfig.h"
#include "Wt/WLogger.h"
#include "Wt/WSslInfo.h"
#include "Wt/Http/Message.h"
#include "Wt/Json/json.hpp"
#include "Wt/Json/Parser.h"

#include <fstream>

#include "web/SslUtils.h"

#define PEM_HEADER "-----BEGIN CERTIFICATE-----"
#define PEM_FOOTER "-----END CERTIFICATE-----"

#define PEM_ESCAPED_HEADER "-----BEGIN%20CERTIFICATE-----"
#define PEM_ESCAPED_FOOTER "-----END%20CERTIFICATE-----%0A"

namespace Wt
{
  LOGGER("wthttp");
}

using namespace http::server;

std::string_view status2Text(Reply::status_type status)
{
  switch (status)
  {
  case Reply::switching_protocols:
    return "101 Switching Protocol\r\n";
    break;
  case Reply::ok:
    return  "200 OK\r\n";
    break;
  case Reply::created:
    return  "201 Created\r\n";
    break;
  case Reply::accepted:
    return  "202 Accepted\r\n";
    break;
  case Reply::no_content:
    return  "204 No Content\r\n";
    break;
  case Reply::partial_content:
    return  "206 Partial Content\r\n";
    break;
  case Reply::multiple_choices:
    return  "300 Multiple Choices\r\n";
    break;
  case Reply::moved_permanently:
    return  "301 Moved Permanently\r\n";
    break;
  case Reply::found:
    return  "302 Found\r\n";
    break;
  case Reply::see_other:
    return  "303 See Other\r\n";
    break;
  case Reply::not_modified:
    return "304 Not Modified\r\n";
    break;
  case Reply::moved_temporarily:
    return "307 Moved Temporarily\r\n";
    break;
  case Reply::bad_request:
    return "400 Bad Request\r\n";
    break;
  case Reply::unauthorized:
    return "401 Unauthorized\r\n";
    break;
  case Reply::forbidden:
    return "403 Forbidden\r\n";
    break;
  case Reply::not_found:
    return "404 Not Found\r\n";
    break;
  case Reply::request_entity_too_large:
    return "413 Request Entity too Large\r\n";
    break;
  case Reply::requested_range_not_satisfiable:
    return "416 Requested Range Not Satisfiable\r\n";
    break;
  case Reply::not_implemented:
    return "501 Not Implemented\r\n";
    break;
  case Reply::bad_gateway:
    return "502 Bad Gateway\r\n";
    break;
  case Reply::service_unavailable:
    return "503 Service Unavailable\r\n";
    break;
  case Reply::version_not_supported:
    return "505 HTTP Version Not Supported\r\n";
    break;
  case Reply::no_status:
  case Reply::internal_server_error:
    return "500 Internal Server Error\r\n";
    break;
  default:
    return (int) status + " Unknown\r\n";
  }
}

namespace http
{
  namespace server
  {

    const std::string uWebSocket::empty_;

    uWebSocket::uWebSocket(uWS::HttpResponse<false> *reply, 
                           uWS::HttpRequest *request, 
                           http::server::Configuration *serverConfiguration, 
                           const Wt::EntryPoint *entryPoint,
                           uWS::Loop *loop)
        : uwsreply_(reply), 
          request_(request),
          serverConfiguration_(serverConfiguration), 
          loop_(loop), 
          in_(&in_mem_), 
          out_(&out_buf_)
    {
      setWebSocketRequest(true);
      entryPoint_ = entryPoint;

      for(auto it = request_->begin(); it != request_->end(); ++it)
      {
        auto [key, value] = *it;
        headers_[std::string(key)] = std::string(value);

      }
    }

    void uWebSocket::reset(uWS::WebSocket<false, true, PerSocketData> *ws)
    {
      ws_ = ws;
      uwsreply_ = nullptr;
      request_ = nullptr;
      
      auto loop = uWS::Loop::get();
      loop->defer([=]{
        if(fetchMoreDataCallback_) {
          fetchMoreDataCallback_(Wt::WebWriteEvent::Completed);
        }
      });
    }

    bool uWebSocket::done() const
    {
      return ws_ == nullptr;
    }

    void uWebSocket::flush(ResponseState state, const WriteCallback &callback)
    {
      if(std::this_thread::get_id() !=  loop_->threadId())
      {
        if(callback)
          fetchMoreDataCallback_ = callback;

        loop_->defer([this]{
          const char *data = asio::buffer_cast<const char *>(out_buf_.data());
          int size = asio::buffer_size(out_buf_.data());

          if (ws_)
          {
            
            ws_->send(std::string_view(data, size), uWS::OpCode::TEXT);

            loop_->defer([=]{
              if(fetchMoreDataCallback_)
                fetchMoreDataCallback_(Wt::WebWriteEvent::Completed);
            });
          }
          
          out_buf_.consume(size);

          if (&in_mem_ != in_)
          {
            dynamic_cast<std::fstream *>(in_)->close();
            delete in_;
            in_ = &in_mem_;
          }

          in_mem_.str("");
          in_mem_.clear();
        });
        return;
      }
      const char *data = asio::buffer_cast<const char *>(out_buf_.data());
      int size = asio::buffer_size(out_buf_.data());
      //std::cout << "---buffer : ------------------ " << size << " % " <<std::endl << std::string_view(data, size)  << std::endl;

      //if(!fetchMoreDataCallback_)
      //Wt::WebRequest::WriteCallback rb = fetchMoreDataCallback_;
          // if(callback)
          //   std::cout << "%%%%%callback is valid%%%%%" << std::endl;
          // else
          //   std::cout << "%%%%%callback is invalid " << std::endl;

      if(callback)
        fetchMoreDataCallback_ = callback;

      if (ws_)
      {
        
        ws_->send(std::string_view(data, size), uWS::OpCode::TEXT);

        loop_->defer([=]{
          if(fetchMoreDataCallback_)
            fetchMoreDataCallback_(Wt::WebWriteEvent::Completed);
        });
      }
      
      out_buf_.consume(size);

      if (&in_mem_ != in_)
      {
        dynamic_cast<std::fstream *>(in_)->close();
        delete in_;
        in_ = &in_mem_;
      }

      in_mem_.str("");
      in_mem_.clear();
    }

    void uWebSocket::readWebSocketMessage(const ReadCallback &callback)
    {
      if(readMessageCallback_)
        return;

      //std::cout << "-------- set callback ------- " << std::endl;


      readMessageCallback_ = callback;

      if (&in_mem_ != in_)
      {
        dynamic_cast<std::fstream *>(in_)->close();
        delete in_;
        in_ = &in_mem_;
      }

      in_mem_.str("");
      in_mem_.clear();
    }

    bool uWebSocket::webSocketMessagePending() const //not correct ??
    {
      in_->seekg(0, std::ios::end);
      int size = in_->tellg();
      in_->seekg(0, std::ios::beg);
      //std::cout << "-------- pending ------- " << size << std::endl;
      return false; //size != 0;
    }

    bool uWebSocket::detectDisconnect(const DisconnectCallback &callback)
    {
      return true;
    }

    void uWebSocket::setStatus(int status)
    {
      status_ = (Reply::status_type)status;
      if(uwsreply_)
        uwsreply_->writeStatus(status2Text(status_));
    }

    void uWebSocket::setContentLength(::int64_t length)
    {
    }

    void uWebSocket::addHeader(const std::string &name, std::string_view value)
    {
      if(uwsreply_)
        uwsreply_->writeHeader(name, value);
    }

    void uWebSocket::setContentType(const std::string &value)
    {
      if(uwsreply_)
        uwsreply_->writeHeader("Content-Type", value);
    }

    void uWebSocket::setRedirect(const std::string &url)
    {
      if(uwsreply_)
        uwsreply_->writeHeader("Location", url);
    }

    std::string_view uWebSocket::headerValue(const char *name) const
    {
      auto it = headers_.find(name);

      return it != headers_.end() ? it->second : "";
    }

    std::vector<Wt::Http::Message::Header> uWebSocket::headers() const
    {
      std::vector<Wt::Http::Message::Header> headerVector;
      WtReplyPtr p = reply_;
      if (!p.get())
        return headerVector;

      const std::list<Request::Header> &headers = p->request().headers;

      for (std::list<Request::Header>::const_iterator it = headers.begin(); it != headers.end(); ++it)
      {
        if (cstr(it->name))
        {
          headerVector.push_back(Wt::Http::Message::Header(it->name.str(), it->value.str()));
        }
      }

      return headerVector;
    }

    const char *uWebSocket::cstr(const buffer_string &bs) const
    {
      if (!bs.next)
        return bs.data;
      else
      {
        s_.push_back(bs.str());
        return s_.back().c_str();
      }
    }

    ::int64_t uWebSocket::contentLength() const
    {
      return contentLength_;
    }

    std::string_view uWebSocket::contentType() const
    {
      return headerValue("content-type");
    }

    std::string_view uWebSocket::envValue(const char *name) const
    {
      if (strcmp(name, "CONTENT_TYPE") == 0)
      {
        return headerValue("content-type");
      }
      else if (strcmp(name, "CONTENT_LENGTH") == 0)
      {
        return headerValue("content-length");
      }
      else if (strcmp(name, "SERVER_SIGNATURE") == 0)
      {
        return "<address>Wt httpd server</address>";
      }
      else if (strcmp(name, "SERVER_SOFTWARE") == 0)
      {
        return "Wthttpd/" WT_VERSION_STR;
      }
      else if (strcmp(name, "SERVER_ADMIN") == 0)
      {
        return "sylvain.guinebert.sg@gmail.com";
      }
      else if (strcmp(name, "REMOTE_ADDR") == 0)
      {
        return remoteAddr().data();
      }
      else if (strcmp(name, "DOCUMENT_ROOT") == 0)
      {
        return serverConfiguration_->docRoot().c_str();
      }
      else
        return nullptr;
    }

    std::string_view uWebSocket::serverName() const
    {
      if (!request_)
        return empty_;

      return serverConfiguration_->serverName();
    }

    std::string_view uWebSocket::serverPort() const
    {
      if (!request_)
        return empty_;

      if (serverPort_.empty())
        serverPort_ = empty_;

      return serverPort_;
    }

    std::string_view uWebSocket::scriptName() const
    {
      if (!request_)
        return empty_;

      return request_->getUrl();
    }

    std::string_view uWebSocket::requestMethod() const
    {
      if (!request_)
        return nullptr;

      return request_->getMethod();
    }

    std::string_view uWebSocket::queryString() const
    {
      if (!request_)
        return empty_;
        
      return request_->getQuery();
    }

    std::string_view uWebSocket::pathInfo() const
    {
      if (!request_)
        return empty_;

      return "";
    }

    std::string_view uWebSocket::remoteAddr() const
    {
      if (!uwsreply_)
        return empty_;

      return uwsreply_->getRemoteAddress();
    }

    const char *uWebSocket::urlScheme() const
    {
      //WtReplyPtr p = reply_;
      if (!request_)
        return "http";

      //return p->request().urlScheme;
      //std::cerr << "urlScheme " << request_->urlScheme() << std::endl;
      return request_->urlScheme();
    }

    bool uWebSocket::isSynchronous() const
    {
      return false;
    }

    std::unique_ptr<Wt::WSslInfo> uWebSocket::sslInfo(const Wt::Configuration &conf) const
    {
      return sslInfoFromHeaders();

      auto result = reply_->request().sslInfo();
      if (conf.behindReverseProxy() ||
          conf.isTrustedProxy(remoteAddr()))
      {
#ifdef HTTP_WITH_SSL
        if (!result)
          result = sslInfoFromJson();
#endif // HTTP_WITH_SSL
        if (!result)
          result = sslInfoFromHeaders();
      }
      return result;
    }

#ifdef HTTP_WITH_SSL
    std::unique_ptr<Wt::WSslInfo> uWebSocket::sslInfoFromJson() const
    {
      const char *const ssl_client_certificates = std::string(headerValue("x-wt-ssl-client-certificates")).c_str();

      if (!ssl_client_certificates)
        return nullptr;

      Wt::Json::Object obj;
      Wt::Json::ParseError error;
      if (!Wt::Json::parse(Wt::Utils::base64Decode(ssl_client_certificates), obj, error))
      {
        LOG_ERROR("error while parsing client certificates");
        return nullptr;
      }

      std::string clientCertificatePem = obj["client-certificate"].get_string("");

      X509 *cert = Wt::Ssl::readFromPem(clientCertificatePem);

      if (cert)
      {
        Wt::WSslCertificate clientCert = Wt::Ssl::x509ToWSslCertificate(cert);
        X509_free(cert);

        Wt::Json::Array &arr = obj["client-pem-certification-chain"].as_array();

        std::vector<Wt::WSslCertificate> clientCertChain;

        for (const auto &cert : arr)
        {
          clientCertChain.push_back(Wt::Ssl::x509ToWSslCertificate(Wt::Ssl::readFromPem(cert.get_string(""))));
        }

        Wt::ValidationState state = static_cast<Wt::ValidationState>(obj["client-verification-result-state"].get_int64(0));
        Wt::WString message = obj["client-verification-result-message"].get_string("");

        return std::make_unique<Wt::WSslInfo>(clientCert,
                                              clientCertChain,
                                              Wt::WValidator::Result(state, message));
      }

      return nullptr;
    }
#endif // HTTP_WITH_SSL

    std::unique_ptr<Wt::WSslInfo> uWebSocket::sslInfoFromHeaders() const
    {
      return nullptr;
      auto client_verify = headerValue("x-ssl-client-verify");
      auto client_s_dn = headerValue("x-ssl-client-s-dn");
      auto client_i_dn = headerValue("x-ssl-client-i-dn");
      auto validity_start = headerValue("x-ssl-client-v-start");
      auto validity_end = headerValue("x-ssl-client-v-end");
      auto client_cert = headerValue("x-ssl-client-cert");

      if (client_verify.empty())
        return nullptr;

      if (boost::iequals(client_verify, "none"))
        return nullptr;

      enum Verify
      {
        SUCCESS,
        FAILED,
        GENEROUS
      };

      const char *failedReason = nullptr;

      Verify v = FAILED;
      if (boost::iequals(client_verify, "success"))
        v = SUCCESS;
      else if (boost::iequals(client_verify, "generous"))
        v = GENEROUS;
      else if (boost::istarts_with(client_verify, "failed:"))
      {
        v = FAILED;
        failedReason = client_verify.data() + sizeof("FAILED");
      }
      else
        return nullptr;

      std::string clientCertStr;
      if (!client_cert.empty())
      {
        clientCertStr = client_cert;
        boost::trim(clientCertStr);
        if (boost::starts_with(clientCertStr, PEM_HEADER))
        {
          const std::size_t start = sizeof(PEM_HEADER) - 1;
          const std::size_t end = clientCertStr.find(PEM_FOOTER);
          if (end != std::string::npos)
          {
            for (std::size_t i = start; i < end; ++i)
            {
              if (clientCertStr[i] == ' ')
                clientCertStr[i] = '\n';
            }
          }
          else
          {
            clientCertStr.clear();
          }
        }
        else if (boost::starts_with(clientCertStr, PEM_ESCAPED_HEADER) &&
                 boost::ends_with(clientCertStr, PEM_ESCAPED_FOOTER))
        {
          clientCertStr = Wt::Utils::urlDecode(clientCertStr);
        }
        else
        {
          clientCertStr.clear();
        }
      }

#ifdef WT_WITH_SSL
      if (!clientCertStr.empty())
      {
        // try parse cert, use cert for all other info
        X509 *cert = Wt::Ssl::readFromPem(clientCertStr);

        if (cert)
        {
          Wt::WSslCertificate clientCert = Wt::Ssl::x509ToWSslCertificate(cert);
          return std::make_unique<Wt::WSslInfo>(clientCert,
                                                std::vector<Wt::WSslCertificate>(),
                                                Wt::WValidator::Result(v == SUCCESS ? Wt::ValidationState::Valid : Wt::ValidationState::Invalid,
                                                                       failedReason ? Wt::utf8(failedReason) : Wt::WString::Empty));
        }
      }
#endif // WT_WITH_SSL

      if (!client_s_dn.empty() &&
          !client_i_dn.empty() &&
          !validity_start.empty() &&
          !validity_end.empty())
      {
        std::vector<Wt::WSslCertificate::DnAttribute> subjectDn =
            Wt::WSslCertificate::dnFromString(client_s_dn);
        std::vector<Wt::WSslCertificate::DnAttribute> issuerDn =
            Wt::WSslCertificate::dnFromString(client_i_dn);

        const Wt::WString validityFormat = Wt::utf8("MMM dd hh:mm:ss yyyy 'GMT'");
        Wt::WDateTime validityStart = Wt::WDateTime::fromString(
            Wt::utf8(std::string(validity_start)), validityFormat);
        Wt::WDateTime validityEnd = Wt::WDateTime::fromString(
            Wt::utf8(std::string(validity_end)), validityFormat);

        Wt::WSslCertificate clientCert(subjectDn,
                                       issuerDn,
                                       validityStart,
                                       validityEnd,
                                       clientCertStr);
        return std::make_unique<Wt::WSslInfo>(clientCert,
                                              std::vector<Wt::WSslCertificate>(),
                                              Wt::WValidator::Result(v == SUCCESS ? Wt::ValidationState::Valid : Wt::ValidationState::Invalid,
                                                                     failedReason ? Wt::utf8(failedReason) : Wt::WString::Empty));
      }

      return nullptr;
    }

    const std::vector<std::pair<std::string, std::string>> &uWebSocket::urlParams() const
    {
      WtReplyPtr p = reply_;
      if (!p.get())
        return WebRequest::urlParams();

      return p->request().url_params;
    }

    bool uWebSocket::consumeWebSocketMessage(uWS::OpCode opcode,
                                             std::string_view message)
    {
      in_mem_.str("");
      in_mem_.clear();
      // auto len = contentLength();
      // auto buf = std::unique_ptr<char[]>(new char[len + 1]);

      // in_mem_.read(buf.get(), len);
      // buf[len] = 0;

      //std::cout << "message : " << message << std::endl;
      msg_ = message;
      in_mem_ << message;

      switch (opcode)
      {
      case uWS::OpCode::CLOSE:
        in_mem_.str("");
        in_mem_.clear();

        /* fall through */
      // case continuation:
      //   LOG_DEBUG("WtReply::consumeWebSocketMessage(): rx continuation");

      case uWS::OpCode::TEXT:
      {

        /*
          * FIXME: check that we have received the entire message.
          *  If yes: call the callback; else resume reading (expecting
          *  continuation frames in that case)
          */
        Wt::WebRequest::ReadCallback cb = readMessageCallback_;
        readMessageCallback_ = nullptr;

         //std::cout << "uWS::OpCode::TEXT : " << &cb << std::endl;

        // We need to post since in Wt we may be entering a recursive event
        // loop and we need to release the strand
        auto loop = uWS::Loop::get();
        loop->defer([=]{
            cb(Wt::WebReadEvent::Message);
        });
        //cb(Wt::WebReadEvent::Message);

        break;
      }
      case uWS::OpCode::PING:
      {
        //LOG_DEBUG("WtReply::consumeWebSocketMessage(): rx ping");
        //std::cout << "OpCode::PING : " << std::endl;

         Wt::WebRequest::ReadCallback cb = readMessageCallback_;
         readMessageCallback_ = nullptr;

        // We need to post since in Wt we may be entering a recursive event
        // loop and we need to release the strand
        auto loop = uWS::Loop::get();
        loop->defer([=]{
            cb(Wt::WebReadEvent::Ping);
        });

        break;
      }
      break;
      case uWS::OpCode::BINARY:
        LOG_ERROR("ws: binary_frame received, don't know what to do.");

        /* fall through */
      case uWS::OpCode::PONG:
      {
        //LOG_DEBUG("WtReply::consumeWebSocketMessage(): rx pong");

        /*
        * We do not need to send a response; resume reading, keeping the
        * same read callback
        */
         Wt::WebRequest::ReadCallback cb = readMessageCallback_;
         readMessageCallback_ = nullptr;
         readWebSocketMessage(cb);
      }

      break;
      }
    return true;
  }

  } // namespace server
} // namespace http
