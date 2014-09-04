#include "../mix_dexoder.h"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
    std::fstream ifh;
    std::fstream ofh;
    uint8_t keysource[80];
    uint8_t key[56];
    char* infile = argv[1];
    
    ifh.open(infile, std::ios_base::in|std::ios_base::binary);
    ifh.seekg(4);
    ifh.read((char*)keysource, 80);
    ifh.close();
    
    get_blowfish_key(keysource, key);
    
    //ofh.open("./testkey.key", std::ios_base::out|std::ios_base::binary);
    //ofh.write((char*)key, 56);
    //ofh.close();
    
    //give_blowfish_key(key, keysource);
    
    //ofh.open("./testkey.source", std::ios_base::out|std::ios_base::binary);
    //ofh.write((char*)keysource, 80);
    //ofh.close();
}
