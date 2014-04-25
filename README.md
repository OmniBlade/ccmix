ccmix
=====

Fork of http://code.google.com/p/tsunmix/

Overview
========

ccmix is a command line tool to create and extract from archive files in the Westwood Studios .mix format. The tool can create any of the 3 main varients used in classic Westwood games. These are Tiberian Dawn versions, Red Alert versions with both encrypted and unencrypted file headers and Tiberian Sun versions with encrypted and unencrypted file headers. It also optionally adds the XCC extension of including a local filenames database to allow recovery of filenames from the one way hash they are stored as.

Useage
======

Generally the useage is of the form ccmix --mode --mix /path/to/file.mix with various options available that alter the progams behaviour when creating or extracting files. The currently supported modes are as follows:

--extract

This will extract a file or all files from a mix. Requires a --mix option.

--create

This will create a mix file from the contents of a directory. Currently requires a minimum of the --dir and --mix options.

--list

This will list the contents of a mix file. Requires the --mix options.

The following options can also be used.

--file /path/to/file.name

Specifies a specific file to extract.

--dir /path/to/dir

Specifies a folder to extract to, or the contents of which to make a .mix from.

--encrypt

Used when creating a file, encrypts the header with blowfish. Key source is generated from rand().

--lmd

Specifies if a local mix database.dat file following the XCC format should be generated and included in the mix.

Acknowledgements
================
ivosh-l author of tsunmix on which ccmix is based.
Olaf van der Spek for his work reverse engineering the C&C files formats and the tools he has developed over the years to allow modding these classic games.
