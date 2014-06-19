/* 
 * File:   mix_file.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 11:32
 */

#ifndef MIX_FILE_H
#define	MIX_FILE_H
#include "mix_db_gmd.h"
#include "mix_db_lmd.h"
#include "mix_header.h"
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
    bool extractAll(std::string outPath = ".");
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
    bool createMix(std::string fileName, std::string in_dir, bool with_lmd = false, 
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
     * @param fileName file name
     * @return true if successful
     */
    bool addFile(std::string name);
    /**
     * @brief checks, if file is present in the archive
     * @param fname file name
     * @return true if present
     */
    bool checkFileName(std::string name);
    /**
     * @brief mix archive header
     * 
     * Prints header in following format:
     * file CRC (hex) || file offset (dec) || file size (dec)
     * @param flags print settings
     * @return file text list
     */
    std::string printFileList();
    /**
     * @brief mix archive header
     * 
     * Prints information about the mix file:
     * file CRC (hex) || file offset (dec) || file size (dec)
     */
    void printInfo();
    /**
     * @brief save file in decrypted format
     * @param outPath output filename
     * @return true if successful
     */
    
    /**
     * @brief close mix file 
     * 
     * Prepare for opening another file.
     */
    void close();
protected:
    typedef std::map<uint32_t, uint32_t> t_skip_map;
    typedef std::pair<uint32_t, uint32_t> t_skip_entry;
    typedef std::map<uint32_t, uint32_t>::const_iterator t_skip_map_iter;
    bool writeCheckSum(std::fstream &fh);
    std::string baseName(std::string const& pathname);
    bool decrypt();
    bool overWriteOld(std::string temp);
    MixHeader m_header; // mix file header
    MixGMD m_global_db;
    MixLMD m_local_db;
    t_skip_map m_skip;
    bool m_has_lmd;
    std::string m_file_path;
    std::fstream fh; // file handler
    uint8_t m_checksum[20];
};

#endif	/* MIX_FILE_H */
