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
#include "CBlowfish.h"
#include "mix_dexoder.h"

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
    //string gmdFile = pDirectory;
    //char dsprt[2] = {DIR_SEPARATOR, '\0'};
    //gmdFile.append(dsprt);
    //gmdFile.append(gmd);
    this->dataoffset = 0;
    mixdb = NULL;
    m_has_checksum = false;
    m_is_encrypted = false;
    globaldb = new MixData(gmdFile);
}

MixFile::~MixFile() {
    fh.close();
}

uint32_t MixFile::getID(t_game game, string name) {
    transform(name.begin(), name.end(), name.begin(),
            (int(*)(int)) std::toupper); // convert to uppercase
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

bool MixFile::open(const std::string path, t_game openGame) {
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
    int i;
    t_mix_index_entry fheader;

    if (!fh.is_open())
        return false;

    if (m_is_encrypted)
        return readEncryptedIndex();


    for (i = 0; i < mix_head.c_files; i++) {
        fh.read((char*) &fheader, sizeof (fheader));
        dataoffset += sizeof (fheader);
        files.push_back(fheader);

        if (fheader.id == 0x366e051f) { // local mix database.dat
            mixdb = new MixData(&fh, fheader.offset + dataoffset, fheader.size);
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
            mixdb = new MixData(&fh, fheader.offset + dataoffset, fheader.size);
        }

    }

    delete[] encIndex;
    return true;

}

bool MixFile::checkFileName(string fname) {
    transform(fname.begin(), fname.end(), fname.begin(),
            (int(*)(int)) std::toupper);
    uint32_t fileID = getID(mixGame, fname);
    for (uint32_t i = 0; i < files.size(); i++) {
        if (files[i].id == fileID)
            return true;
    }
    return false;
}

bool MixFile::extractAll(std::string outPath, bool withFileNames) {
    int i;

    vector<string> filenamesdb;
    vector<string> filenamesdb_local;

    char buffer[16];

    if (!withFileNames)
        return extractAllFast(outPath);


    // get filenames
    filenamesdb = globaldb->getFileNames();
    if (mixdb)
        filenamesdb_local = mixdb->getFileNames();

    for (i = 0; i < mix_head.c_files; i++) {
        bool found = false;
        for (uint32_t j = 0; j < filenamesdb.size(); j++) {
            if (getID(mixGame, filenamesdb[j]) == files[i].id) {
                extractFile(files[i].id, outPath + "/" + filenamesdb[j]);
                found = true;
                break;
            }
        }
        if (mixdb && !found) {
            for (uint32_t j = 0; j < filenamesdb_local.size(); j++) {
                if (getID(mixGame, filenamesdb_local[j]) == files[i].id) {
                    extractFile(files[i].id, outPath + "/" + filenamesdb_local[j]);
                    found = true;
                    break;
                }

            }
        }
        if (!found) {
            sprintf(buffer, "%x", files[i].id);
            extractFile(files[i].id, outPath + "/unknown_" + string(buffer));
        }

    }


    return true;
}

bool MixFile::extractAllFast(std::string outPath) {
    int i;
    char buffer[16];

    for (i = 0; i < mix_head.c_files; i++) {
        sprintf(buffer, "%x", files[i].id);
        extractFile(files[i].id, outPath + "/unknown_" + string(buffer));
    }
    return true;
}

bool MixFile::extractFile(uint32_t fileID, std::string outPath) {
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

bool MixFile::extractFile(std::string fileName, std::string outPath) {
    return extractFile(getID(mixGame, fileName), outPath);
}

vector<string> MixFile::getFileNames() {
    if (filenames.empty())
        readFileNames();

    return filenames;
}

bool MixFile::readFileNames() {
    int i;

    vector<string> filenamesdb;
    vector<string> filenamesdb_local;

    /* read global and local database of filenames */
    filenamesdb = globaldb->getFileNames();
    if (mixdb)
        filenamesdb_local = mixdb->getFileNames();

    /* clear previous filenames list */
    filenames.clear();

    for (i = 0; i < mix_head.c_files; i++) {
        bool found = false;

        /* try filenames from global database */
        for (uint32_t j = 0; j < filenamesdb.size(); j++) {
            if (getID(mixGame, filenamesdb[j]) == files[i].id) {
                filenames.push_back(filenamesdb[j]);
                found = true;
                break;
            }
        }

        /* try filenames from local database */
        if (mixdb && !found) {
            for (uint32_t j = 0; j < filenamesdb_local.size(); j++) {
                if (getID(mixGame, filenamesdb_local[j]) == files[i].id) {
                    filenames.push_back(filenamesdb_local[j]);
                    found = true;
                    break;
                }

            }
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

bool MixFile::decrypt(std::string outPath) {
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
    delete mixdb;
    mixdb = NULL;
    dataoffset = 0;
    m_has_checksum = false;
    m_is_encrypted = false;    
}
