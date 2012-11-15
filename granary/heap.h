/*
 * heap.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_HEAP_H_
#define Granary_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/// DynamoRIO-compatible heap allocation functions

void *heap_alloc(void *, unsigned long long);
void heap_free(void *, void *, unsigned long long);

#ifdef __cplusplus
}
#endif


#endif /* Granary_HEAP_H_ */
