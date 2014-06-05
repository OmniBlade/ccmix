/* 
 * File:   mix_db_gmd.h
 * Author: fbsagr
 *
 * Created on May 30, 2014, 4:30 PM
 */

#ifndef MIX_DB_GMD_H
#define	MIX_DB_GMD_H

#include "mix_db_gamedb.h"

#include <vector>
#include <fstream>
#include <map>

// Callers job to ensure file handles are valid and in usable state.
class MixGMD
{
public:
    MixGMD();
    void readDB(std::fstream &fh);
    void writeDB(std::fstream &fh);
    std::string getName(t_game game, int32_t id);
    bool addName(t_game game, std::string name, std::string desc);
    bool deleteName(t_game game, std::string name);
private:
    MixGameDB m_td_list : game_td;
    MixGameDB m_ra_list : game_ra;
    MixGameDB m_ts_list : game_ts;
    MixGameDB m_ra2_list : game_ra2;
    std::vector<MixGameDB*> m_db_array(6);
};

#endif	/* MIX_DB_GMD_H */
