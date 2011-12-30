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



/**
 * @brief header
 */
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


/**
 * @brief mix file controller
 * 
 * Some parts of code and code are taken from XCC mix file specification.
 * @sa TS mix file format specification (http://xhp.xwis.net/documents/MIX_Format.html)
 */
class MixFile {
public:
    MixFile();
    MixFile(const MixFile& orig);
    virtual ~MixFile();
    /**
     * @brief open mix archive
     * @param path mix file path
     * @retval true file opened
     * @retval false file not found
     */
    bool open(const std::string path);
    /**
     * @brief extract file from mix archive
     * @param fileID CRC ID of file
     * @param outPath extracted file path
     * @retval true file extracted
     * @retval false file not present in the archive 
     */
    bool extractFile(unsigned int fileID, std::string outPath);
    /**
     * @brief extract file from mix archive
     * @param fileName name of file
     * @param outPath extracted file path
     * @retval true file extracted
     * @retval false file not present in the archive 
     */
    bool extractFile(std::string fileName, std::string outPath);
    /**
     * @brief chcecks, if file is present in the archive
     * @param fname file name
     * @return true if present
     */
    bool checkFileName(std::string fname);
    /**
     * @brief prints mix archive header
     * 
     * Flags not implemented yet. Prints header in following format:
     * file CRC (hex) || file offset (dec) || file size (dec)
     * @param flags print settings (not implemented yet)
     */
    void printFileList(int flags);
    /**
     * @brief count CRC ID from filename
     * @param game t_game game selection
     * @param name filename
     * @return  CRC ID of file
     */
    unsigned int get_id(t_game game, std::string name);
private:
    void get_files();
    void readIndex(unsigned int * mixdb_offset, unsigned int * mixdb_size);
    t_mix_header mix_head; // mix file header
    std::vector<t_mix_index_entry> files; // list of file headers
    std::vector<std::string> filenames; // file names
    bool m_is_encrypted;
    bool m_has_checksum;
    bool has_local_mixdb;
    MixData * mixdb; // local mix database.dat
    MixData * globaldb; // global filenames database
    int dataoffset;
    std::ifstream fh; // file handler
    char key_source[80];
    char key[56];
    char decrypt_buffer[8]; // begining of next index read at the end of last block
    int decrypt_size; // size of valid buffer data
};

#endif	/* MIX_FILE_H */

