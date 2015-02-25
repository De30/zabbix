/*
** Zabbix
** Copyright (C) 2001-2015 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "common.h"
#include "db.h"
#include "log.h"
#include "proxy.h"

#include "proxyhosts.h"

/******************************************************************************
 *                                                                            *
 * Function: recv_host_availability                                           *
 *                                                                            *
 * Purpose: update hosts availability, monitored by proxies                   *
 *                                                                            *
 * Author: Alexander Vladishev                                                *
 *                                                                            *
 ******************************************************************************/
void	recv_host_availability(zbx_sock_t *sock, struct zbx_json_parse *jp)
{
	const char	*__function_name = "recv_host_availability";

	zbx_uint64_t	proxy_hostid;
	char		host[HOST_HOST_LEN_MAX], *error = NULL;
	int		ret;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	if (SUCCEED != (ret = get_active_proxy_id(jp, &proxy_hostid, host, &error)))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot parse host availability data from active proxy at \"%s\": %s",
				get_ip_by_socket(sock), error);
		goto out;
	}

	process_host_availability(jp);
out:
	zbx_send_response(sock, ret, error, CONFIG_TIMEOUT);

	zbx_free(error);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: send_host_availability                                           *
 *                                                                            *
 * Purpose: send hosts availability data from proxy                           *
 *                                                                            *
 * Author: Alexander Vladishev                                                *
 *                                                                            *
 ******************************************************************************/
void	send_host_availability(zbx_sock_t *sock)
{
	const char	*__function_name = "send_host_availability";

	struct zbx_json	j;
	char		*info = NULL, *error = NULL, *msg = NULL;
	int		ret = SUCCEED;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

	zbx_json_init(&j, ZBX_JSON_STAT_BUF_LEN);

	get_host_availability_data(&j);

	zabbix_log(LOG_LEVEL_DEBUG, "%s() [%s]", __function_name, j.buffer);

	if (SUCCEED != (ret = zbx_tcp_send_to(sock, j.buffer, CONFIG_TIMEOUT)))
	{
		msg = zbx_dsprintf(msg, "%s", zbx_tcp_strerror());
		goto out;
	}

	if (SUCCEED != (ret = zbx_recv_response(sock, &info, CONFIG_TIMEOUT, &error)))
		msg = zbx_dsprintf(msg, "%s; info: %s", ZBX_NULL2EMPTY_STR(error), ZBX_NULL2EMPTY_STR(info));
out:
	if (SUCCEED != ret)
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot send host availability data to server at \"%s\": %s",
				get_ip_by_socket(sock), msg);
	}

	zbx_json_free(&j);
	zbx_free(info);
	zbx_free(error);
	zbx_free(msg);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}
