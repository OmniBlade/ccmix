#include "mix_header.h"
#include "CBlowfish.h"
#include "mix_dexoder.h"

#include <iostream>
#include <vector>

MixHeader::MixHeader(t_game game) :
mix_checksum(0x00010000),
mix_encrypted(0x00020000)
{
    m_game_type = game;
    m_file_count = 0;
    m_body_size = 0;
    m_header_flags = 0;
    m_has_checksum = 0;
    m_is_encrypted = 0;
    //header will always be at bare min 6 bytes
    if(game) {
        m_header_size = 10;
    } else {
        m_header_size = 6;
    }
}

bool MixHeader::readHeader(std::fstream &fh)
{
    fh.seekg(0, std::ios::beg);
    //read first 6 bytes, determine if we have td mix or not.
    char flagbuff[6];
    fh.read(flagbuff, 6);
    
    //std::cout << MixID::idStr(flagbuff, 6) << " Retrieved from header." << std::endl;
    
    if(*reinterpret_cast<uint16_t*>(flagbuff)) {
        if(m_game_type){
            m_game_type = game_td;
            std::cout << "Header indicates mix is of type \"game_td\" and will"
                      << " be handled as such." << std::endl;
        }
        
        m_file_count = *reinterpret_cast<uint16_t*>(flagbuff);
        m_body_size = *reinterpret_cast<uint32_t*>(flagbuff + 2);
        
    } else {
        if(!m_game_type){
            std::cout << "Error, game type is td but header is new format." << std::endl;
            return false;
        }
        //lets work out what kind of header we have
        m_header_flags = *reinterpret_cast<int32_t*>(flagbuff);
        m_has_checksum = m_header_flags & mix_checksum;
        m_is_encrypted = m_header_flags & mix_encrypted;
        
        if(m_is_encrypted){
            return readEncrypted(fh);
        } else {
            return readUnEncrypted(fh);
        }
    }
    return false;
}

bool MixHeader::readUnEncrypted(std::fstream &fh)
{
    t_mix_entry entry;
    std::pair<t_mix_index_iter,bool> rv;
    
    //game_td == 0 get read pointer to correct location
    if(!m_game_type) { 
        fh.seekg(6, std::ios::beg); 
    } else {
        fh.seekg(4, std::ios::beg);
        fh.read(reinterpret_cast<char*>(&m_file_count), sizeof(uint16_t));
        fh.read(reinterpret_cast<char*>(&m_body_size), sizeof(uint32_t));
    }
    
    m_header_size += m_file_count * 12;
    
    //read entries
    for (uint16_t i = 0; i < m_file_count; i++) {
        fh.read(reinterpret_cast<char*>(&entry.first), sizeof(int32_t));
        fh.read(reinterpret_cast<char*>(&entry.second), sizeof(t_index_info));
        rv = m_index.insert(entry);
        if(!rv.second) {
            std::cout << "Error reading header, duplicate ID" << std::endl;
            return false;
        }
    }
    return true;
}

bool MixHeader::readEncrypted(std::fstream& fh)
{
    t_mix_entry entry;
    std::pair<t_mix_index_iter,bool> rv;
    
    Cblowfish blfish;
    int block_count;
    char pblkbuf[8];
    
    //header is at least 84 bytes long at this point due to key source
    m_header_size = 84;
    
    //read keysource to obtain blowfish key
    //fh.seekg(4, std::ios::beg);
    readKeySource(fh);
    blfish.set_key(reinterpret_cast<uint8_t*>(m_key), 56);
    
    //read first block to get file count, needed to calculate header size
    fh.read(pblkbuf, 8);
    blfish.decipher(reinterpret_cast<void*>(pblkbuf), 
                    reinterpret_cast<void*>(pblkbuf), 8);
    memcpy(reinterpret_cast<char*>(&m_file_count), pblkbuf, 2);
    memcpy(reinterpret_cast<char*>(&m_body_size), pblkbuf + 2 , 4);
    
    //workout size of our header and how much we need to decrypt
    //take into account 2 bytes left from getting the file count
    block_count = ((m_file_count * 12) - 2) / 8;
    if (((m_file_count * 12) - 2) % 8) block_count++;
    //add 8 to compensate for block we already decrypted
    m_header_size += block_count * 8 + 8;
    
    //prepare our buffer and copy in first 2 bytes we got from first block
    //index_buffer.resize(block_count * 8 + 2);
    char pindbuf[block_count * 8 + 2];
    memcpy(pindbuf, pblkbuf + 6 , 2);
    
    //loop to decrypt index into index buffer
    for(int i = 0; i < block_count; i++) {
        fh.read(pblkbuf, 8);
        blfish.decipher(reinterpret_cast<void*>(pblkbuf), 
                    reinterpret_cast<void*>(pblkbuf), 8);
        memcpy(pindbuf + 2 + 8 * i, pblkbuf, 8);
    }
    
    //read entries
    for (uint16_t i = 0; i < m_file_count; i++) {
        memcpy(reinterpret_cast<char*>(&entry.first), pindbuf + i * 12,
               sizeof(int32_t));
        memcpy(reinterpret_cast<char*>(&entry.second), pindbuf + 4 + i * 12,
               sizeof(t_index_info));
        rv = m_index.insert(entry);
        if(!rv.second) {
            std::cout << "Error reading header, duplicate ID" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool MixHeader::readKeySource(std::fstream& fh)
{
    if(!fh.is_open()) return false;
    
    fh.seekg(0, std::ios::beg);
    //read first 4 bytes, determine if we have keysource mix or not.
    char flagbuff[4];
    fh.read(flagbuff, 4);
    
    if(!*reinterpret_cast<int32_t*>(flagbuff) & mix_encrypted) {
        std::cout << "key_source not suitable." << std::endl;
        return false;
    }
    
    fh.read(m_keysource, 80);
    get_blowfish_key(reinterpret_cast<uint8_t*>(m_keysource),
                     reinterpret_cast<uint8_t*>(m_key));
    return true;
}

bool MixHeader::writeHeader(std::fstream& fh)
{   
    //make sure we are at the start of the file stream we were passed
    fh.seekg(0, std::ios::beg);
    if(m_is_encrypted){
        return writeEncrypted(fh);
    } else {
        return writeUnEncrypted(fh);
    }
}

bool MixHeader::writeUnEncrypted(std::fstream& fh)
{
    //write flags if supported by game format
    if(m_game_type) fh.write(reinterpret_cast<char*>(&m_header_flags), 4);
    
    //write header info
    fh.write(reinterpret_cast<char*>(&m_file_count), 2);
    fh.write(reinterpret_cast<char*>(&m_body_size), 4);
    
    //write index, first is id, second is struct of offset and size
    for(t_mix_index_iter it = m_index.begin(); it != m_index.end(); ++it) {
        fh.write(reinterpret_cast<const char*>(&(it->first)), 4);
        fh.write(reinterpret_cast<const char*>(&(it->second)), 8);
    }
    return true;
}

bool MixHeader::writeEncrypted(std::fstream& fh)
{
    Cblowfish blfish;
    //std::vector<char> block_buff(8);
    //std::vector<char> index_buffer(8);
    int block_count;
    int offset = 0;
    //char* pindbuf = &index_buffer[0];
    //char* pblkbuf = &block_buff[0];
    char pblkbuf[8];
    
    //always write flags if encrypted.
    fh.write(reinterpret_cast<char*>(&m_header_flags), 4);
    //encrypted file needs its key_source
    fh.write(m_keysource, 80);
    
    //work out how much data needs encrypting, set vector size
    block_count = ((m_file_count * 12) + 6) / 8;
    if(((m_file_count * 12) + 6) % 8) block_count++;
    //index_buffer.resize(block_count * 8);
    char pindbuf[block_count * 8];
    
    //fill the buffer
    memcpy(pindbuf + offset, reinterpret_cast<char*>(&m_file_count), 2);
    offset += 2;
    memcpy(pindbuf + offset, reinterpret_cast<char*>(&m_body_size), 4);
    offset += 4;
    
    for(t_mix_index_iter it = m_index.begin(); it != m_index.end(); ++it) {
        memcpy(pindbuf + offset, reinterpret_cast<const char*>(&(it->first)), 4);
        offset += 4;
        memcpy(pindbuf + offset, reinterpret_cast<const char*>(&(it->second)), 8);
        offset += 8;
    }
    
    //prepare blowfish
    blfish.set_key(reinterpret_cast<const uint8_t*>(m_key), 56);
    
    //encrypt and write to file.
    offset = 0;
    while(block_count--){
        memcpy(pblkbuf, pindbuf + offset, 8);
        blfish.encipher(pblkbuf, pblkbuf, 8);
        fh.write(pblkbuf, 8);
        offset += 8;
    }
    
    return true;
}

bool MixHeader::addEntry(int32_t id, uint32_t size)
{
    t_index_info info;
    t_mix_entry entry;
    std::pair<t_mix_index_iter,bool> rv;
    
    std::cout << "Adding entry " << std::hex << id << std::dec << " with size " << size << std::endl;
    
    info.offset = m_body_size;
    info.size = size;
    
    std::cout << "Entry has offset: " << info.offset << " and size " << info.size << std::endl;
    
    entry.first = id;
    entry.second = info;
    
    rv = m_index.insert(entry);
    if(!rv.second) {
        std::cout << "Error entry not added, duplicate ID of " << entry.first << std::endl;
        return false;
    }
    m_body_size += size;
    m_header_size += 12;
    m_file_count += 1;
    return true;
}

bool MixHeader::removeEntry(int32_t id, bool adjust = false)
{
    t_mix_index_iter old = m_index.find(id);
    
    if(old == m_index.end()) {
        std::cout << "Element not found" << std::endl;
        return false;
    }
    
    //if we adjust for compact, reduce offsets > removed to be - removed size
    if(adjust) {
        for(t_mix_index_iter it = m_index.begin(); it != m_index.end(); ++it) {
            if(it->second.offset > old->second.offset) {
                it->second.offset -= old->second.size;
            }
        }
    }
    
    m_body_size -= old->second.size;
    m_header_size -= 12;
    m_file_count -= 1;
    m_index.erase(old);
    return true;
}

//Would there be a use for this?
/*void MixHeader::setGame(t_game game)
{
    t_game old = m_game_type
    m_game_type = game;
    
    //have we gone from new type header to old?
    if(old & !m_game_type) {
        clearIsEncrypted();
        clearHasChecksum();
        m_header_size -= 4;
    } else if (!old & m_game_type) {
        m_header_size += 4;
    }
    
}*/

t_index_info MixHeader::getEntry(int32_t id)
{
    t_index_info rv;
    rv.offset = 0;
    rv.size = 0;
    
    t_mix_index_iter info = m_index.find(id);
    
    if(info != m_index.end()) rv = info->second;
    
    return rv;
}

void MixHeader::setHasChecksum()
{
    if(m_game_type) {
        m_has_checksum = true;
        m_header_flags |= mix_checksum;
    }
}

void MixHeader::clearHasChecksum()
{
    if(m_game_type) {
        m_has_checksum = false;
        m_header_flags &= ~(mix_checksum);
    }
}

void MixHeader::setIsEncrypted()
{
    if(m_game_type) {
        uint32_t block_count = ((m_file_count * 12) + 6) / 8;
        if(((m_file_count * 12) + 6) % 8) block_count++;
        m_header_size = 84 + block_count * 8;
        m_is_encrypted = true;
        m_header_flags |= mix_encrypted;
    }
}

void MixHeader::clearIsEncrypted()
{
    if(m_game_type) {
        m_header_size = 10 + m_file_count * 12;
        m_is_encrypted = false;
        m_header_flags &= ~(mix_encrypted);
    }
}
