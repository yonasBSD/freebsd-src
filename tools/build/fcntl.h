/*-
 * Copyright (c) 2016 Jilles Tjoelker <jilles@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LEGACY_SYS_FCNTL_H_
#define	_LEGACY_SYS_FCNTL_H_

#include_next <fcntl.h>

/*
 * On FreeBSD fcntl.h indirectly brings in cdefs.h. On Linux with musl, it does
 * not. Unconditionally inlude it here since there's no harm in including it
 * multiple times since we use __BEGIN_DECLS and __END_DECLS from it below.
 */
#include <sys/cdefs.h>

struct spacectl_range {
	off_t	r_offset;
	off_t	r_len;
};

#define SPACECTL_DEALLOC	1

#define SPACECTL_F_SUPPORTED	0

__BEGIN_DECLS
int	fspacectl(int, int, const struct spacectl_range *, int,
	    struct spacectl_range *);
__END_DECLS

#endif /* !_LEGACY_SYS_STAT_H_ */
