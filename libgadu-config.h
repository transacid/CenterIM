#ifndef __GG_LIBGADU_CONFIG_H
#define __GG_LIBGADU_CONFIG_H

#ifdef WORDS_BIGENDIAN
#define __GG_LIBGADU_BIGENDIAN
#endif

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
#define __GG_LIBGADU_HAVE_OPENSSL
#endif

#include <stdint.h>

#endif
