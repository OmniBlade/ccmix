/* 
 * File:   Ccrc.h
 * Author: Olaf van der Spek
 *
 * Created on 29. prosinec 2011, 14:22
 */

#ifndef CCRC_H
#define	CCRC_H

#ifdef _MSC_VER
#include "win32/stdint.h"
#else
#include <stdint.h>
#endif

class Ccrc
{
public:
    void do_block(const void* data, int size);

    void init()
    {
        m_crc = 0;
    }

    uint32_t get_crc() const
    {
        return m_crc;
    }
private:
    uint32_t m_crc;
};

#endif	/* CCRC_H */

