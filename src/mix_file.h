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
#include <map>
//#include "MixData.h"

#ifdef _MSC_VER
#include "win32/stdint.h"
#else
#include <stdint.h>
#endif



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
		uint16_t c_files;
		uint32_t size;
	};
	uint32_t flags;
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
    int32_t id;                // id, used to identify the file instead of a normal name
    uint32_t offset;                     // offset from start of body
    uint32_t size;                       // size of this internal file
};

typedef enum 
{ 
    game_td,
    game_ra,
    game_ts
} t_game;

const int32_t mix_checksum = 0x00010000;
const int32_t mix_encrypted = 0x00020000;

/**
 * @brief Mix Databases.
 * 
 * Global Mix Database holds mappings between file id's and their names as well
 * as additional details about the file such as what it represents in game. The 
 * file consists of several sections that contain a little endian uint32_t count
 * of how many entries are in the DB followed by 0 separated strings of 
 * alternating filenames and descriptions. The ID's themselves are generated on
 * loading database.
 * 
 * Local Mix Databases hold just filenames but have a more complicated header.
 * It consists first of a 32 byte string, the xcc_id which appears to be a kind
 * of digital signature for the XCC tools author.
 * 
 * 
 */
struct t_id_data {
    std::string name;
    std::string description;
};

const char xcc_id[] = "XCC by Olaf van der Spek\x1a\x04\x17\x27\x10\x19\x80";

const std::string lmd_name = "local mix database.dat"; 

typedef std::map<int32_t, t_id_data> t_id_datamap;
typedef std::pair<int32_t, t_id_data> t_id_datapair;

/**
 * @brief mix file controller
 * 
 * Some parts of code and code are taken from XCC mix file specification.
 * @sa TS mix file format specification (http://xhp.xwis.net/documents/MIX_Format.html)
 */
class MixFile {
public:
    MixFile(const std::string gmd = "global mix database.dat" , 
            t_game openGame = game_td);
    //MixFile(const MixFile& orig);
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
    bool extractFile(int32_t fileID, std::string outPath);
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
     * @brief Creates a new mix file
     * @param fileName name and path of mix to create
     * @param infiles vector containing files to add to the new mix
     * @param game game the mix should be compatible with
     * @param in_dir location we should create the new mix at
     * @param with_lmd should we generate a local mix database for this mix
     * @param encrypted should we encrypt the header of this mix
     * @param checksum should we generate a checksum for this mix
     * @param key_src string of the path to a key_source to use in encryption
     * @return true if creation is successful
     */
    bool createMix(std::string fileName, std::string in_dir, 
                   t_game game = game_td, bool with_lmd = false, 
                   bool encrypted = false, bool checksum = false, 
                   std::string key_src = "");
    /**
     * @brief adds a sha1 checksum to the end of the file and flags it in the
     *        header.
     * @return true if successful
     */
    bool addCheckSum();
    /**
     * @brief checks, if file is present in the archive
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
     * @brief mix archive header
     * 
     * Prints information about the mix file:
     * file CRC (hex) || file offset (dec) || file size (dec)
     */
    void printInfo();
    /**
     * @brief count CRC ID from filename
     * @param game t_game game selection
     * @param name filename
     * @return  CRC ID of file
     */
    static int32_t getID(t_game game, std::string name);
    /**
     * @brief save file in decrypted format
     * @param outPath output filename
     * @return true if successful
     */
    bool decrypt(std::string outPath);
    std::vector<std::string> getFileNames();
    std::vector<t_mix_index_entry> getFileIndex(){ return files; };
    /**
     * @brief close mix file 
     * 
     * Prepare for opening another file.
     */
    void close();
protected:
    bool readIndex();
    bool readEncryptedIndex();
    bool readFileNames();
    bool extractAllFast(std::string outPath = ".");
    void readLocalMixDb(uint32_t offset, uint32_t size);
    void readGlobalMixDb(std::string filePath);
    bool writeHeader(int16_t c_files, uint32_t flags = 0);
    bool writeEncryptedHeader(int16_t c_files, uint32_t flags = 0);
    bool writeLmd();
    bool writeCheckSum();
    uint32_t lmdSize();
    static bool compareId(const t_mix_index_entry &a, const t_mix_index_entry &b);
    static bool compareTdName(const std::string &a, const std::string &b);
    static bool compareTsName(const std::string &a, const std::string &b);
    t_mix_header mix_head; // mix file header
    std::vector<t_mix_index_entry> files; // list of file headers
    std::vector<std::string> filenames; // file names
    bool m_is_encrypted;
    bool m_has_checksum;
    bool m_has_neoheader;
    bool m_has_lmd;
    int32_t dataoffset;
    std::fstream fh; // file handler
    char key_source[80];
    char key[56];
    char decrypt_buffer[8]; // begining of next index read at the end of last block
    int32_t decrypt_size; // size of valid buffer data
    t_id_datamap name_map;
    t_game mixGame;
    uint8_t m_checksum[20];
};

#endif	/* MIX_FILE_H */
