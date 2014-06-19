#include "mix_db_gmd.h"

MixGMD::MixGMD() :
m_td_list(game_td),
m_ra_list(game_ra),
m_ts_list(game_ts),
m_ra2_list(game_ra2),
m_db_array(6)
{
    m_db_array.push_back(&m_td_list);
    m_db_array.push_back(&m_ra_list);
    m_db_array.push_back(&m_ts_list);
    m_db_array.push_back(0);
    m_db_array.push_back(0);
    m_db_array.push_back(&m_ra2_list);
}

void MixGMD::readDB(std::fstream &fh)
{
    uint32_t begin, end, size, offset;
    
    // get file size
    fh.seekg(0, std::ios::beg);
    begin = fh.tellg();
    fh.seekg(0, std::ios::end);
    end = fh.tellg();
    size = end - begin;
    offset = 0;
    
    //read file into data buffer
    char* data = new char[size];

    fh.seekg(0, std::ios::beg);
    fh.read(data, size);
    
    // read file from buffer into respective dbs
    for (int i = 0; i < 6; i++){
        if (!m_db_array[i]) continue;
        m_db_array[i]->readDB(data, offset);
        offset += m_db_array[i]->getSize();
    }
}

void MixGMD::writeDB(std::fstream& fh)
{
    for (int i = 0; i < 6; i++){
        if (!m_db_array[i]) continue;
        m_db_array[i]->writeDB(fh);
    }
}

std::string MixGMD::getName(t_game game, int32_t id)
{
    switch(game){
        case game_td:
            return m_td_list.getName(id);
        case game_ra:
            return m_ra_list.getName(id);
        case game_ts:
            return m_ts_list.getName(id);
        case game_ra2:
            return m_ra2_list.getName(id);
        default:
            return "";
    }
}

bool MixGMD::addName(t_game game, std::string name, std::string desc = "")
{
    switch(game){
        case game_td:
            return m_td_list.addName(name, desc);
        case game_ra:
            return m_ra_list.addName(name, desc);
        case game_ts:
            return m_ts_list.addName(name, desc);
        case game_ra2:
            return m_ra2_list.addName(name, desc);
        default:
            return false;
    }
}

bool MixGMD::deleteName(t_game game, std::string name)
{
    switch(game){
        case game_td:
            return m_td_list.deleteName(name);
        case game_ra:
            return m_ra_list.deleteName(name);
        case game_ts:
            return m_ts_list.deleteName(name);
        case game_ra2:
            return m_ra2_list.deleteName(name);
        default:
            return false;
    }
}
