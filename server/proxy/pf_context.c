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

#include "pf_client.h"
#include "pf_context.h"
#include "pf_common.h"

/* Proxy context initialization callback */
static BOOL client_to_proxy_context_new(freerdp_peer* client,
                                        pServerContext* context)
{
	context->vcm = WTSOpenServerA((LPSTR) client->context);

	if (!context->vcm || context->vcm == INVALID_HANDLE_VALUE)
		goto fail_open_server;

	return TRUE;
fail_open_server:
	context->vcm = NULL;
	return FALSE;
}

/* Proxy context free callback */
static void client_to_proxy_context_free(freerdp_peer* client,
        pServerContext* context)
{
	WINPR_UNUSED(client);

	if (!context)
		return;

	WTSCloseServer((HANDLE) context->vcm);

	if (context->dynvcReady)
	{
		CloseHandle(context->dynvcReady);
		context->dynvcReady = NULL;
	}
}

BOOL init_p_server_context(freerdp_peer* client)
{
	client->ContextSize = sizeof(pServerContext);
	client->ContextNew = (psPeerContextNew) client_to_proxy_context_new;
	client->ContextFree = (psPeerContextFree) client_to_proxy_context_free;
	return freerdp_peer_context_new(client);
}

void pf_context_copy_settings(rdpSettings* dst, const rdpSettings* src, BOOL is_server)
{
	rdpSettings* before_copy = freerdp_settings_clone(dst);
	if (!before_copy)
		return;

	if (!freerdp_settings_copy(dst, src))
	{
		freerdp_settings_free(before_copy);
		return;
	}

	free(dst->ConfigPath);
	free(dst->PrivateKeyContent);
	free(dst->RdpKeyContent);
	free(dst->RdpKeyFile);
	free(dst->PrivateKeyFile);
	free(dst->CertificateFile);
	free(dst->CertificateName);
	free(dst->CertificateContent);
	free(dst->ClientRandom);

	/* adjust pointer to instance pointer */
	dst->ServerMode = is_server;
	dst->ConfigPath = _strdup(before_copy->ConfigPath);
	dst->PrivateKeyContent = _strdup(before_copy->PrivateKeyContent);
	dst->RdpKeyContent = _strdup(before_copy->RdpKeyContent);
	dst->RdpKeyFile = _strdup(before_copy->RdpKeyFile);
	dst->PrivateKeyFile = _strdup(before_copy->PrivateKeyFile);
	dst->CertificateFile = _strdup(before_copy->CertificateFile);
	dst->CertificateName = _strdup(before_copy->CertificateName);
	dst->CertificateContent = _strdup(before_copy->CertificateContent);
	dst->ClientRandomLength = before_copy->ClientRandomLength;
	CopyMemory(dst->ClientRandom, before_copy->ClientRandom, before_copy->ClientRandomLength);

	if (is_server)
	{
		free(dst->ServerCertificate);
		dst->ServerCertificateLength = before_copy->ServerCertificateLength;
	}
	else
	{
		/* adjust instance pointer for client's context */
		dst->instance = before_copy->instance;

		/* RdpServerRsaKey must be set to NULL if `dst` is client's context */
		dst->RdpServerRsaKey = NULL;
	}

	freerdp_settings_free(before_copy);
}

rdpContext* p_client_context_create(rdpSettings* clientSettings)
{
	RDP_CLIENT_ENTRY_POINTS clientEntryPoints;
	rdpContext* context;
	RdpClientEntry(&clientEntryPoints);
	context = freerdp_client_context_new(&clientEntryPoints);

	if (!context)
		return NULL;

	pf_context_copy_settings(context->settings, clientSettings, FALSE);

	if (!context->settings)
		goto error;

	return context;
error:
	freerdp_client_context_free(context);
	return NULL;
}

static void connection_info_free(connectionInfo* info)
{
	free(info->TargetHostname);
	free(info->ClientHostname);
	free(info->Username);
	free(info);
}

proxyData* proxy_data_new()
{
	proxyData* pdata = calloc(1, sizeof(proxyData));

	if (pdata == NULL)
	{
		return NULL;
	}

	pdata->info = calloc(1, sizeof(connectionInfo));

	if (pdata->info == NULL)
	{
		free(pdata);
		return NULL;
	}

	if (!(pdata->connectionClosed = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		proxy_data_free(pdata);
		return NULL;
	}

	return pdata;
}

/* sets connection info values using the settings of both server & client */
BOOL proxy_data_set_connection_info(proxyData* pdata, rdpSettings* ps, rdpSettings* pc)
{
	if (!(pdata->info->TargetHostname = _strdup(pc->ServerHostname)))
		goto out_fail;

	if (!(pdata->info->Username = _strdup(pc->Username)))
		goto out_fail;

	if (!(pdata->info->ClientHostname = _strdup(ps->ClientHostname)))
		goto out_fail;

	return TRUE;
out_fail:
	proxy_data_free(pdata);
	return FALSE;
}

void proxy_data_free(proxyData* pdata)
{
	connection_info_free(pdata->info);
	if (pdata->connectionClosed)
	{
		CloseHandle(pdata->connectionClosed);
		pdata->connectionClosed = NULL;
	}

	free(pdata);
}
