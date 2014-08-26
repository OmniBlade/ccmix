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
//#include <cctype>

#ifdef _MSC_VER
#include "win32/dirent.h"
#else
#include <dirent.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

#ifdef _WIN32

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
    gmdfile.open(gmd.c_str(), fstream::in | fstream::binary);
    if(gmdfile.is_open()){
        m_global_db.readDB(gmdfile);
        gmdfile.close();
    } else {
        cout << "Could not open global mix database.dat" << endl;
    }
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
    string fname;
            
    for(t_mix_index_iter it = m_header.getBegin(); it != m_header.getEnd(); it++) {
        
        fname = m_local_db.getName(it->first);
        
        if(fname.substr(0, 4) == "[id]") {
            fname = m_global_db.getName(m_header.getGame(), it->first);
        }
        
        extractFile(it->first, outPath + DIR_SEPARATOR + fname);
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
            
            //check if we have an ID containing file name, if not add to localdb
            if(!MixID::isIdName(string(dirp->d_name))){
                if(!m_local_db.addName(string(dirp->d_name))) {
                    cout << "Skipping " << dirp->d_name << ", ID Collision" << endl;
                    continue;
                }
            }
            
            //try adding entry to header, skip if failed
            stat((in_dir + DIR_SEPARATOR + dirp->d_name).c_str(), &st);
            
            if(!m_header.addEntry(MixID::idGen(m_header.getGame(), 
                                  string(dirp->d_name)), 
                                  st.st_size)) {
                continue;
            }
            
            //finally add the filename to the list of files we will add
            filenames.push_back(string(dirp->d_name));
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
    fstream ifh;
    fstream ofh;
    t_index_info lmd;
    std::vector<std::string> filenames; // file names
    //int location;
    
    //get filename without path info
    string basename = baseName(name);
    
    cout << "Trying to add " << basename << endl;
    
    //save the old data offset from header before we started changing it.
    uint32_t old_offset = m_header.getHeaderSize();
    uint32_t old_size = old_offset + m_header.getBodySize();
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
        cout << "We think we need to re-add an lmd" << endl;
        m_header.addEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()),
                          m_local_db.getSize());
    }
    
    //open a temp file
    ofh.open("~ccmix.tmp", ios::binary|ios::out);
    if(!ofh.is_open()){
        cout << "Couldn't open a temporary file to buffer the changes" << endl;
        return false;
    }
    
    //write our new header
    m_header.writeHeader(ofh);
    
    cout << "We think we have an lmd offset of " << lmd.offset << endl;
    
    //copy the body of the old mix, skipping the old lmd
    fh.seekg(old_offset, ios::beg);
    cout << "lmd offset and old offset = " << lmd.offset + old_offset << endl;
    while(fh.tellg() < lmd.offset + old_offset){
        ofh.put(fh.get());
    }
    
    if(lmd.size) fh.seekg(lmd.size + fh.tellg());
    
    cout << "after lmd seek get pos is " << fh.tellg() << endl;
    
    while(fh.tellg() < old_size){
        ofh.put(fh.get());
    }
    
    //open the file to add, if we can't open, bail and delete temp file
    ifh.open(name.c_str(), ios::binary|ios::in);
    if(!ifh.is_open()){
        cout << "Failed to open file to add" << endl;
        remove("~ccmix.tmp");
        return false;
    }
    
    //add new file to mix body
    while(ifh.tellg() < st.st_size){
        ofh.put(ifh.get());
    }
    
    ifh.close();
    
    //write lmd if needed
    if(lmd.size){
        m_local_db.writeDB(ofh);
    }
    
    //write checksum to end of file if required.
    if(m_header.getHasChecksum()){
        writeCheckSum(ofh, 0);
    }
    
    //TODO add logic to move new file over the old file
    overWriteOld("~ccmix.tmp");
    
    return true;
}

bool MixFile::removeFile(std::string name)
{
    return removeFile(MixID::idGen(m_header.getGame(), baseName(name)));
}

bool MixFile::removeFile(int32_t id)
{
    t_index_info lmd;
    t_index_info rem;
    fstream ofh;
    
    //empty the skip map
    m_skip.clear();
    
    //save the old data offset from header before we started changing it.
    uint32_t old_offset = m_header.getHeaderSize();
    uint32_t old_size = old_offset + m_header.getBodySize();
    
    //set up to skip copying the lmd if the mix contains one
    lmd = m_header.getEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()));
    rem = m_header.getEntry(id);
    
    if(!rem.size) return false;
            
    //add skip entry for lmd if we have one and remove from header
    if(lmd.size) {
        m_skip[lmd.offset] = lmd.size;
        m_header.removeEntry(MixID::idGen(m_header.getGame(), 
                             m_local_db.getDBName()), true);
    }
    
    //add our file to the skip map and remove it
    m_skip[rem.offset] = rem.size;
    m_header.removeEntry(id, true);
    m_local_db.deleteName(id);
    
    //re-add our lmd entry if we had one will recalc its position
    if(lmd.size) {
        m_header.addEntry(MixID::idGen(m_header.getGame(), m_local_db.getDBName()),
                          m_local_db.getSize());
    }
    
    //open a temp file
    ofh.open("~ccmix.tmp", ios::binary|ios::out);
    if(!ofh.is_open()){
        cout << "Couldn't open a temporary file to buffer the changes" << endl;
        return false;
    }
    
    //write our new header
    m_header.writeHeader(ofh);
    
    //copy the body of the old mix, skipping files in m_skip
    fh.seekg(old_offset, ios::beg);
    
    for(t_skip_map_iter it = m_skip.begin(); it != m_skip.end(); it++) {
        while(fh.tellg() != it->first + old_offset){
            ofh.put(fh.get());
        }
        fh.seekp(old_offset + it->first + it->second);
    }
    
    while(fh.tellg() < old_size){
        ofh.put(fh.get());
    }
    
    //write lmd if needed
    if(lmd.size){
        m_local_db.writeDB(ofh);
    }
    
    //write checksum to end of file if required. will have to overwrite old one
    if(m_header.getHasChecksum()){
        writeCheckSum(ofh, -20);
    }
    
    overWriteOld("~ccmix.tmp");
    
    return true;
}

bool MixFile::addCheckSum()
{
    //check if we think this file is checksummed already
    if(m_header.getHasChecksum()){
        cout << "File is already flagged as having a checksum" << endl;
        return false;
    }
    
    //toggle flag for checksum and then write it
    m_header.setHasChecksum();
    fh.seekp(0, ios::beg);
    m_header.writeHeader(fh);
    
    //write the actual checksum
    writeCheckSum(fh);
    
    return true;
}

bool MixFile::writeCheckSum(fstream &fh, int32_t pos) 
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
    
    //write checksum, pos is position from end to start at.
    fh.seekp(pos, ios::end);
    fh.write(reinterpret_cast<const char*>(hash), 20);
    
    delete[] buffer;
    free(hash);
    
    return false;
}
    
void MixFile::printFileList() 
{
    string fname;
    t_mix_index_iter it = m_header.getBegin();
    while(it != m_header.getEnd()){
        //try to get a filename, if lmd doesn't have it try gmd.
        fname = m_local_db.getName(it->first);
        if(fname.substr(0, 4) == "[id]") {
            fname = m_global_db.getName(m_header.getGame(), it->first);
        }
        
        cout << setw(24) << fname << setw(10) << MixID::idStr(it->first) <<
                setw(12) << it->second.offset << setw(12) << 
                it->second.size << endl;
        it++;
    }
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
    if(m_header.getIsEncrypted()){
        cout << "The mix has an encrypted header.\n\nThe blowfish key is " <<
                MixID::idStr(m_header.getKey(), 56) << "\n" << endl;
        cout << "The key was recovered from the following key source:\n" <<
                MixID::idStr(m_header.getKeySource(), 80) << "\n" <<
                endl;
    }
    if(m_header.getHasChecksum()){
        cout << "The mix has a SHA1 checksum:\nSHA1: " << 
                MixID::idStr(reinterpret_cast<char*>(m_checksum), 20) << "\n" << endl;
    }
}

bool MixFile::decrypt()
{
    fstream ofh;
    
    //are we already decrypted?
    if (!m_header.getIsEncrypted()) return false;
    
    //get some info on original file and then set header to decrypted
    uint32_t dataoffset = m_header.getHeaderSize();
    m_header.clearIsEncrypted();
    
    ofh.open("~ccmix.tmp", fstream::out | fstream::binary | fstream::trunc);
    m_header.writeHeader(ofh);
    
    fh.seekg(dataoffset);
    
    while(!fh.eof()){
        ofh.put(fh.get());
    }
    
    ofh.close();
    
    overWriteOld("~ccmix.tmp");

    return true;
}

bool MixFile::encrypt()
{
    fstream ofh;
    
    //are we already encrypted?
    if (m_header.getIsEncrypted()) return false;
    
    //get some info on original file and then set header to decrypted
    uint32_t dataoffset = m_header.getHeaderSize();
    m_header.setIsEncrypted();
    
    ofh.open("~ccmix.tmp", fstream::out | fstream::binary | fstream::trunc);
    m_header.writeHeader(ofh);
    
    fh.seekg(dataoffset);
    
    while(!fh.eof()){
        ofh.put(fh.get());
    }
    
    ofh.close();
    
    //overWriteOld("~ccmix.tmp");
    
    return true;
}

bool MixFile::overWriteOld(std::string temp)
{
    fstream ifh;
    string newname = m_file_path;
    
    remove(m_file_path.c_str());
    rename(temp.c_str(), m_file_path.c_str());
    
    return true;
}

void MixFile::close()
{
    fh.close();   
}
