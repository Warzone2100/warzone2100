/*
 * autorevision - a tool to incorporate Subversion revisions into binary builds
 * Copyright (C) 2005 Thomas Denk
 *
 * This program is distributed under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * Nicely taken from:
 * Revision: 4366
 * Id: autorevision.cpp 4366 2007-08-12 19:08:51Z killerbot
 * HeadURL: http://svn.berlios.de/svnroot/repos/codeblocks/trunk/src/build_tools/autorevision/autorevision.cpp
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <stdio.h>
#include <string>
#include <fstream>

#include "tinyxml/tinystr.h"
#include "tinyxml/tinyxml.h"

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
        puts("Usage: autorevision [options] directory [autorevision.h]");
        puts("Options:");
        puts("         +int assign const unsigned int");
        puts("         +std assign const std::string");
        puts("         +wx  assing const wxString");
        puts("         +t   add Unicode translation macros to strings");
        puts("         -v   be verbose");
        return 1;
    }

    if(outputFile.empty())
        outputFile.assign("autorevision.h");

    string revision;
    string date;
    string comment;
    string old;

    QuerySvn(workingDir, revision, date);
    WriteOutput(outputFile, revision, date);

    return 0;
}



bool QuerySvn(const string& workingDir, string& revision, string &date)
{
    revision = "0";
    date = "unknown date";
    string svncmd("svn info --xml --non-interactive ");
    svncmd.append(workingDir);

    FILE *svn = popen(svncmd.c_str(), "r");

    if(svn)
    {
        char buf[16384] = {'0'};
        fread(buf, 16383, 1, svn);
        pclose(svn);

        TiXmlDocument doc;
        doc.Parse(buf);

        if(doc.Error())
            return 0;

        TiXmlHandle hCommit(&doc);
        hCommit = hCommit.FirstChildElement("info").FirstChildElement("entry").FirstChildElement("commit");
        if(const TiXmlElement* e = hCommit.ToElement())
        {
            revision = e->Attribute("revision") ? e->Attribute("revision") : "";
            const TiXmlElement* d = e->FirstChildElement("date");
            if(d && d->GetText())
            {
                date = d->GetText();
            }
            return 1;
        }
    }
    return 0;
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


    FILE *header = fopen(outputFile.c_str(), "wb");
    if(!header)
    {
        puts("Error: Could not open output file.");
        return false;
    }

    fprintf(header, "%s\n", comment.c_str());
    fprintf(header, "//don't include this header, only configmanager-revision.cpp should do this.\n");
    fprintf(header, "#ifndef AUTOREVISION_H\n");
    fprintf(header, "#define AUTOREVISION_H\n\n\n");

    if(do_std)
        fprintf(header, "#include <string>\n");
    if(do_wx)
        fprintf(header, "#include <wx/string.h>\n");

    if(do_int || do_std || do_wx)
        fprintf(header, "\nnamespace autorevision\n{\n");

    if(do_int)
        fprintf(header, "\tconst unsigned int svn_revision = %s;\n", revision.c_str());

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
        fprintf(header, "\tconst std::string svn_revision_s(%s);\n", revision.c_str());
    if(do_wx)
        fprintf(header, "\tconst wxString svnRevision(%s);\n", revision.c_str());

    if(do_std)
        fprintf(header, "\tconst std::string svn_date_s(%s);\n", date.c_str());
    if(do_wx)
        fprintf(header, "\tconst wxString svnDate(%s);\n", date.c_str());

    if(do_int || do_std || do_wx)
        fprintf(header, "}\n\n");

    fprintf(header, "\n\n#endif\n");
    fclose(header);

    return true;
}

