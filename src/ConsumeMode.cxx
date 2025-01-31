/*
 * Copyright 2003-2022 The Music Player Daemon Project
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

#include "ConsumeMode.hxx"
#include "util/Compiler.h"

#include <cassert>
#include <stdexcept>

#include <string.h>

const char *
ConsumeToString(ConsumeMode mode) noexcept
{
	switch (mode) {
	case ConsumeMode::OFF:
		return "0";

	case ConsumeMode::ON:
		return "1";

	case ConsumeMode::ONE_SHOT:
		return "oneshot";
	}

	assert(false);
	gcc_unreachable();
}

ConsumeMode
ConsumeFromString(const char *s)
{
	assert(s != nullptr);

	if (strcmp(s, "0") == 0)
		return ConsumeMode::OFF;
	else if (strcmp(s, "1") == 0)
		return ConsumeMode::ON;
	else if (strcmp(s, "oneshot") == 0)
		return ConsumeMode::ONE_SHOT;
	else
		throw std::invalid_argument("Unrecognized consume mode, expected 0, 1, or oneshot");
}
