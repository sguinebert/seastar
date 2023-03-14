/*
 * Copyright (C) 2008 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include <fstream>

#include "Wt/WLogger.h"
#include "Wt/WFileResource.h"

#include <seastar/core/seastar.hh>
#include <seastar/core/file.hh>
#include <seastar/core/reactor.hh>

//#include <coroutine>

using namespace seastar;

namespace Wt {

LOGGER("WFileResource");

WFileResource::WFileResource()
{ }

WFileResource::WFileResource(const std::string& fileName)
  : fileName_(fileName)
{ }

WFileResource::WFileResource(const std::string& mimeType,
                             const std::string& fileName)
  : WStreamResource(mimeType),
    fileName_(fileName)
{ }

WFileResource::~WFileResource()
{
  beingDeleted();
}

void WFileResource::setFileName(const std::string& fileName)
{
  fileName_ = fileName;
  setChanged();
}


#ifdef CLASSIC_HANDLE
void WFileResource::handleRequest(const Http::Request& request,
                                  Http::Response& response)
{
  std::ifstream r(fileName_.c_str(), std::ios::in | std::ios::binary);
  if (!r) {
    LOG_ERROR("Could not open file for reading: " << fileName_);
  }
  handleRequestPiecewise(request, response, r);
}
#else
seastar::future<std::unique_ptr<seastar::http::reply> > WFileResource::handle(const seastar::sstring &path,
                                                                             std::unique_ptr<seastar::http::request> request,
                                                                             std::unique_ptr<seastar::http::reply> response)
{
//  std::ifstream r(fileName_.c_str(), std::ios::in | std::ios::binary);
//  if (!r) {
//      LOG_ERROR("Could not open file for reading: " << fileName_);
//  }
  auto file = co_await seastar::open_file_dma(fileName_, seastar::open_flags::ro);
  auto size = co_await file.size();

  //auto size_read = co_await file.dma_read(0, );


  //handleRequestPiecewise(request, response, r);

  //  for (unsigned int i = 0; i < (*data).size(); ++i)
  //    response.out().put((*data)[i]);
  co_return co_await seastar::make_ready_future<std::unique_ptr<seastar::http::reply>>(std::move(response));
}
#endif

}
