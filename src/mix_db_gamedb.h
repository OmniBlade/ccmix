/* 
 * File:   mix_db_gamedb.h
 * Author: fbsagr
 *
 * Created on June 3, 2014, 3:40 PM
 */

#ifndef MIX_DB_GAMEDB_H
#define	MIX_DB_GAMEDB_H

#include "mixid.h"
#include <string>
#include <fstream>
#include <map>

//GameDB databases represent internal databases in global mix database
//each handled game has its own DB internally
class MixGameDB
{
public:
    MixGameDB(t_game game);
    void readDB(const char* data, uint32_t offset);
    void writeDB(std::fstream &fh);
    std::string getName(int32_t id);
    bool addName(std::string name, std::string description);
    bool deleteName(std::string name);
    t_game getGame() { return m_game_type; }
    uint32_t getSize() { return m_size; }
    
private:
    struct t_id_data {
        std::string name;
        std::string description;
    };
    
    typedef std::map<int32_t, t_id_data> t_id_map;
    typedef std::pair<int32_t, t_id_data> t_id_pair;
    typedef std::map<int32_t, t_id_data>::const_iterator t_id_iter;
    
    t_id_map m_name_map;
    uint32_t m_size;
    uint32_t m_entries;
    t_game m_game_type;
};

#endif	/* MIX_DB_GAMEDB_H */

