/* 
 * File:   mix_db_lmd.h
 * Author: fbsagr
 *
 * Created on May 21, 2014, 11:57 AM
 */

#ifndef MIX_DB_LMD_H
#define	MIX_DB_LMD_H

#include "mix_db_base.h"

#include <fstream>
//#include <vector>
#include <map>

class MixLMD : public BaseMixDB
{
public:
    MixLMD(t_game game);
    void readDB(std::fstream &fh, uint32_t offset, uint32_t size);
    void writeDB(std::fstream &fh);
    std::string getName(int32_t id);
    bool addName(std::string name);
    
private:
    typedef std::map<int32_t, std::string> t_id_map;
    typedef std::pair<int32_t, std::string> t_id_pair;
    typedef std::map<int32_t, std::string>::const_iterator t_id_iter;
    const char m_xcc_id[]; // = "XCC by Olaf van der Spek\x1a\x04\x17\x27\x10\x19\x80";
    //const std::string m_lmd_name; // = "local mix database.dat";
    t_id_map m_name_map;
    uint32_t m_size;
};

#endif	/* MIX_DB_LMD_H */

