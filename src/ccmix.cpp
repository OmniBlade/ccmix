/* 
 * File:   ccmix.cpp
 * Author: fbsagr
 *
 * Created on April 10, 2014, 1:43 PM
 */

#include "SimpleOpt.h"
#include "mix_file.h"
//#include "SimpleGlob.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>

#ifdef _MSC_VER

#include <windows.h>
#include <tchar.h>
#include "win32/dirent.h"

#else

#include <pwd.h>
#include <dirent.h>
#define TCHAR		char
#define _T(x)		x
#define _tprintf	printf
#define _tmain		main

#endif

using namespace std;

enum { OPT_HELP, OPT_EXTRACT, OPT_CREATE, OPT_GAME, OPT_FILES, OPT_DIR };
typedef enum { NONE, EXTRACT, CREATE, ADD, REMOVE } t_mixmode;

#ifdef WINDOWS

string gmd_paths[3] = { "c:\\Program Files\\ccmix\\global mix database.dat",
                        "c:\\Users\\user\\global mix database.dat",
                        "c:\\Program Files\\xcc\\global mix database.dat" };

string user_home = getenv("HOMEDRIVE") + getenv("HOMEPATH");

#else

string gmd_paths[3] = { "/usr/local/ccmix/global mix database.dat",
                        "/home/user/global mix database.dat",
                        "/usr/shared/ccmix/global mix database.dat" };

string user_home = getpwuid(getuid())->pwd_dir;

#endif

// This just shows a quick usage guide in case an incorrect parameter was used
// or not all required args were provided.
inline void ShowUsage(TCHAR** argv)
{
    cout << "Usage: " << argv[0] << " [-x|-c] (-f FILE) (-d DIR) MIXFILE" << endl;
    cout << "Try `" << argv[0] << " -?` or `" << argv[0] << 
            " --help` for more information." << endl;
}

// Shows more detailed help if help flags were used in invocation.
inline void ShowHelp(TCHAR** argv)
{
    cout << "/n***ccmix program usage***\n" << endl;
    cout << "Usage: " << argv[0] << 
            " [COMMAND] (-f FILE) (-d DIR) (-g GAME) MIXFILE\n" << endl;
    cout << "Commands:" << endl;
    cout << "*extract*" << endl;
    cout << "Extracts the contents of the specified mix file to the current "
            "directory.\n"
            "-f specifies a single file to extract.\n"
            "-d specifies an alternative directory to extract to.\n" << endl;
    cout << "*create*" << endl;
    cout << "Creates a new mix file from the contents of the current folder.\n"
            "-f specifies a single file as initial file to add to the new mix.\n"
            "-d specifies an alternative directory to create mix from.\n" << endl;
}

//quick inline to respond to more than one command being specified.
inline void NoMultiMode()
{
    _tprintf(_T("You cannot specify more than one command at once.\n"));
    ShowUsage();
}

// Test if a file exists, used to find global mix database
inline bool exists_test (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

string FindGMD()
{
    string gmd_loc = "global mix database.dat";
    return gmd_loc;
}

CSimpleOpt::SOption g_rgOptions[] = {
    { OPT_EXTRACT,  _T("extract"),      SO_NONE    },
    { OPT_CREATE,   _T("create"),       SO_NONE    },
    { OPT_FILES,    _T("-f"),           SO_REQ_SEP }, // "-f ARG"
    { OPT_DIR,      _T("-d"),           SO_REQ_SEP }, // "-d ARG"
    { OPT_GAME,     _T("-g"),           SO_REQ_SEP }, // "-g ARG"
    { OPT_HELP,     _T("-?"),           SO_NONE    }, // "-?"
    { OPT_HELP,     _T("--help"),       SO_NONE    }, // "--help"
    SO_END_OF_OPTIONS                       // END
};
    
int _tmain(int argc, TCHAR** argv)
{
    if(argc <= 1) {
        ShowUsage(argv);
        return 0;
    }
    
    string file = "";
    string dir = "";
    string mixfile = string(argv[argc - 1]);
    t_game game = game_td;
    t_mixmode mode = NONE;
    
    CSimpleOpt args(argc, argv, g_rgOptions);
    
    while (args.Next()) {
        if (args.LastError() != SO_SUCCESS) {
            _tprintf(stderr, _T("Invalid argument: %s\n"), args.OptionText());
            ShowUsage(argv);
            return 1;
        }
        
        switch (args.OptionId()) {
        case OPT_HELP:
            ShowHelp(argv);
            return 0;
        case OPT_FILES:
            if (args.OptionArg() != NULL) {
                file = string(args.OptionArg());
            } else {
                _tprintf(stderr, _T("-f option requires a filename.\n"));
                return 1;
            }
            return 0;
        case OPT_DIR:
            if (args.OptionArg() != NULL) {
                dir = string(args.OptionArg());
            } else {
                _tprintf(stderr, _T("-d option requires a directory name.\n"));
                return 1;
            }
            return 0;
        case OPT_CREATE:
            if (mode != NONE) { NoMultiMode(); return 1; }
            mode = CREATE;
            break;
        case OPT_EXTRACT:
            if (mode != NONE) { NoMultiMode(); return 1; }
            mode = CREATE;
            break;
        default:
            if (args.OptionArg()) {
                _tprintf(_T("option: %2d, text: '%s', arg: '%s'\n"),
                    args.OptionId(), args.OptionText(), args.OptionArg());
            }
            else {
                _tprintf(_T("option: %2d, text: '%s'\n"),
                    args.OptionId(), args.OptionText());
            }
        }
    }
    
    return 0;
}

