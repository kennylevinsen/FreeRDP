/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 *
 * Copyright 2019 Mati Shabtay <matishabtay@gmail.com>
 * Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
 * Copyright 2019 Idan Freiberg <speidy@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FREERDP_SERVER_PROXY_PFCONTEXT_H
#define FREERDP_SERVER_PROXY_PFCONTEXT_H

#include <freerdp/freerdp.h>
#include <freerdp/channels/wtsvc.h>
#include <freerdp/client/rdpei.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/server/rdpgfx.h>
#include <freerdp/client/disp.h>
#include <freerdp/server/disp.h>

#include "pf_config.h"
#include "pf_server.h"
#include "pf_filters.h"

typedef struct proxy_data proxyData;

/**
 * Wraps rdpContext and holds the state for the proxy's server.
 */
struct p_server_context
{
	rdpContext _context;

	proxyData* pdata;

	HANDLE vcm;
	HANDLE thread;
	HANDLE dynvcReady;

	RdpgfxServerContext* gfx;
	DispServerContext* disp;

	BOOL dispOpened;
};
typedef struct p_server_context pServerContext;

/**
 * Wraps rdpContext and holds the state for the proxy's client.
 */
struct p_client_context
{
	rdpContext _context;

	proxyData* pdata;

	RdpeiClientContext* rdpei;
	RdpgfxClientContext* gfx;
	DispClientContext* disp;

	/*
	 * In a case when freerdp_connect fails,
	 * Used for NLA fallback feature, to check if the server should close the connection.
	 * When it is set to TRUE, proxy's client knows it shouldn't signal the server thread to closed
	 * the connection when pf_client_post_disconnect is called, because it is trying to connect reconnect without NLA.
	 * It must be set to TRUE before the first try, and to FALSE after the connection fully established,
	 * to ensure graceful shutdown of the connection when it will be closed.
	 */
	BOOL during_connect_process;
};
typedef struct p_client_context pClientContext;

/**
 * Holds data common to both sides of a proxy's session.
 */
struct proxy_data
{
	proxyConfig* config;

	pServerContext* ps;
	pClientContext* pc;

	HANDLE connectionClosed;

	connectionInfo* info;
	filters_list* filters;
};

BOOL init_p_server_context(freerdp_peer* client);
rdpContext* p_client_context_create(rdpSettings* clientSettings);
proxyData* proxy_data_new();
BOOL proxy_data_set_connection_info(proxyData* pdata, rdpSettings* ps, rdpSettings* pc);
void proxy_data_free(proxyData* pdata);
void pf_context_copy_settings(rdpSettings* dst, const rdpSettings* src, BOOL is_server);

#endif /* FREERDP_SERVER_PROXY_PFCONTEXT_H */
