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
 * @brief Mix archive header.
 * 
 * Mix archive starts with archive header storing information about number of files
 * included and their total size. Some archives (RA/TS) starts with 4b flags, which
 * is followed by header.
 * 
 * @par old mix format header
 *  - 2B - number of files
 *  - 4B - total content's size
 * 
 * @par RA/TS mix format header
 *  - 4B - flags
 *  - 2B - number of files
 *  - 4B - total content's size
 * 
 * @section archivetype determine what type archive is
 * Now here comes part which might be hard to understand. Data are stored in 
 * little endian format, which means little ending is stored first and big ending
 * last. Number 0x11223344, where little ending is 44, will be stored like this
 * 44 33 22 11 - bytes are stored in reverse direction as you can see.
 * 
 * Flags are stored only in LAST 2 bytes, that means if file starts with flags,
 * FIRST 2 bytes are 00 00, and other 2 bytes contains flag. If there are no flags,
 * archive can not start with 00 00, because that would mean it contains no files.
 * 
 * So we read header no matter what type archive is and check for first two bytes,which are stored in c_files. If c_files == 0, we take first 4 bytes, which are
 * stored in flags as flag information, move file pointer to 4th byte and repeat 
 * reading of header (6b). Now c_files contains number of files and size contains
 * size of all files.
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

/**
 * @brief Included file header.
 * 
 * Right after mix file header comes file index. Every included file has it's entry
 * there storing information about it's CRC ID (mix archive doesn't store information
 * about file names, but CRC ID can be calculated from file name), offset in body
 * block and size. Remember that you have to add body offset to file offset before you read
 * a file data.
 * 
 * @par structure of index entry
 * - 4B - CRC id
 * - 4B - file offset
 * - 4B - file size
 * 
 * @par count body offset
 * To get body offset, you have to count size of information before body.
 * - does archive contain flags? (4B if so)
 * - mix archive header (6B) 
 * - file index (c_files * 12B)
 * - is archive encrypted? (80B if so)
 * 
 * 
 */
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
    MixFile(const char *);
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
     * @brief extract all files from the archive
     * @param outPath output directory
     * @param withFileNames try to get file names of the content
     * @return true if extraction successful
     */
    bool extractAll(std::string outPath = ".", bool withFileNames = true);
    /**
     * @brief chcecks, if file is present in the archive
     * @param fname file name
     * @return true if present
     */
    bool checkFileName(std::string fname);
    /**
     * @brief mix archive header
     * 
     * Prints header in following format:
     * file CRC (hex) || file offset (dec) || file size (dec)
     * @param flags print settings
     * @return file text list
     */
    std::string printFileList(int flags);
    /**
     * @brief count CRC ID from filename
     * @param game t_game game selection
     * @param name filename
     * @return  CRC ID of file
     */
    unsigned int getID(t_game game, std::string name);
    std::vector<std::string> getFileNames();
    std::vector<t_mix_index_entry> getFileIndex(){ return files; };
private:
    bool readIndex();
    bool readEncryptedIndex();
    bool readFileNames();
    bool extractAllFast(std::string outPath = ".");
    t_mix_header mix_head; // mix file header
    std::vector<t_mix_index_entry> files; // list of file headers
    std::vector<std::string> filenames; // file names
    bool m_is_encrypted;
    bool m_has_checksum;
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

