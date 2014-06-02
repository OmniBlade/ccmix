/* 
 * File:   mix_db_gmd.h
 * Author: fbsagr
 *
 * Created on May 30, 2014, 4:30 PM
 */

#ifndef MIX_DB_GMD_H
#define	MIX_DB_GMD_H

#include "mix_db_base.h"

#include <fstream>
//#include <vector>
#include <map>

class MixGMD
{
public:
    MixGMD();
    MixLMD(t_game game);
    void readDB(std::fstream &fh);
    void writeDB(std::fstream &fh);
    std::string getName(t_game game, int32_t id);
    bool addName(t_game game, std::string name, std::string desc);
};

#endif	/* MIX_DB_GMD_H */

