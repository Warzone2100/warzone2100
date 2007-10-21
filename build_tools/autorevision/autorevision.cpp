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

bool QuerySvn(const string& workingDir, string& revision, string &date);
bool ParseFile(const string& docFile, string& revision, string &date);
bool WriteOutput(const string& outputFile, string& revision, string& date);
int main(int argc, char** argv);


bool do_int = false;
bool do_std = false;
bool do_wx  = false;
bool do_translate  = false;
bool be_verbose  = false;

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
             << "         +int assign const unsigned int\n"
             << "         +std assign const std::string\n"
             << "         +wx  assing const wxString\n"
             << "         +t   add Unicode translation macros to strings\n"
             << "         -v   be verbose\n";

        return 1;
    }

    if(outputFile.empty())
        outputFile.assign("autorevision.h");

    string docFile(workingDir);
    docFile.append("/.svn/entries");
    string docFile2(workingDir);
    docFile2.append("/_svn/entries");

    string revision;
    string date;
    string comment;
    string old;

    QuerySvn(workingDir, revision, date) || ParseFile(docFile, revision, date) || ParseFile(docFile2, revision, date);

    if(revision == "")
        revision = "0";

    WriteOutput(outputFile, revision, date);

    return 0;
}



bool QuerySvn(const string& workingDir, string& revision, string &date)
{
    string svncmd("svn info ");
    svncmd.append(workingDir);
    set_env("LANG", "en_US");
    FILE *svn = popen(svncmd.c_str(), "r");

    if(svn)
    {
        char buf[1024];
        string line;
        while(fgets(buf, 4095, svn))
        {
            line.assign(buf);
            if(line.find("Revision:") != string::npos)
            {
                revision = line.substr(strlen("Revision: "));

                    string lbreak("\r\n");
                    size_t i;
                    while((i = revision.find_first_of(lbreak)) != string::npos)
                        revision.erase(revision.length()-1);
            }
            if(line.find("Last Changed Date: ") != string::npos)
            {
                    date = line.substr(strlen("Last Changed Date: "), strlen("2006-01-01 12:34:56"));
            }
        }
    }
    pclose(svn);
    return !revision.empty();
}


bool ParseFile(const string& docFile, string& revision, string &date)
{
    string token[6];
    date.clear();
    revision.clear();
    int c = 0;

    ifstream inFile(docFile.c_str());
    if (!inFile)
    {
        return false;
    }
    else
    {
        while(!inFile.eof() && c < 6)
            inFile >> token[c++];

        revision = token[2];
        date = token[5].substr(0, strlen("2006-01-01T12:34:56"));
        date[10] = 32;

        return true;
    }
}


bool WriteOutput(const string& outputFile, string& revision, string& date)
{
    string old;
    string comment("/*");
    comment.append(revision);
    comment.append("*/");

    {
        ifstream in(outputFile.c_str());
        if (!in.bad() && !in.eof())
        {
            in >> old;
            if(old >= comment)
            {
                if(be_verbose)
                    printf("Revision unchanged (%s). Skipping.", revision.c_str());
                in.close();
                return false;
            }
        }
        in.close();
    }


    ofstream header(outputFile.c_str(), ios_base::out | ios_base::binary);
    if(!header.is_open())
    {
        puts("Error: Could not open output file.");
        return false;
    }

    header << comment << "\n"
           << "#ifndef AUTOREVISION_H\n"
           << "#define AUTOREVISION_H\n\n\n";

    if(do_std)
        header << "#include <string>\n";
    if(do_wx)
        header << "#include <wx/string.h>\n";

    header << "\n#define SVN_REVISION \"" << revision << "\"\n"
           << "\n#define SVN_DATE     \"" << date << "\"\n\n";

    if(do_int || do_std || do_wx)
        header << "namespace autorevision\n{\n";

    if(do_int)
        header << "\tconst unsigned int svn_revision = " << revision << ";\n";

    if(do_translate)
    {
        revision = "_T(\"" + revision + "\")";
        date = "_T(\"" + date + "\")";
    }
    else
    {
        revision = "\"" + revision + "\"";
        date = "\"" + date + "\"";
    }

    if(do_std)
        header << "\tconst std::string svn_revision_s(" << revision << ");\n";
    if(do_wx)
        header << "\tconst wxString svnRevision(" << revision << ");\n";

    if(do_int || do_std || do_wx)
        header << "}\n\n";

    header << "\n\n#endif\n";
    header.close();

    return true;
}

