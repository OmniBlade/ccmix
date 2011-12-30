/* 
 * File:   main.cpp
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 11:30
 */

#include <cstdlib>
#include "mix_file.h"
#include <iostream>
#include <string.h>

using namespace std;

int main(int argc, char** argv) {
    MixFile mix;
    string oPath;
    string iFile;
    if (argc < 2) {
        cout << "Usage \"tsunmix <filename.mix> [list,extract,getid] {file}\"." << endl;
        return 0;
    }
    if (!mix.open(argv[1])) {
        cout << "File \"" << argv[1] << "\" not found!" << endl;
        return 1;
    }
    if (argc < 3) {
        mix.printFileList(0);
    } else {
        if (!strcmp(argv[2], "list")) {
            mix.printFileList(1);
        } else if (!strcmp(argv[2], "extract") || !strcmp(argv[2], "extid") || !strcmp(argv[2], "extid16")) {

            if (argc < 4) {
                cout << "Usage \"tsunmix <filename.mix> extract <file>\"." << endl;
            } else {

                for (int i = 3; i < argc; i++) {
                    oPath.clear();
                    iFile = argv[i];
                    if (argc > (i + 2)) {
                        if (!strcmp(argv[i + 1], "-o")) {
                            oPath = argv[i + 2];
                        }
                    }
                    if (oPath.empty())
                        oPath = argv[i];
                    else
                        i += 2;
                    if (!strcmp(argv[2], "extid")) {
                        if (!mix.extractFile(strtol(iFile.c_str(), NULL, 10), oPath.c_str())) {
                            cout << "File \"" << iFile.c_str() << "\" not found in the archive." << endl;
                        }
                    } if (!strcmp(argv[2], "extid16")) {
                        if (!mix.extractFile(strtol(iFile.c_str(), NULL, 16), oPath.c_str())) {
                            cout << "File \"" << iFile.c_str() << "\" not found in the archive." << endl;
                        }
                    } else {
                        if (!mix.extractFile(iFile.c_str(), oPath.c_str())) {
                            cout << "File \"" << iFile.c_str() << "\" not found in the archive." << endl;
                        }
                    }
                }
            }
        } else if (!strcmp(argv[2], "getid")) {
            if (argc < 4) {
                cout << "Usage \"tsunmix <filename.mix> getid <file>\"." << endl;

            } else {
                cout << argv[3] << " : " << hex << mix.get_id(game_ts, argv[3]) << ((mix.checkFileName(argv[3])) ? "" : " (NOT PRESENT)") << endl;
            }
        }
    }


    return 0;
}

