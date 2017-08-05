#ifndef _ACKNEXT_CONFIG_H_
#define _ACKNEXT_CONFIG_H_

#ifdef _ACKNEXT_INTERNAL_
#define ACKCONST
#else
#define ACKCONST const
#endif

#ifdef __cplusplus
#define ACKFUN extern "C"
#define ACKVAR extern "C"
#else
#define ACKFUN extern
#define ACKVAR extern
#endif

#define RAD_TO_DEG 57.29580406904963
#define DEG_TO_RAD 0.0174533

// Fancy global define saying that we are in Acknext
// and which version do we have
#define __ACKNEXT__
#define ACKNEXT_VERSION_MAJOR 0
#define ACKNEXT_VERSION_MINOR 0

#define ACKNEXT_TEMP_VECTORS 64

#endif // _ACKNEXT_CONFIG_H_