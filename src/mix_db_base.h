/* 
 * File:   mix_db.h
 * Author: fbsagr
 *
 * Created on May 13, 2014, 10:50 AM
 */

#ifndef MIX_DB_BASE_H
#define	MIX_DB_BASE_H

#include <string>
//#include <fstream>
//#include <vector>
//#include <map>

#ifdef _MSC_VER
#include "win32/stdint.h"
#else
#include <stdint.h>
#endif

/*struct t_id_data {
    std::string name;
    std::string description;
};*/

//typedef std::map<int32_t, t_id_data> t_id_datamap;

//typedef std::pair<int32_t, t_id_data> t_id_datapair;

class BaseMixDB {
protected:
    t_game m_game_type;
    
public:
    BaseMixDB(t_game game)
        : m_game_type(game)
        {   
        }
        
    virtual void readDB() = 0;
    virtual void writeDB() = 0;
    virtual std::string getName() = 0;
    bool addName() = 0;
    
    t_game getGame() { return m_game_type; }
};


#endif	/* MIX_DB_H */

