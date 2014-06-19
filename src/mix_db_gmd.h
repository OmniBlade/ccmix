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
    MixGameDB m_td_list;
    MixGameDB m_ra_list;
    MixGameDB m_ts_list;
    MixGameDB m_ra2_list;
    std::vector<MixGameDB*> m_db_array;
};

#endif	/* MIX_DB_GMD_H */
