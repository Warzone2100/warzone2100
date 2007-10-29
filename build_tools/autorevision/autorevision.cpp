/*
 * autorevision - a tool to incorporate Subversion revisions into binary builds
 * Copyright (C) 2005 Thomas Denk
 * Copyright (C) 2007 Giel van Schijndel
 * Copyright (C) 2007 Warzone Resurrection Project
 *
 * This program is distributed under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * Nicely taken from:
 * Revision: 3543
 * Id: autorevision.cpp 3543 2007-01-27 00:25:43Z daniel2000
 * HeadURL: http://svn.berlios.de/svnroot/repos/codeblocks/trunk/src/build_tools/autorevision/autorevision.cpp
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <iostream>
#include <string>
#include <fstream>

using namespace std;

#ifdef __WIN32__
    #define WIN32_LEAN_AND_MEAN 1
    #define NOGDI
    #include <windows.h>
    inline void set_env(const char* k, const char* v) { SetEnvironmentVariable(k, v); };
#else
    #include <stdlib.h>
    inline void set_env(const char* k, const char* v) { setenv(k, v, 1); };
#endif

bool QuerySvn(const string& workingDir, string& revision, string& date, string& wc_uri);
bool ParseFile(const string& docFile, string& revision, string &date, string& wc_uri);
bool WriteOutput(const string& outputFile, const string& revision, const string& date, const string& wc_uri);
int main(int argc, char** argv);


static bool do_int  = false;
static bool do_std  = false;
static bool do_cstr = false;
static bool do_wx   = false;
static bool do_translate = false;
static bool be_verbose  = false;

int main(int argc, char** argv)
{
    string outputFile;
    string workingDir;


    for(int i = 1; i < argc; ++i)
    {
        if(strcmp("+int", argv[i]) == 0)
            do_int = true;
        else if(strcmp("+std", argv[i]) == 0)
            do_std = true;
        else if(strcmp("+cstr", argv[i]) == 0)
            do_cstr = true;
        else if(strcmp("+wx", argv[i]) == 0)
            do_wx = true;
        else if(strcmp("+t", argv[i]) == 0)
            do_translate = true;
        else if(strcmp("-v", argv[i]) == 0)
            be_verbose = true;
        else if(workingDir.empty())
            workingDir.assign(argv[i]);
        else if(outputFile.empty())
            outputFile.assign(argv[i]);
        else
            break;
    }

    if (workingDir.empty())
    {
        cout << "Usage: autorevision [options] directory [autorevision.h]\n"
             << "Options:\n"
             << "         +int  assign const unsigned int\n"
             << "         +std  assign const std::string\n"
             << "         +cstr assign const char[]\n"
             << "         +wx  assing const wxString\n"
             << "         +t    add Unicode translation macros to strings\n"
             << "         -v    be verbose\n";

        return 1;
    }

    if(outputFile.empty())
        outputFile.assign("autorevision.h");

    string docFile(workingDir);
    docFile.append("/.svn/entries");
    string docFile2(workingDir);
    docFile2.append("/_svn/entries");

    // Strings to extract Subversion information we want into
    // wc_uri: working copy root, e.g. trunk, branches/2.0, etc.
    string revision, date, wc_uri;

    if (!QuerySvn(workingDir, revision, date, wc_uri) && !ParseFile(docFile, revision, date, wc_uri) && !ParseFile(docFile2, revision, date, wc_uri)
     || revision == "")
    {
        cerr << "Error: failed retrieving version information.\n"
             << "Warning: using 0 as revision.\n";

        revision = "0";
    }

    WriteOutput(outputFile, revision, date, wc_uri);

    return 0;
}

void removeAfterNewLine(string& str)
{
    // Find a newline and erase everything that comes after it
    string::size_type lbreak_pos = str.find_first_of("\r\n");
    if (lbreak_pos != string::npos)
        str.erase(lbreak_pos);
}

bool QuerySvn(const string& workingDir, string& revision, string& date, string& wc_uri)
{
    string svncmd("svn info ");
    svncmd.append(workingDir);
    set_env("LANG", "C");
    FILE *svn = popen(svncmd.c_str(), "r");

    if(svn)
    {
        char buf[1024];
        string line, wc_repo_root;

        while(fgets(buf, sizeof(buf), svn))
        {
            line.assign(buf);
            if(line.find("Revision: ") != string::npos)
            {
                revision = line.substr(strlen("Revision: "));

                removeAfterNewLine(revision);
            }
            if(line.find("Last Changed Date: ") != string::npos)
            {
                date = line.substr(strlen("Last Changed Date: "), strlen("2006-01-01 12:34:56"));
            }
            if (line.find("Repository Root: ") != string::npos)
            {
                wc_repo_root = line.substr(strlen("Repository Root: "));

                removeAfterNewLine(wc_repo_root);
            }
            if (line.find("URL: ") != string::npos)
            {
                wc_uri = line.substr(strlen("URL: "));

                removeAfterNewLine(wc_uri);
            }
        }

        if (wc_uri.find(wc_repo_root) != string::npos)
        {
            wc_uri.erase(0, wc_repo_root.length() + 1); // + 1 to remove the prefixed slash also
        }
    }
    pclose(svn);
    return !revision.empty();
}


bool ParseFile(const string& docFile, string& revision, string &date, string& wc_uri)
{
    string token[6];
    date.clear();
    revision.clear();

    ifstream inFile(docFile.c_str());
    if (!inFile)
    {
        cerr << "Warning: could not open input file.\n"
             << "         This does not seem to be a revision controlled project.\n";

        return false;
    }

    unsigned int c = 0;

    // Read in the first entry's data (represents the current directory's entry)
    while(!inFile.eof() && c < 6)
        inFile >> token[c++];

    // The third non-empty line (zero-index) from the start contains the revision number
    revision = token[2];

    // The fourth non-empty line from the start contains the working copy URL. The fifth
    // non-empty line contains the repository root, which is the part of the working
    // copy URL we're not interested in.
    wc_uri = token[3].substr(token[4].length() + 1); // + 1 to remove the prefixed slash also

    // The sixth non-empty line from the start contains the revision date
    date = token[5].substr(0, strlen("2006-01-01T12:34:56"));
    // Set the 11th character from a 'T' to a space (' ')
    date[10] = ' ';

    return true;
}


bool WriteOutput(const string& outputFile, const string& revision, const string& date, const string& wc_uri)
{
    string comment("/*" + revision + "*/");

    // Check whether this file already contains the correct revision date. Don't
    // modify it if it does, we don't want to go messing with it's timestamp for
    // no gain.
    {
        ifstream in(outputFile.c_str());
        if (!in.bad() && !in.eof() && in.is_open())
        {
            string old;
            in >> old;
            if(old >= comment)
            {
                if(be_verbose)
                    cout << "Revision unchanged (" << revision << "). Skipping.\n";

                return false;
            }
        }
    }


    ofstream header(outputFile.c_str());
    if(!header.is_open())
    {
        cerr << "Error: Could not open output file.";
        return false;
    }

    header << comment << "\n"
              "#ifndef AUTOREVISION_H\n"
              "#define AUTOREVISION_H\n"
              "\n"
              "\n"
              "#ifndef SVN_AUTOREVISION_STATIC\n"
              "#define SVN_AUTOREVISION_STATIC\n"
              "#endif\n"
              "\n"
              "\n";

    if(do_std)
        header << "#include <string>\n";
    if(do_wx)
        header << "#include <wx/string.h>\n";

    header << "\n#define SVN_REV       " << revision << ""
           << "\n#define SVN_REVISION \"" << revision << "\""
           << "\n#define SVN_DATE     \"" << date << "\""
           << "\n#define SVN_URI      \"" << wc_uri << "\"\n";

    // Open namespace
    if(do_int || do_std || do_wx)
        header << "namespace autorevision\n{\n";

    if(do_int)
        header << "\tSVN_AUTOREVISION_STATIC const unsigned int svn_revision = " << revision << ";\n";

    if(do_std)
        header << "\tSVN_AUTOREVISION_STATIC const std::string svn_revision_s(\"" << revision << "\");\n"
               << "\tSVN_AUTOREVISION_STATIC const std::string svn_date_s(\"" << date << "\");\n"
               << "\tSVN_AUTOREVISION_STATIC const std::string svn_uri_s(\"" << wc_uri << "\");\n";
    if(do_cstr)
        header << "\tSVN_AUTOREVISION_STATIC const char svn_revision_cstr[] = \"" << revision << "\";\n"
               << "\tSVN_AUTOREVISION_STATIC const char svn_date_cstr[] = \"" << date << "\";\n"
               << "\tSVN_AUTOREVISION_STATIC const char svn_uri_cstr[] = \"" << wc_uri << "\";\n";
    if(do_wx)
    {
        header << "\tSVN_AUTOREVISION_STATIC const wxString svnRevision(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << revision << "\"));\n"

               << "\tSVN_AUTOREVISION_STATIC const wxString svnDate(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << date << "\"));\n"

               << "\tSVN_AUTOREVISION_STATIC const wxString svnUri(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << wc_uri << "\"));\n";
    }

    // Terminate/close namespace
    if(do_int || do_std || do_wx)
        header << "}\n\n";

    header << "\n\n#endif\n";

    return true;
}
