/* 
 * File:   mix_header.h
 * Author: fbsagr
 *
 * Created on June 5, 2014, 4:37 PM
 */

#ifndef MIX_HEADER_H
#define	MIX_HEADER_H

#include "mixid.h"

#include <string>
#include <fstream>
#include <map>

struct t_index_info
{
    uint32_t offset;    // offset from start of body
    uint32_t size;      // size of this internal file
};

typedef std::map<int32_t, t_index_info> t_mix_index;
typedef std::pair<int32_t, t_index_info> t_mix_entry;
typedef std::map<int32_t, t_index_info>::const_iterator t_mix_index_iter;

class MixHeader
{
public:
    MixHeader(t_game game);
    bool readHeader(std::fstream &fh);
    bool writeHeader(std::fstream &fh);
    bool addEntry(int32_t id, uint32_t size);
    bool removeEntry(int32_t id);
    t_mix_entry getEntry(int32_t id);
    void printContents();
    
private:
    const int32_t mix_checksum = 0x00010000;
    const int32_t mix_encrypted = 0x00020000;
    
    t_game m_game_type;
    uint16_t m_file_count;
    uint32_t m_body_size;
    uint32_t m_header_flags;
    uint32_t m_header_size;
    bool m_has_checksum;
    bool m_is_encrypted;
    t_mix_index m_index;
    
    bool readEncrypted(std::fstream &fh);
    bool writeEncrypted(std::fstream &fh);
    bool readUnEncrypted(std::fstream &fh);
    bool writeUnEncrypted(std::fstream &fh);
};

#endif	/* MIX_HEADER_H */
