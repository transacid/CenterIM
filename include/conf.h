#ifndef __KONST_CONF_H__
#define __KONST_CONF_H__

#if defined(__sgi) && !defined(__GNUC__) && (_COMPILER_VERSION >= 721) && defined(_NAMESPACES)
#define __KTOOL_USE_NAMESPACES
#endif

#if defined(__COMO__)
#define __KTOOL_USE_NAMESPACES
#endif

#ifdef __GNUC__
#include <_G_config.h>
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8)
#else
#define __KTOOL_USE_NAMESPACES
#endif
#endif

#ifdef __KTOOL_USE_NAMESPACES

#define __KTOOL_BEGIN_NAMESPACE         namespace ktool {
#define __KTOOL_END_NAMESPACE           }

#else

#define __KTOOL_BEGIN_NAMESPACE
#define __KTOOL_END_NAMESPACE

#endif

#define __KTOOL_BEGIN_C                 extern "C" {
#define __KTOOL_END_C                   }

#endif
