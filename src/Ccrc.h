/* 
 * File:   Ccrc.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 14:22
 */

#ifndef CCRC_H
#define	CCRC_H

class Ccrc
{
public:
    void do_block(const void* data, int size);

    void init()
    {
        m_crc = 0;
    }

    int get_crc() const
    {
        return m_crc;
    }
private:
    unsigned int m_crc;
};

#endif	/* CCRC_H */

