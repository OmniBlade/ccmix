/* 
 * File:   mix_file.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 11:32
 */

#ifndef MIX_FILE_H
#define	MIX_FILE_H
#include <string>
#include <fstream>
#include <vector>
#include "MixData.h"




union t_mix_header
{
	struct
	{
		unsigned short c_files;
		unsigned int size;
	};
	unsigned int flags;
};


struct t_mix_index_entry
{
    unsigned int id;                // id, used to identify the file instead of a normal name
    unsigned int offset;                     // offset from start of body
    unsigned int size;                       // size of this internal file
};

typedef enum 
{ 
    game_ts,
    game_ra,
    game_td
} t_game;

const int mix_checksum = 0x00010000;
const int mix_encrypted = 0x00020000;



class mix_file {
public:
    mix_file();
    mix_file(const mix_file& orig);
    virtual ~mix_file();
    
    bool open(const std::string path);
    bool extractFile(unsigned int fileID, std::string outPath);
    bool extractFile(std::string fileName, std::string outPath);
    bool checkFileName(std::string fname);
    void printFileList(int flags);
    unsigned int get_id(t_game game, std::string name);
private:
    void get_files();
    void readIndex(unsigned int * mixdb_offset, unsigned int * mixdb_size);
    t_mix_header mix_head;
    std::vector<t_mix_index_entry> files;
    bool m_is_encrypted;
    bool m_has_checksum;
    MixData * mixdb;
    int dataoffset;
    std::ifstream fh; // file handler
    char key_source[80];
    char key[56];
    char decrypt_buffer[8]; // begining of next index read at the end of last block
    int decrypt_size; // size of valid buffer data
};

#endif	/* MIX_FILE_H */

