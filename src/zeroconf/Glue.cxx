/*
 * Copyright 2003-2021 The Music Player Daemon Project
 * http://www.musicpd.org
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "Glue.hxx"
#include "config/Data.hxx"
#include "config/Option.hxx"
#include "Listen.hxx"
#include "util/Domain.hxx"
#include "Log.hxx"
#include "util/Compiler.h"

#ifdef HAVE_AVAHI
#include "avahi/Helper.hxx"
#endif

#ifdef HAVE_BONJOUR
#include "Bonjour.hxx"
#endif

#include <climits>

#include <string.h>
#include <unistd.h>

#ifndef HOST_NAME_MAX
/* HOST_NAME_MAX is not a portable macro; it is undefined on some
   systems */
#define HOST_NAME_MAX 255
#endif

static constexpr Domain zeroconf_domain("zeroconf");

/* The default service name to publish
 * (overridden by 'zeroconf_name' config parameter)
 */
#define SERVICE_NAME		"Music Player @ %h"

#define DEFAULT_ZEROCONF_ENABLED 1

static int zeroconfEnabled;

#ifdef HAVE_AVAHI
static std::unique_ptr<AvahiHelper> avahi_helper;
#endif

#ifdef HAVE_BONJOUR
static std::unique_ptr<BonjourHelper> bonjour_helper;
#endif

void
ZeroconfInit(const ConfigData &config, [[maybe_unused]] EventLoop &loop)
{
	const char *serviceName;

	zeroconfEnabled = config.GetBool(ConfigOption::ZEROCONF_ENABLED,
					 DEFAULT_ZEROCONF_ENABLED);
	if (!zeroconfEnabled)
		return;

	if (listen_port <= 0) {
		LogWarning(zeroconf_domain,
			   "No global port, disabling zeroconf");
		zeroconfEnabled = false;
		return;
	}

	serviceName = config.GetString(ConfigOption::ZEROCONF_NAME,
				       SERVICE_NAME);

	/* replace "%h" with the host name */
	const char *h = strstr(serviceName, "%h");
	std::string buffer;
	if (h != nullptr) {
		char hostname[HOST_NAME_MAX+1];
		if (gethostname(hostname, HOST_NAME_MAX) == 0) {
			buffer = serviceName;
			buffer.replace(h - serviceName, 2, hostname);
			serviceName = buffer.c_str();
		}
	}

#ifdef HAVE_AVAHI
	avahi_helper = AvahiInit(loop, serviceName, listen_port);
#endif

#ifdef HAVE_BONJOUR
	bonjour_helper = BonjourInit(loop, serviceName, listen_port);
#endif
}

void
ZeroconfDeinit() noexcept
{
#ifdef HAVE_AVAHI
	avahi_helper.reset();
#endif

#ifdef HAVE_BONJOUR
	bonjour_helper.reset();
#endif
}