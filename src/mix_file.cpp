/* 
 * File:   mix_file.cpp
 * Author: ivosh-l
 * 
 * Created on 29. prosinec 2011, 11:32
 */

#include "mix_file.h"
#include <iostream>
#include "Ccrc.h"
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "CBlowfish.h"
#include "mix_dexoder.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

#ifdef WINDOWS

#define DIR_SEPARATOR '\\'

#else

#define DIR_SEPARATOR '/'

#endif

void t_mix_header_copy(t_mix_header* header, char * data) {
    memcpy((char *) &(header->c_files), data, 2);
    data += 2;
    memcpy((char *) &(header->size), data, 4);
}

//MixFile::MixFile(const char * pDirectory, const string gmdFile) {
MixFile::MixFile(const string gmdFile) {
    this->dataoffset = 0;
    //mixdb = NULL;
    m_has_checksum = false;
    m_is_encrypted = false;
    readGlobalMixDb(gmdFile);
}

MixFile::~MixFile() {
    fh.close();
}

uint32_t MixFile::getID(t_game game, string name) {
    transform(name.begin(), name.end(), name.begin(),
            (int(*)(int)) toupper); // convert to uppercase
    if (game != game_ts) { // for TD and RA
        int i = 0;
        uint32_t id = 0;
        int l = name.length(); // length of the filename
        while (i < l) {
            uint32_t a = 0;
            for (int j = 0; j < 4; j++) {
                a >>= 8;
                if (i < l)
                    a += static_cast<uint32_t> (name[i]) << 24;
                i++;
            }
            id = (id << 1 | id >> 31) + a;
        }
        return id;
    } else { // for TS
        const int l = name.length();
        int a = l >> 2;
        if (l & 3) {
            name += static_cast<char> (l - (a << 2));
            int i = 3 - (l & 3);
            while (i--)
                name += name[a << 2];
        }
        Ccrc crc; // use a normal CRC function
        crc.init();
        crc.do_block(name.c_str(), name.length());
        return crc.get_crc();
    }
}

bool MixFile::open(const string path, t_game openGame) {
    if (fh.is_open())
        close();

    mixGame = openGame;
    
    fh.open(path.c_str(), ios::binary);
    if (fh.rdstate() & ifstream::failbit) {
        return false;
    }

    fh.read((char*) &mix_head, sizeof (mix_head));
    dataoffset = 6;

    if (!mix_head.c_files) {
        dataoffset += 4;
        m_has_checksum = mix_head.flags & mix_checksum;
        m_is_encrypted = mix_head.flags & mix_encrypted;
        if (m_is_encrypted) {

            /* read key_source */
            fh.seekg(4);
            fh.read(key_source, 80);
            get_blowfish_key((uint8_t *) key_source, (uint8_t *) key);
            Cblowfish blfish;
            uint8_t enc_header[8];
            blfish.set_key((uint8_t *) key, 56);

            /* read encrypted header */
            fh.seekg(84);
            fh.read((char*) enc_header, 8);
            blfish.decipher((void *) enc_header, (void *) enc_header, 8);
            t_mix_header_copy(&mix_head, (char *) enc_header);

            /* encrypted block has 8b, but header only 6b => 2b is part of first index entry */
            memcpy(decrypt_buffer, (char*) (&enc_header) + 6, 2);
            decrypt_size = 2;

            readEncryptedIndex();
        } else {
            fh.seekg(4);
            fh.read((char*) &mix_head, 6);
            readIndex();
        }

    } else {
        fh.seekg(6);
        readIndex();
    }
    
    return true;
}

bool MixFile::readIndex() {
    t_mix_index_entry fheader;

    if (!fh.is_open())
        return false;

    if (m_is_encrypted)
        return readEncryptedIndex();


    for (int i = 0; i < mix_head.c_files; i++) {
        fh.read((char*) &fheader, sizeof (fheader));
        dataoffset += sizeof (fheader);
        files.push_back(fheader);

        if (fheader.id == 0x366e051f) { // local mix database.dat
            readLocalMixDb(&fh, fheader.offset + dataoffset, fheader.size);
        }
    }

    return true;

}

bool MixFile::readEncryptedIndex() {
    int indexSize;
    int blockCnt;
    uint8_t encBuff[8];
    Cblowfish blfish;
    char * encIndex;
    t_mix_index_entry fheader;

    if (!fh.is_open()) return false;

    dataoffset += 80;

    indexSize = mix_head.c_files * 12;
    blockCnt = (indexSize - decrypt_size) / 8;

    if ((indexSize - decrypt_size) % 8) blockCnt++;

    encIndex = new char[blockCnt * 8 + decrypt_size];
    memcpy(encIndex, decrypt_buffer, decrypt_size);

    blfish.set_key((uint8_t *) key, 56);

    for (int i = 0; i < blockCnt; i++) {
        fh.read((char*) &encBuff, 8);
        blfish.decipher((void *) &encBuff, (void *) &encBuff, 8);
        memcpy(encIndex + decrypt_size + 8 * i, (char *) &encBuff, 8);
    }
    dataoffset += blockCnt * 8 + decrypt_size;

    for (int i = 0; i < mix_head.c_files; i++) {
        memcpy((char *) &fheader, encIndex + i * 12, 12);
        files.push_back(fheader);

        if (fheader.id == 0x366e051f) { // local mix database.dat
            readLocalMixDb(&fh, fheader.offset + dataoffset, fheader.size);
        }

    }

    delete[] encIndex;
    return true;

}

bool MixFile::checkFileName(string fname) {
    transform(fname.begin(), fname.end(), fname.begin(),
            (int(*)(int)) toupper);
    uint32_t fileID = getID(mixGame, fname);
    for (uint32_t i = 0; i < files.size(); i++) {
        if (files[i].id == fileID)
            return true;
    }
    return false;
}

bool MixFile::extractAll(string outPath, bool withFileNames) {

    char buffer[16];

    if (!withFileNames)
        return extractAllFast(outPath);
    
    t_id_datamap::iterator it;
    
    for (int i = 0; i < mix_head.c_files; i++) {
        bool found = false;
        
        it = name_map.find(files[i].id);
        
        if (it != name_map.end()){
            extractFile(files[i].id, outPath + "/" + it->second.name);
            found = true;
        }
        
        if (!found) {
            sprintf(buffer, "%x", files[i].id);
            extractFile(files[i].id, outPath + "/unknown_" + string(buffer));
        }
    }

    return true;
}

bool MixFile::extractAllFast(string outPath) {
    
    char buffer[16];

    for (int i = 0; i < mix_head.c_files; i++) {
        sprintf(buffer, "%x", files[i].id);
        extractFile(files[i].id, outPath + "/unknown_" + string(buffer));
    }
    return true;
}

bool MixFile::extractFile(uint32_t fileID, string outPath) {
    ofstream oFile;
    uint32_t f_offset = 0, f_size = 0;
    char * buffer;
    bool found = false;


    // find file index entry
    for (uint32_t i = 0; i < files.size(); i++) {
        if (files[i].id == fileID) {
            f_offset = files[i].offset;
            f_size = files[i].size;
            found = true;
        }
    }
    if (!found)
        return false;

    buffer = new char[f_size];
    fh.seekg(dataoffset + f_offset);
    fh.read(buffer, f_size);

    oFile.open(outPath.c_str(), ios_base::binary);
    oFile.write(buffer, f_size);

    oFile.close();
    delete[] buffer;

    return true;
}

bool MixFile::extractFile(string fileName, string outPath) {
    return extractFile(getID(mixGame, fileName), outPath);
}

bool MixFile::createMix(string fileName, string in_dir, t_game game, 
                        bool with_lmd, bool encrypted) {
    DIR* dp;
    struct dirent *dirp;
    struct stat st;
    t_mix_index_entry finfo;
    uint32_t offset = 0;
    ofstream ofile;
    ifstream ifile;
    
    //ensure vectors are clear
    filenames.clear();
    files.clear();
    
    //make sure we can open the directory
    if((dp  = opendir(in_dir.c_str())) == NULL) {
        cout << "Error opening " << in_dir << endl;
        return false;
    }
    
    //iterate through entries in directory, ignoring directories
    while ((dirp = readdir(dp)) != NULL) {
        lstat(dirp->d_name, &st);
        if(!S_ISDIR(st.st_mode)){
            filenames.push_back(string(dirp->d_name));
            finfo.id = getID(game, string(dirp->d_name));
            finfo.offset = offset;
            stat((in_dir + DIR_SEPARATOR + dirp->d_name).c_str(), &st);
            finfo.size = st.st_size;
            offset += finfo.size;
            files.push_back(finfo);
        }
    }
    closedir(dp);
    
    //are we wanting lmd? if so start preparing for it here
    if(with_lmd){
        filenames.push_back(lmd_name);
        finfo.id = getID(game, lmd_name);
        finfo.offset = offset;
        finfo.size = lmdSize();
    }
    
    ofile.open(fileName.c_str(), ofstream::binary);
    
    ofile.close();
    
    return true;
}

//calculate the size of an lmd from filenames
uint32_t MixFile::lmdSize() {
    //lmd header is 52 bytes big
    uint32_t rv = 52;
    for (int i = 0; i < filenames.size(); i++){
        rv += filenames[i].size();
    }
    //add number of files to account for null termination
    return rv + filenames.size();
}

vector<string> MixFile::getFileNames() {
    if (filenames.empty())
        readFileNames();

    return filenames;
}

bool MixFile::readFileNames() {

    /* clear previous filenames list */
    filenames.clear();
    
    //create iterator for holding results of searching map
    t_id_datamap::iterator it;
    
    for (int i = 0; i < mix_head.c_files; i++) {
        bool found = false;
        
        cout << files[i].id << endl;
        it = name_map.find(files[i].id);
        
        if (it != name_map.end()){
            filenames.push_back(it->second.name);
            found = true;
        }

        /* file does not match filename in any database */
        if (!found) {
            filenames.push_back("<unknown>");
        }

    }

    return true;
}

string MixFile::printFileList(int flags = 1) {
    stringstream os;

    if (flags & 1) {
        if (filenames.empty())
            readFileNames();

        os << setw(15) << setfill(' ') << "FILENAME |";
    }


    os << setw(12) << setfill(' ') << "ID | " << setw(12) << setfill(' ') << "ADDRESS |" << setw(10) << setfill(' ') << "SIZE" << endl;

    for (uint32_t i = 0; i < files.size(); i++) {
        if (flags & 1) {
            os << setw(13) << setfill(' ') << filenames[i] << " |";
        }
        os << " " << setw(8) << setfill('0') << hex << files[i].id << " | " << setw(10) << setfill(' ') << dec << (files[i].offset + dataoffset) << " | " << setw(10) << setfill(' ') << files[i].size << endl;
    }

    return os.str();
}

bool MixFile::decrypt(string outPath) {
    ofstream ofh;
    char * buff;
    if (!m_is_encrypted)
        return false;

    ofh.open(outPath.c_str(), ios::binary);
    ofh.write((char *) &(mix_head.c_files), 2);
    ofh.write((char *) &(mix_head.size), 4);

    for (unsigned short i = 0; i < mix_head.c_files; i++) {
        ofh.write((char *) &(files[i]), 12);
    }
    buff = new char[mix_head.size];
    fh.seekg(dataoffset);
    fh.read(buff, mix_head.size);
    ofh.write(buff, mix_head.size);

    ofh.close();

    delete[] buff;
    return true;
}

void MixFile::close() {
    fh.close();
    filenames.clear();
    files.clear();
//    delete mixdb;
//    mixdb = NULL;
    dataoffset = 0;
    m_has_checksum = false;
    m_is_encrypted = false;    
}

void MixFile::readLocalMixDb(ifstream * fh, uint32_t offset, uint32_t size)
{
    char* data = new char[size];
    fh->seekg(offset, ios_base::beg);
    fh->read(data, size);
    
    //move pointer past most of header to entry count, total header is 52 bytes.
    data += 48;
    
    //get count of entries
    int32_t count = *reinterpret_cast<const int32_t*>(data);
    data += 4;

    //retrieve each entry into the struct as a string then push to the map.
    //relies on string constructor reading to 0;
    //local mix db doesn't have descriptions.
    t_id_data id_data;
    id_data.description = "";
    while (count--) {
        //get data and move pointer to next entry
        id_data.name = data;
        data += id_data.name.length() + 1;
        name_map.insert(pair<uint32_t,t_id_data>(getID(mixGame,
                        id_data.name), id_data));
    }
}

void MixFile::readGlobalMixDb(string filePath)
{
    ifstream fh;
    uint32_t begin, end, size;
    fh.open(filePath.c_str(), ios::binary);
    if (fh.rdstate() & ifstream::failbit) {
        fh.clear();
        cout << "Unable to load global mix database! (" << filePath << ")" << endl;
    }
    
    /* get file size */
    begin = fh.tellg();
    fh.seekg(0, ios::end);
    end = fh.tellg();
    size = end - begin;
    
    char* data = new char[size];

    fh.seekg(0);
    fh.read(data, size);
    
    //get 1st 4 bytes as count of entries in db
    char* data_end = &data[size - 1];
    int32_t count = *reinterpret_cast<const int32_t*>(data);
    data += 4;
    
    //retrieve each entry into the struct as a string then push to the map.
    //relies on string constructor reading to 0;
    t_id_data id_data;
    while (count--) {
        //get data and move pointer to next entry
        id_data.name = data;
        data += id_data.name.length() + 1;
        id_data.description = data;
        data += id_data.description.length() + 1;
        name_map.insert(pair<uint32_t,t_id_data>(getID(mixGame,
                        id_data.name), id_data));
        //mix db is several databases on top of one another.
        if (count < 1 && data < data_end){
            count = *reinterpret_cast<const int32_t*>(data);
            data += 4;
        }
    }
}