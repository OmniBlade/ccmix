#include "mix_header.h"

#include <iostream>

MixHeader::MixHeader(t_game game)
{
    m_game_type = game;
    m_file_count = 0;
    m_body_size = 0;
    m_header_flags = 0;
    m_has_checksum = 0;
    m_is_encrypted = 0;
    //header will always be at bare min 6 bytes
    m_header_size = 6;
}

bool MixHeader::readHeader(std::fstream &fh)
{
    fh.seekg(0, std::ios::beg);
    //read first 6 bytes, determine if we have td mix or not.
    char* flagbuff = new char[6];
    fh.read(flagbuff, 6);
    
    if(*reinterpret_cast<uint16_t*>(flagbuff)) {
        if(m_game_type != game_td){
            m_game_type = game_td;
            std::cout << "Header indicates mix is of type \"game_td\" and will"
                      << " be handled as such." << std::endl;
            
        }
        
        m_file_count = *reinterpret_cast<uint16_t*>(flagbuff);
        m_body_size = *reinterpret_cast<uint32_t*>(flagbuff + 2);
        
    } else {
        
        //header is now at least 10 bytes
        m_header_size += 4;
        //lets work out what kind of header we have
        m_has_checksum = *reinterpret_cast<int32_t*>(flagbuff) & mix_checksum;
        m_is_encrypted = *reinterpret_cast<int32_t*>(flagbuff) & mix_encrypted;
        
        if(m_is_encrypted){
            return readEncrypted(fh);
        } else {
            return readUnEncrypted(fh);
        }
    }
    
    delete[] flagbuff;
}
