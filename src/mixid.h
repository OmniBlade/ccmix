/* 
 * File:   Ccrc.h
 * Author: Olaf van der Spek
 *
 * Created on 29. prosinec 2011, 14:22
 */

#ifndef MIXID_H
#define	MIXID_H

#include <string>

#ifdef _MSC_VER
#include "win32/stdint.h"
#else
#include <stdint.h>
#endif

typedef enum 
{ 
    game_td,
    game_ra,
    game_ts
} t_game;

namespace MixID
{
    int32_t idGen(t_game game, std::string fname);
    std::string idStr(int32_t id);
}

#endif	/* CCRC_H */

