/* 
 * File:   ccmix.cpp
 * Author: fbsagr
 *
 * Created on April 10, 2014, 1:43 PM
 */

#include "SimpleOpt.h"
#include "SimpleGlob.h"
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _MSC_VER

#include <windows.h>
#include <tchar.h>
#include "win32/dirent.h"

#else

#include <dirent.h>
#define TCHAR		char
#define _T(x)		x
#define _tprintf	printf
#define _tmain		main

#endif

using namespace std;

enum { OPT_HELP, OPT_EXTRACT, OPT_CREATE, OPT_GAME, OPT_FILES };

void ShowUsage()
{
    cout << "Usage: ccmix [-x|-c] (-f FILE) (-d DIR) MIXFILE" << endl;
    cout << "Try `ccmix -?` or `ccmix --help` for more information." << endl;
}

void ShowHelp()
{
    cout << "***ccmix program usage***\n" << endl;
    cout << "Usage: ccmix [-x|-c] (-f FILE) (-d DIR) (-g GAME) MIXFILE" << endl;
    cout << "Extract mode -x." << endl;
    cout << "Extracts the contents specified mix file to the current directory.\n"
            "-f specifies a single file to extract\n"
            "-d specifies an alternative directory to extract to\n" << endl;
}

CSimpleOpt::SOption g_rgOptions[] = {
    { OPT_EXTRACT,  _T("-x"),           SO_NONE    }, // "-x"
    { OPT_CREATE,   _T("-c"),           SO_NONE    }, // "-c"
    { OPT_FILES,    _T("-f"),           SO_REQ_SEP }, // "-f ARG"
    { OPT_FILES,    _T("-d"),           SO_REQ_SEP }, // "-d ARG"
    { OPT_GAME,     _T("-g"),           SO_REQ_SEP }, // "-g ARG"
    { OPT_HELP,     _T("-?"),           SO_NONE    }, // "-?"
    { OPT_HELP,     _T("--help"),       SO_NONE    }, // "--help"
    SO_END_OF_OPTIONS                       // END
};
    
int _tmain(int argc, TCHAR** argv)
{
    
    CSimpleOpt args(argc, argv, g_rgOptions);
    
     while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
            if (args.OptionId() == OPT_HELP) {
                ShowHelp();
                return 0;
            }
            _tprintf(_T("Option, ID: %d, Text: '%s', Argument: '%s'\n"),
                args.OptionId(), args.OptionText(),
                args.OptionArg() ? args.OptionArg() : _T(""));
        }
        else {
            _tprintf(_T("Invalid argument: %s\n"), args.OptionText());
            ShowUsage();
            return 1;
        }
    }
    
    return 0;
}

