/* 
 * File:   mix_file.cpp
 * Author: ivosh-l
 * 
 * Created on 29. prosinec 2011, 11:32
 */

#include "sha1.h"
#include "mix_file.h"
#include <iostream>
//#include "mixid.h"
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

#ifdef WINDOWS

#define DIR_SEPARATOR '\\'

#else

#define DIR_SEPARATOR '/'

#endif

string MixFile::baseName(string const& pathname)
{
    string rv = pathname;
    const size_t last_slash_idx = rv.find_last_of("\\/");
    if (string::npos != last_slash_idx)
    {
        rv.erase(0, last_slash_idx + 1);
    }
    
    return rv;
}

MixFile::MixFile(const string gmd, t_game game) :
m_header(game),
m_global_db(),
m_local_db(game)
{
    fstream gmdfile;
    gmdfile.open(gmd.c_str(), std::ios::binary);
    m_global_db.readDB(gmdfile);
    gmdfile.close();
}

MixFile::~MixFile()
{
    close();
}

bool MixFile::open(const string path)
{
    t_index_info lmd;
    m_file_path = path;
    
    if (fh.is_open())
        close();
    
    fh.open(path.c_str(), fstream::in | fstream::out | fstream::binary);
    if (!fh.is_open()) {
        cout << "File " << path << " failed to open" << endl;
        return false;
    }

    if(!m_header.readHeader(fh)) {
        return false;
    }
    
    if(m_header.getHasChecksum()) {
        fh.seekg(-20, ios::end);
        fh.read(reinterpret_cast<char*>(m_checksum), 20);
    }
    
    //check if we have a local mix db
    lmd = m_header.getEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()));
    cout << "LMD Stats are Offset: " << lmd.offset << " Size: " << lmd.size << endl;
    if(lmd.size){
        m_local_db.readDB(fh, lmd.offset + m_header.getHeaderSize(), lmd.size);
    }
    
    return true;
}

bool MixFile::checkFileName(string name) 
{
    t_index_info rv = m_header.getEntry(MixID::idGen(m_header.getGame(), name));
    //size of a valid name will be > 0 hence true;
    return rv.size != 0;
}

bool MixFile::extractAll(string outPath) 
{
    fstream ofile;
    for(t_mix_index_iter it = m_header.getBegin(); it != m_header.getEnd(); it++) {
        extractFile(it->first, 
                    outPath + DIR_SEPARATOR + m_local_db.getName(it->first));
    }
    return true;
}

bool MixFile::extractFile(int32_t id, string out) 
{
    ofstream of;
    char * buffer;
    t_index_info entry;

    // find file index entry
    entry = m_header.getEntry(id);
    
    if(entry.size) {
        buffer = new char[entry.size];
        fh.seekg(m_header.getHeaderSize() + entry.offset);
        fh.read(buffer, entry.size);

        of.open(out.c_str(), ios_base::binary);
        of.write(buffer, entry.size);

        of.close();
        delete[] buffer;

        return true;
    }
    
    return false;
}

bool MixFile::extractFile(string filename, string outpath) 
{
    return extractFile(MixID::idGen(m_header.getGame(), filename), outpath);
}

bool MixFile::createMix(string fileName, string in_dir, 
                        bool with_lmd, bool encrypted, bool checksum, 
                        string key_src) 
{
    
    DIR* dp;
    struct dirent *dirp;
    struct stat st;
    //t_index_info finfo;
    //use mix_head.size uint32_t offset = 0;
    fstream ifile;
    //int32_t file_id;
    std::vector<std::string> filenames; // file names
    
    if(encrypted) m_header.setIsEncrypted();
    if(checksum) m_header.setHasChecksum();
    
    cout << "Game we are building for is " << m_header.getGame() << endl;
    if(m_header.getIsEncrypted()){
        cout << "We are going to try to encrypt" << endl;
    }
    
    //make sure we can open the directory
    if((dp = opendir(in_dir.c_str())) == NULL) {
        cout << "Error opening " << in_dir << endl;
        return false;
    }
    
    //iterate through entries in directory, ignoring directories
    while ((dirp = readdir(dp)) != NULL) {
        stat((in_dir + DIR_SEPARATOR + dirp->d_name).c_str(), &st);
        if(!S_ISDIR(st.st_mode)){
            if(!m_local_db.addName(string(dirp->d_name))) {
                cout << "Skipping " << dirp->d_name << ", ID Collision" << endl;
                continue;
            }
            
            filenames.push_back(string(dirp->d_name));
            stat((in_dir + DIR_SEPARATOR + dirp->d_name).c_str(), &st);
            m_header.addEntry(MixID::idGen(m_header.getGame(), string(dirp->d_name)), 
                              st.st_size);
        }
    }
    closedir(dp);
    
    //are we wanting lmd? if so push header info for it here
    if(with_lmd){
        filenames.push_back(m_local_db.getDBName());
        m_header.addEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()), 
                          m_local_db.getSize());
    }
    
    cout << m_header.getBodySize() << " files, total size " << m_header.getFileCount() << 
            " before writing header" << endl;
    
    //if we are encrypted, get a key source
    if(m_header.getIsEncrypted()){
        ifile.open(key_src.c_str(), ios::binary|ios::in);
        if(ifile.is_open()){
            m_header.readKeySource(ifile);
        } else {
            cout << "Could not open a key_source, encryption disabled" << endl;
            m_header.clearIsEncrypted();
        }
        ifile.close();
    }
    
    //time to start writing our new file
    fh.open(fileName.c_str(), fstream::in | fstream::out | fstream::binary | 
            fstream::trunc);
    
    if(!fh.is_open()){
        cout << "Failed to create empty file" << endl;
        return false;
    }
    
    //write a header
    if(!m_header.writeHeader(fh)) {
        cout << "Failed to write header." << endl;
        return false;
    }
    
    cout << "Writing the body now" << endl;
    
    cout << "Writing " << filenames.size() << " files." << endl;
    
    //write the body, offset is set in header based on when added so should match
    for(unsigned int i = 0; i < filenames.size(); i++){
        char c;
        //last entry is lmd if with_lmd so break for special handling
        if(with_lmd && i == filenames.size() - 1) break;
        
        cout << in_dir + DIR_SEPARATOR + filenames[i] << endl;
        ifile.open((in_dir + DIR_SEPARATOR + filenames[i]).c_str(), 
                    fstream::in | fstream::binary);
        if(!ifile.is_open()) {
            cout << "Could not open input file " << filenames[i] << endl;
            return false;
        }
        while(ifile.get(c)) {
            fh.write(&c, 1);
        }
        ifile.close();
    }
    
    //handle lmd writing here.
    if(with_lmd){
        m_local_db.writeDB(fh);
    }
    
    //just write the checksum, flag is set further up
    if(m_header.getHasChecksum()){
        writeCheckSum(fh);
    }
    
    cout << "Offset to data is " << m_header.getHeaderSize() << endl;
    
    fh.close();
    
    return true;
}

bool MixFile::addFile(string name)
{
    struct stat st;
    //use mix_head.size uint32_t offset = 0;
    fstream ifile;
    fstream ofile;
    t_index_info lmd;
    std::vector<std::string> filenames; // file names
    //int location;
    
    //get filename without path info
    string basename = baseName(name);
    
    //save the old data offset from header before we started changing it.
    uint32_t old_offset = m_header.getHeaderSize();
    //uint32_t old_end = old_offset + m_header.getBodySize();
    
    //set up to skip copying the lmd if the mix contains one
    lmd = m_header.getEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()));
    
    m_header.removeEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()),
                         true);
    
    //stat file to add and check its not a directory
    stat(name.c_str(), &st);
    if(!S_ISDIR(st.st_mode)){
        if(!m_local_db.addName(basename)) {
                cout << "Cannot add " << basename << ", ID Collision" << endl;
                return false;
        }
        filenames.push_back(basename);
        m_header.addEntry(MixID::idGen(m_header.getGame(), basename), 
                          st.st_size);
    } else {
        cout << "Cannot add directory as a file" << endl;
        return false;
    }
    
    //if the lmd had a size before (thus existed), add it back to header now
    if(lmd.size) {
        m_header.addEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()),
                          m_local_db.getSize());
    }
    
    //open a temp file
    ofile.open("~ccmix.tmp", ios::binary|ios::out);
    if(!ofile.is_open()){
        cout << "Couldn't open a temporary file to buffer the changes" << endl;
        return false;
    }
    
    //write our new header
    m_header.writeHeader(ofile);
    
    //copy the body of the old mix, skipping the old lmd
    fh.seekg(old_offset, ios::beg);
    while(fh.tellg() != lmd.offset + old_offset){
        ofile.put(fh.get());
    }
    //location =;
    fh.seekg(lmd.size + fh.tellg());
    while(!fh.eof()){
        ofile.put(fh.get());
    }
    
    //seek back length of checksum if we had one
    if(m_header.getHasChecksum()) ofile.seekp(-20, ios::end);
    
    //open the file to add, if we can't open, bail and delete temp file
    ifile.open(name.c_str(), ios::binary|ios::in);
    if(!ifile.is_open()){
        cout << "Failed to open file to add" << endl;
        remove("~ccmix.tmp");
        return false;
    }
    
    //add new file to mix body
    while(!ifile.eof()){
        ofile.put(ifile.get());
    }
    
    ifile.close();
    
    //write lmd if needed
    if(lmd.size){
        m_local_db.writeDB(ofile);
    }
    
    //write checksum to end of file if required.
    if(m_header.getHasChecksum()){
        writeCheckSum(ofile);
    }
    
    //TODO add logic to move new file over the old file
    overWriteOld("~ccmix.tmp");
    
    return true;
}

bool MixFile::addCheckSum()
{
    //check if we think this file is checksummed already
    if(m_header.getHasChecksum()){
        cout << "File is already flagged as having a checksum" << endl;
        return true;
    }
    
    //toggle flag for checksum and then write it
    m_header.setHasChecksum();
    fh.seekp(0, ios::beg);
    m_header.writeHeader(fh);
    
    //write the actual checksum
    writeCheckSum(fh);
    
    return true;
}

bool MixFile::writeCheckSum(fstream &fh) 
{
    SHA1 sha1;
    const size_t BufferSize = 144*7*1024; 
    char* buffer = new char[BufferSize];
    //int blocks = mix_head.size / BufferSize;
    //int rem = mix_head.size % BufferSize;
    uint8_t* hash;
    ofstream testout;
    
    //read data into sha1 algo from dataoffset
    fh.seekg(m_header.getHeaderSize(), ios::beg);
    
    while(!fh.eof()) {
        fh.read(buffer, BufferSize);
        std::size_t numBytesRead = size_t(fh.gcount());
        sha1.addBytes(buffer, numBytesRead);
    }
    
    //clear stream
    fh.clear();
    
    // get our hash and print it to console as well
    hash = sha1.getDigest();
    cout << "Checksum is "
    << MixID::idStr(reinterpret_cast<char*>(hash), 20);
    cout << endl;
    
    //write checksum
    fh.seekp(0, ios::end);
    fh.write(reinterpret_cast<const char*>(hash), 20);
    
    delete[] buffer;
    free(hash);
    
    return false;
}
    
string MixFile::printFileList() 
{
    return "Not Implemented.";
}

void MixFile::printInfo()
{
    if(m_header.getGame()){
        cout << "This mix is a new style mix that supports header encryption"
                " and checksums.\nRed Alert onwards can read this type of mix"
                " but the ID's used differ between Red Alert and later games.\n"
                << endl;
    } else {
        cout << "This mix is an old style mix that doesn't support header"
                " encryption or\nchecksums.\nTiberian Dawn and Sole Survivor"
                " use this format exclusively and Red Alert can\nuse them as well"
                ".\n" << endl;
    }
    cout << "It contains " << m_header.getFileCount() << " files"
            " which take up " << m_header.getBodySize() << " bytes\n" << endl;
    /*if(m_header.getIsEncrypted()){
        cout << "The mix has an encrypted header.\n\nThe blowfish key is " <<
                SHA1::hexPrinter(reinterpret_cast<uint8_t*>(m_header.getKey()), 56) << "\n" << endl;
        cout << "The key was recovered from the following key source:\n" <<
                SHA1::hexPrinter(reinterpret_cast<uint8_t*>(m_header.getKeySource()), 80) << "\n" <<
                endl;
    }
    if(m_header.getHasChecksum()){
        cout << "The mix has a SHA1 checksum:\nSHA1: " << 
                SHA1::hexPrinter(m_checksum, 20) << "\n" << endl;
    }*/
}

bool MixFile::decrypt()
{
    fstream ofh;
    
    //are we already decrypted?
    if (!m_header.getIsEncrypted()) return false;
    
    //get some info on original file and then set header to decrypted
    uint32_t dataoffset = m_header.getHeaderSize();
    m_header.clearIsEncrypted();
    
    ofh.open("~ccmix.tmp", ios::binary);
    m_header.writeHeader(ofh);
    
    fh.seekg(dataoffset);
    
    while(!fh.eof()){
        ofh.put(fh.get());
    }
    
    overWriteOld("~ccmix.tmp");
    
    ofh.close();

    return true;
}

bool MixFile::overWriteOld(std::string temp)
{
    fstream ifh;
    
    ifh.open(temp.c_str(), fstream::in | fstream::binary);
    if(!ifh.is_open()) return false;
    
    if(fh.is_open()) close();
        
    fh.open(m_file_path.c_str(), fstream::in | fstream::out |
                                 fstream::binary | fstream::trunc);
    if(!fh.is_open()) return false;
    
    while(!ifh.eof()){
        fh.put(ifh.get());
    }
    return true;
}

void MixFile::close()
{
    fh.close();   
}
