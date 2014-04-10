/* 
 * File:   mix_dexoder.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 11:31
 */

#ifndef MIX_DEXODER_H
#define	MIX_DEXODER_H

#include <cstring>

#ifdef _MSC_VER
#include "win32/stdint.h"
#else
#include <stdint.h>
#endif


void get_blowfish_key(const uint8_t* s, uint8_t* d);


#endif	/* MIX_DEXODER_H */

