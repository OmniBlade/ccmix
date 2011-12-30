/* 
 * File:   mix_dexoder.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 11:31
 */

#ifndef MIX_DEXODER_H
#define	MIX_DEXODER_H

#include <cstring>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;



void get_blowfish_key(const byte* s, byte* d);


#endif	/* MIX_DEXODER_H */

