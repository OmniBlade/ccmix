/* 
 * File:   mix_db_lmd.h
 * Author: fbsagr
 *
 * Created on May 21, 2014, 11:57 AM
 */

#ifndef MIX_DB_LMD_H
#define	MIX_DB_LMD_H

#include "mixid.h"
#include <fstream>
//#include <vector>
#include <map>

class MixLMD
{
public:
    MixLMD(t_game game);
    void readDB(std::fstream &fh, uint32_t offset, uint32_t size);
    void writeDB(std::fstream &fh);
    std::string getName(int32_t id);
    bool addName(std::string name);
    bool deleteName(std::string name);
    t_game getGame() { return m_game_type; }
    uint32_t getSize() { return m_size; }
    std::string getDBName() { return "local mix database.dat"; }
    
private:
    typedef std::map<int32_t, std::string> t_id_map;
    typedef std::pair<int32_t, std::string> t_id_pair;
    typedef std::map<int32_t, std::string>::const_iterator t_id_iter;
    static const char m_xcc_id[32];
    t_id_map m_name_map;
    uint32_t m_size;
    t_game m_game_type;
    int32_t m_id;
};

#endif	/* MIX_DB_LMD_H */

