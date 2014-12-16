#include "mix_header.h"

#ifdef _MSC_VER

#include <Windows.h>

#endif

#include "cryptopp/rsa.h"
#include "cryptopp/blowfish.h"
#include "cryptopp/integer.h"
#include "cryptopp/modes.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

const uint32_t REN_SIG = 0x3158494D;
const char* PUBKEY = "0x51bcda086d39fce4565160d651713fa2e8aa54fa6682b04aabdd0e6af8b0c1e6d1fb4f3daa437f15";
//const char* PUBKEY = "AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V";
const char* PRVKEY = "0x0a5637bc99139c47c422c67c54105e5bdbd0aeae4ab4d4334358274e1bdf5706a1fbf4e682893081";
//const char* PRVKEY = "AigKVje8mROcR8QixnxUEF5b29Curkq01DNDWCdOG99XBqH79OaCiTCB";
const int KEYSIZE = 56;

using CryptoPP::Integer;
using CryptoPP::RSA;
using CryptoPP::ECB_Mode;
using CryptoPP::Blowfish;

MixHeader::MixHeader(t_game game) :
mix_checksum(0x00010000),
mix_encrypted(0x00020000)
{
    m_game_type = game;
    m_file_count = 0;
    m_body_size = 0;
    m_header_flags = 0;
    m_has_checksum = 0;
    m_is_encrypted = 0;
    //header will always be at bare min 6 uint8_ts
    if(game == game_td) {
        m_header_size = 6;
    } else {
        m_header_size = 10;
    }
    //seed random number for generating random keys
    srand(time(NULL));
}

bool MixHeader::readHeader(std::fstream &fh)
{
    fh.seekg(0, std::ios::beg);
    //read first 6 uint8_ts, determine if we have td mix or not.
    char flagbuff[6];
    fh.read(flagbuff, 6);
    
    //std::cout << MixID::idStr(flagbuff, 6) << " Retrieved from header." << std::endl;
    
    if(*reinterpret_cast<uint32_t*>(flagbuff) == REN_SIG){
        std::cout << "Error, Renegade mix format not supported." << std::endl;
        return false;
    }
    
    if(*reinterpret_cast<uint16_t*>(flagbuff)) {
        if(m_game_type > 1){
            //m_game_type = game_td;
            std::cout << "Warning, header indicates mix is of type \"game_td\" "<<
                    " but isn't what you specified" << std::endl;
        }
        
        m_file_count = *reinterpret_cast<uint16_t*>(flagbuff);
        m_body_size = *reinterpret_cast<uint32_t*>(flagbuff + 2);
        
        return readUnEncrypted(fh);
        
    } else {
        if(!m_game_type){
            std::cout << "Warning, game type is td but header is new format." << std::endl;
        }
        //lets work out what kind of header we have
        m_header_flags = *reinterpret_cast<int32_t*>(flagbuff);
        m_has_checksum = m_header_flags & mix_checksum;
        m_is_encrypted = m_header_flags & mix_encrypted;
        m_header_size += 4; 
                
        if(m_is_encrypted){
            return readEncrypted(fh);
        } else {
            return readUnEncrypted(fh);
        }
    }
    return false;
}

bool MixHeader::readUnEncrypted(std::fstream &fh)
{
    t_mix_entry entry;
    std::pair<t_mix_index_iter,bool> rv;
    
    //new format won't have filecount yet
    if(m_file_count) { 
        fh.seekg(6, std::ios::beg); 
    } else {
        fh.seekg(4, std::ios::beg);
        fh.read(reinterpret_cast<char*>(&m_file_count), sizeof(uint16_t));
        fh.read(reinterpret_cast<char*>(&m_body_size), sizeof(uint32_t));
    }
    
    m_header_size += m_file_count * 12;
    
    //read entries
    for (uint16_t i = 0; i < m_file_count; i++) {
        fh.read(reinterpret_cast<char*>(&entry.first), sizeof(int32_t));
        fh.read(reinterpret_cast<char*>(&entry.second), sizeof(t_index_info));
        rv = m_index.insert(entry);
        if(!rv.second) {
            std::cout << "Error reading header, duplicate ID" << std::endl;
            return false;
        }
    }
    return true;
}

bool MixHeader::readEncrypted(std::fstream& fh)
{
    t_mix_entry entry;
    std::pair<t_mix_index_iter,bool> rv;
    
    ECB_Mode< Blowfish >::Decryption blfish;
    int block_count;
    char pblkbuf[8];
    
    //header is at least 84 uint8_ts long at this point due to key source
    m_header_size = 84;
    
    //read keysource to obtain blowfish key
    //fh.seekg(4, std::ios::beg);
    readKeySource(fh);
    blfish.SetKey(reinterpret_cast<uint8_t*>(m_key), 56);
    
    //read first block to get file count, needed to calculate header size
    fh.read(pblkbuf, 8);
    blfish.ProcessString(reinterpret_cast<uint8_t*>(pblkbuf), 8);
    memcpy(reinterpret_cast<char*>(&m_file_count), pblkbuf, 2);
    memcpy(reinterpret_cast<char*>(&m_body_size), pblkbuf + 2 , 4);
    
    //workout size of our header and how much we need to decrypt
    //take into account 2 uint8_ts left from getting the file count
    block_count = ((m_file_count * 12) - 2) / 8;
    if (((m_file_count * 12) - 2) % 8) block_count++;
    //add 8 to compensate for block we already decrypted
    m_header_size += block_count * 8 + 8;
    
    //prepare our buffer and copy in first 2 uint8_ts we got from first block
    //index_buffer.resize(block_count * 8 + 2);
	std::vector<char> indbuf(block_count * 8 + 2);
    char* pindbuf = &indbuf.at(0);
    memcpy(pindbuf, pblkbuf + 6 , 2);
    
    //loop to decrypt index into index buffer
    for(int i = 0; i < block_count; i++) {
        fh.read(pblkbuf, 8);
        blfish.ProcessString(reinterpret_cast<uint8_t*>(pblkbuf), 8);
        memcpy(pindbuf + 2 + 8 * i, pblkbuf, 8);
    }
    
    //read entries
    for (uint16_t i = 0; i < m_file_count; i++) {
        memcpy(reinterpret_cast<char*>(&entry.first), pindbuf + i * 12,
               sizeof(int32_t));
        memcpy(reinterpret_cast<char*>(&entry.second), pindbuf + 4 + i * 12,
               sizeof(t_index_info));
        rv = m_index.insert(entry);
        if(!rv.second) {
            std::cout << "Error reading header, duplicate ID" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool MixHeader::readKeySource(std::fstream& fh)
{
    if(!fh.is_open()) return false;
    
    fh.seekg(0, std::ios::beg);
    //read first 4 uint8_ts, determine if we have keysource mix or not.
    char flagbuff[4];
    fh.read(flagbuff, 4);
    int32_t flags = *reinterpret_cast<int32_t*>(flagbuff);
    
    if(flags != mix_encrypted && flags != mix_encrypted + mix_checksum) {
        std::cout << "key_source not suitable." << std::endl;
        return false;
    }
    
    fh.read(m_keysource, 80);
    //sets the blowfish key from the keysource
    setKey();
    return true;
}

void MixHeader::setKey()
{
    //need this for cryptopp as it uses bigendian big int, game uses little.
    //temp buffer to flip the keysource and final key
    uint8_t keybuf[80];
    
    //decode public and private keys and convert to Integer
    /*
    std::string pubkey;
    std::string prvkey;
    Base64Decoder decode;
    decode.Put(reinterpret_cast<const uint8_t*>(PUBKEY), KEYSIZE);
    pubkey.resize(decode.MaxRetrievable());
    decode.Get((uint8_t*)pubkey.data(), pubkey.size());
    decode.Put(reinterpret_cast<const uint8_t*>(PRVKEY), KEYSIZE);
    prvkey.resize(decode.MaxRetrievable());
    decode.Get((uint8_t*)prvkey.data(), prvkey.size());
    pubkey.erase(0, 2);
    prvkey.erase(0, 2);
     * 
    Integer n((uint8_t*)pubkey.data(), pubkey.size()), 
            e("0x10001"), 
            d((uint8_t*)prvkey.data(), prvkey.size());
     */
    
    //setup the key integers for RSA
    /*
    Integer n(PUBKEY), 
            e("0x10001"), 
            d(PRVKEY);
     */
    
    //setup RSA key structure
    RSA::PrivateKey rsakey;
    rsakey.Initialize(Integer(PUBKEY), Integer("0x10001"), Integer(PRVKEY));
    
    //reverse endianess of the keysource for cryptopp
    int i = 0;
    int j = 79;
    for(; i < 80; i++) {
        keybuf[j] = m_keysource[i];
        j--;
    }
    
    //convert keysource to integers
    Integer keyblk1(keybuf, 40), keyblk2(keybuf + 40, 40);
    
    //decrypt
    keyblk1 = rsakey.ApplyFunction(keyblk1);
    keyblk2 = rsakey.ApplyFunction(keyblk2);
    
    Integer blowfishint = (keyblk1 << 312) + keyblk2;
    
    //encode to bytes
    blowfishint.Encode(keybuf, 56);
    
    //reverse endianess to final key
    i = 0;
    j = 55;
    for(; i < 56; i++)
    {
        m_key[i] = keybuf[j];
        j--;
    }
}

//This function is called by setEncrypted and generated a new key
void MixHeader::setKeySource()
{
    //buffer for flipping the array
    uint8_t keybuf[80];
    uint8_t byte1;
    uint8_t byte2;
            
    //set our private key, this is other way round to decrypting
    RSA::PrivateKey rsakey;
    rsakey.Initialize(Integer(PUBKEY), Integer(PRVKEY), Integer("0x10001"));
    
    if(m_game_type > game_ra) {
        byte1 = 0x18;
        byte2 = 0xBB;
    } else {
        byte1 = 0x0C;
        byte2 = 0x05;
    }
    
    //generate a random blowfish key that mimics the ones WW used
    //they probably didn't generate them this way
    for(int i = 0; i < 56; i++) {
        if(i < 7) {
            m_key[i] = rand() % 256;
        } else if (i < 26) {
            m_key[i] = byte1;
        } else if (i < 33) {
            m_key[i] = rand() % 256;
        } else if (i < 56) {
            m_key[i] = byte2;
        }
    }
    
    //reverse endianess for RSA enc
    for(int i = 0, j = 55; i < 56; i++)
    {
        keybuf[i] = m_key[j];
        j--;
    }
    Integer blowfish((uint8_t*)keybuf, 56);
    Integer keyblk1 = blowfish >> 312;
    Integer keyblk2 = blowfish - (keyblk1 << 312);
    
    std::cout << "Generated Blowfish Key:\n";
    std::cout << std::hex << blowfish << std::dec << "\n";
    
    //encrypt
    keyblk1 = rsakey.ApplyFunction(keyblk1);
    keyblk2 = rsakey.ApplyFunction(keyblk2);
    
    //buffer the big endian keyblock
    keyblk1.Encode(keybuf, 40);
    keyblk2.Encode(keybuf + 40, 40);
    
    //reverse endianess of the keysource for the game
    for(int i = 0, j = 79; i < 80; i++) {
        m_keysource[j] = keybuf[i];
        j--;
    }
}

bool MixHeader::writeHeader(std::fstream& fh)
{   
    //make sure we are at the start of the file stream we were passed
    fh.seekg(0, std::ios::beg);
    if(m_is_encrypted){
        return writeEncrypted(fh);
    } else {
        return writeUnEncrypted(fh);
    }
}

bool MixHeader::writeUnEncrypted(std::fstream& fh)
{
    //write flags if supported by game format
    if(m_game_type) fh.write(reinterpret_cast<char*>(&m_header_flags), 4);
    
    //write header info
    fh.write(reinterpret_cast<char*>(&m_file_count), 2);
    fh.write(reinterpret_cast<char*>(&m_body_size), 4);
    
    //write index, first is id, second is struct of offset and size
    for(t_mix_index_iter it = m_index.begin(); it != m_index.end(); ++it) {
        fh.write(reinterpret_cast<const char*>(&(it->first)), 4);
        fh.write(reinterpret_cast<const char*>(&(it->second)), 8);
    }
    return true;
}

bool MixHeader::writeEncrypted(std::fstream& fh)
{
    ECB_Mode< Blowfish >::Encryption blfish;
    //std::vector<char> block_buff(8);
    //std::vector<char> index_buffer(8);
    int block_count;
    int offset = 0;
    //char* pindbuf = &index_buffer[0];
    //char* pblkbuf = &block_buff[0];
    char pblkbuf[8];
    
    //always write flags if encrypted.
    fh.write(reinterpret_cast<char*>(&m_header_flags), 4);
    //encrypted file needs its key_source
    fh.write(m_keysource, 80);
    
    //work out how much data needs encrypting, set vector size
    block_count = ((m_file_count * 12) + 6) / 8;
    if(((m_file_count * 12) + 6) % 8) block_count++;
    //index_buffer.resize(block_count * 8);
	std::vector<char> indbuf(block_count * 8);
    char* pindbuf = &indbuf.at(0);
    //char pindbuf[block_count * 8];
    
    //this needs to be here for checksum when encrypted
    m_header_size = 84 + block_count * 8;
    
    //fill the buffer
    memcpy(pindbuf + offset, reinterpret_cast<char*>(&m_file_count), 2);
    offset += 2;
    memcpy(pindbuf + offset, reinterpret_cast<char*>(&m_body_size), 4);
    offset += 4;
    
    for(t_mix_index_iter it = m_index.begin(); it != m_index.end(); ++it) {
        memcpy(pindbuf + offset, reinterpret_cast<const char*>(&(it->first)), 4);
        offset += 4;
        memcpy(pindbuf + offset, reinterpret_cast<const char*>(&(it->second)), 8);
        offset += 8;
    }
    
    //prepare blowfish
    blfish.SetKey(reinterpret_cast<uint8_t*>(m_key), 56);
    
    //encrypt and write to file.
    offset = 0;
    while(block_count--){
        memcpy(pblkbuf, pindbuf + offset, 8);
        //blfish.encipher(pblkbuf, pblkbuf, 8);
        blfish.ProcessString(reinterpret_cast<uint8_t*>(pblkbuf), 8);
        fh.write(pblkbuf, 8);
        offset += 8;
    }
    
    return true;
}

bool MixHeader::addEntry(int32_t id, uint32_t size)
{
    t_index_info info;
    t_mix_entry entry;
    std::pair<t_mix_index_iter,bool> rv;
    
    info.offset = m_body_size;
    info.size = size;

    entry.first = id;
    entry.second = info;
    
    rv = m_index.insert(entry);
    if(!rv.second) {
        std::cout << "Error entry not added, duplicate ID of " << entry.first << std::endl;
        return false;
    }
    m_body_size += size;
    m_header_size += 12;
    m_file_count += 1;
    return true;
}

bool MixHeader::removeEntry(int32_t id, bool adjust = false)
{
    t_mix_index_iter old = m_index.find(id);
    
    if(old == m_index.end()) {
        std::cout << "Element not found" << std::endl;
        return false;
    }
    
    //if we adjust for compact, reduce offsets > removed to be - removed size
    if(adjust) {
        for(t_mix_index_iter it = m_index.begin(); it != m_index.end(); ++it) {
            if(it->second.offset > old->second.offset) {
                it->second.offset -= old->second.size;
            }
        }
    }
    
    m_body_size -= old->second.size;
    m_header_size -= 12;
    m_file_count -= 1;
    m_index.erase(old);
    return true;
}

//Would there be a use for this?
/*void MixHeader::setGame(t_game game)
{
    t_game old = m_game_type
    m_game_type = game;
    
    //have we gone from new type header to old?
    if(old & !m_game_type) {
        clearIsEncrypted();
        clearHasChecksum();
        m_header_size -= 4;
    } else if (!old & m_game_type) {
        m_header_size += 4;
    }
    
}*/

t_index_info MixHeader::getEntry(int32_t id)
{
    t_index_info rv;
    rv.offset = 0;
    rv.size = 0;
    
    t_mix_index_iter info = m_index.find(id);
    
    if(info != m_index.end()) rv = info->second;
    
    return rv;
}

void MixHeader::setHasChecksum()
{
    if(m_game_type) {
        m_has_checksum = true;
        m_header_flags |= mix_checksum;
    }
}

void MixHeader::clearHasChecksum()
{
    if(m_game_type) {
        m_has_checksum = false;
        m_header_flags &= ~(mix_checksum);
    }
}

void MixHeader::setIsEncrypted()
{
    if(m_game_type) {
        uint32_t block_count = ((m_file_count * 12) + 6) / 8;
        if(((m_file_count * 12) + 6) % 8) block_count++;
        m_header_size = 84 + block_count * 8;
        m_is_encrypted = true;
        m_header_flags |= mix_encrypted;
        setKeySource();
    }
}

void MixHeader::clearIsEncrypted()
{
    if(m_game_type) {
        m_header_size = 10 + m_file_count * 12;
        m_is_encrypted = false;
        m_header_flags &= ~(mix_encrypted);
    }
}
