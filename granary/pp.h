/*
 * pp.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_PP_H_
#define Granary_PP_H_

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#   if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
#       define FORCE_INLINE inline
#   else
#       define FORCE_INLINE __attribute__((always_inline))
#   endif
#elif defined(__clang__)
#   define FORCE_INLINE __attribute__((always_inline))
#else
#   define FORCE_INLINE inline
#endif

#define GRANARY

#define ALIGN_TO(lval, const_align) ((const_align) - ((lval) % (const_align)))

#endif /* Granary_PP_H_ */
