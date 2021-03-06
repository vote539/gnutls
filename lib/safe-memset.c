/*
 * Copyright (C) 2014 Red Hat
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifdef TEST_SAFE_MEMSET
# include <string.h>
#else
# include <gnutls_int.h>
#endif

/* This is based on a nice trick for safe memset,
 * sent by David Jacobson in the openssl-dev mailing list.
 */

void _gnutls_safe_memset(void *data, int c, size_t size)
{
	volatile unsigned volatile_zero = 0;
	volatile char *vdata = (volatile char*)data;

	do {
		memset(data, c, size);
	} while(vdata[volatile_zero] != c);
}

#ifdef TEST_SAFE_MEMSET
int main()
{
	char x[64];

	safe_memset(x, 0, sizeof(x));

	return 0;

}

#endif
