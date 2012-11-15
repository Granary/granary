/*
 * pp.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_PP_H_
#define Granary_PP_H_

#define FORCE_INLINE __attribute__((always_inline))
#define GRANARY

#define ALIGN_TO(lval, const_align) ((const_align) - ((lval) % (const_align)))

#endif /* Granary_PP_H_ */
