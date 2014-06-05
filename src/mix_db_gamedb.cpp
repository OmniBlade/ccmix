#include "mix_db_gamedb.h"
#include "mixid.h"
#include <iostream>

using namespace std;

MixGameDB::MixGameDB(t_game game)
{
    m_game_type = game;
    m_size = 0;
}

void MixGameDB::readDB(const char* data)
{
    m_entries = *reinterpret_cast<const uint32_t*>(data);
    m_size += 4;
    
    //get count of entries
    cout << "Count for gameDB entries is " << m_entries << endl;
    
    cout << "Game for this DB read is " << m_game_type << endl;
    
    //retrieve each entry into the struct as a string then push to the map.
    //relies on string constructor reading to 0;
    //local mix db doesn't have descriptions.
    uint32_t count = m_entries;
    t_id_data id_data;
    while (count--) {
        std::pair<t_id_iter,bool> rv;
        
        //data is incremented and read twice, once for filename, once for desc.
        id_data.name = data;
        data += id_data.name.length() + 1;
        id_data.description = data;
        data += id_data.description.length() + 1;
        
        //attempt to insert data and figure out if we had a collision.
        rv = m_name_map.insert(t_id_pair(MixID::idGen(m_game_type,
                        id_data), id_data));
        if(rv.second) {
            m_size += id_data.name.length() + 1;
            m_size += id_data.description.length() + 1;
        } else {
            cout << id_data << " generates an ID conflict with existing entry " << 
                    rv.first->second.name;
        }
    }
}

void MixGameDB::writeDB(std::fstream& fh)
{
    //first record how many entries we have for this db.
    fh.write(m_entries, sizeof(uint32_t));
    
    //filenames
    for(t_id_iter it = m_name_map.begin(); it != m_name_map.end(); ++it) {
        fh.write(it->second.name.c_str(), it->second.name.size() + 1);
        fh.write(it->second.description.c_str(), it->second.description.size() + 1);
    }
}

std::string MixGameDB::getName(int32_t id)
{
    t_id_iter rv = m_name_map.find(id);
    
    if(rv != m_name_map.end()){
        return rv->second.name;
    }
    
    return "<unknown>" << MixID::idStr(id);
}

bool MixGameDB::addName(std::string name, std::string description)
{
    t_id_data id_data;
    id_data.name = name;
    id_data.description = description;
    
    std::pair<t_id_iter,bool> rv;
    rv = m_name_map.insert(t_id_pair(MixID::idGen(m_game_type,
                    name), id_data));
    if(rv.second) {
        m_size += name.length() + 1;
        m_size += description.length() + 1;
        return true;
    } else {
        cout << name << " generates an ID conflict with existing entry " << 
                rv.first->second.name << endl;
        return false;
    }
    return false;
}

bool MixGameDB::deleteName(std::string name)
{
    cout << "deleteName not implemented yet" << endl;
    return false;
}