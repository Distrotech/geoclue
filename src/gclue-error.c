/* vim: set et ts=8 sw=8: */
/* gclue-error.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Bastien Nocera <hadess@hadess.net>
 */

#include "gclue-error.h"

/**
 * SECTION:gclue-error
 * @short_description: Error helper functions
 * @include: gclue-glib/gclue-glib.h
 *
 * Contains helper functions for reporting errors to the user.
 **/

/**
 * gclue_error_quark:
 *
 * Gets the gclue-glib error quark.
 *
 * Return value: a #GQuark.
 **/
GQuark
gclue_error_quark (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("gclue_error");

	return quark;
}

