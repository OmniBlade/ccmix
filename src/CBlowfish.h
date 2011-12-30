/* 
 * File:   CBlowfish.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 18:53
 */

#ifndef CBLOWFISH_H
#define	CBLOWFISH_H

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

typedef dword t_bf_p[18];
typedef dword t_bf_s[4][256];

class Cblowfish
{
public:
    void set_key(const byte* key, int cb_key);
    void encipher(dword& xl, dword& xr) const;
    void encipher(const void* s, void* d, int size) const;
    void decipher(dword& xl, dword& xr) const;
    void decipher(const void* s, void* d, int size) const;
private:
    dword S(dword x, int i) const;
    dword bf_f(dword x) const;
    void ROUND(dword& a, dword b, int n) const;

    t_bf_p m_p;
    t_bf_s m_s;
};

#endif	/* CBLOWFISH_H */

