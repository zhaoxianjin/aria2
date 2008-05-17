/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "HttpInitiateConnectionCommand.h"
#include "Request.h"
#include "DownloadEngine.h"
#include "HttpConnection.h"
#include "HttpRequest.h"
#include "Segment.h"
#include "HttpRequestCommand.h"
#include "HttpProxyRequestCommand.h"
#include "DlAbortEx.h"
#include "Option.h"
#include "Logger.h"
#include "Socket.h"
#include "message.h"
#include "prefs.h"

namespace aria2 {

HttpInitiateConnectionCommand::HttpInitiateConnectionCommand
(int cuid,
 const RequestHandle& req,
 RequestGroup* requestGroup,
 DownloadEngine* e):
  InitiateConnectionCommand(cuid, req, requestGroup, e) {}

HttpInitiateConnectionCommand::~HttpInitiateConnectionCommand() {}

Command* HttpInitiateConnectionCommand::createNextCommand
(const std::deque<std::string>& resolvedAddresses)
{
  Command* command;
  if(useHTTPProxy()) {
    logger->info(MSG_CONNECTING_TO_SERVER, cuid,
		 e->option->get(PREF_HTTP_PROXY_HOST).c_str(),
		 e->option->getAsInt(PREF_HTTP_PROXY_PORT));
    socket.reset(new SocketCore());
    socket->establishConnection(resolvedAddresses.front(),
				e->option->getAsInt(PREF_HTTP_PROXY_PORT));
    if(useProxyTunnel()) {
      command = new HttpProxyRequestCommand(cuid, req, _requestGroup, e, socket);
    } else if(useProxyGet()) {
      SharedHandle<HttpConnection> httpConnection(new HttpConnection(cuid, socket, e->option));
      command = new HttpRequestCommand(cuid, req, _requestGroup,
				       httpConnection, e, socket);
    } else {
      // TODO
      throw DlAbortEx("ERROR");
    }
  } else {
    SharedHandle<SocketCore> pooledSocket =
      e->popPooledSocket(resolvedAddresses, req->getPort());
    if(pooledSocket.isNull()) {
      logger->info(MSG_CONNECTING_TO_SERVER, cuid, req->getHost().c_str(),
		   req->getPort());
      socket.reset(new SocketCore());
      socket->establishConnection(resolvedAddresses.front(), req->getPort());
    } else {
      socket = pooledSocket;
    }
    SharedHandle<HttpConnection> httpConnection(new HttpConnection(cuid, socket, e->option));
    command = new HttpRequestCommand(cuid, req, _requestGroup, httpConnection,
				     e, socket);
  }
  return command;
}

bool HttpInitiateConnectionCommand::useProxyGet() const
{
  return e->option->get(PREF_HTTP_PROXY_METHOD) == V_GET;
}

bool HttpInitiateConnectionCommand::useProxyTunnel() const
{
  return e->option->get(PREF_HTTP_PROXY_METHOD) == V_TUNNEL;
}

} // namespace aria2
