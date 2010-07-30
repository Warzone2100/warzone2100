/*
 * autorevision - a tool to incorporate Subversion revisions into binary builds
 * Copyright (C) 2005       Thomas Denk
 * Copyright (C) 2008       Paul Wise
 * Copyright (C) 2007-2008  Giel van Schijndel
 * Copyright (C) 2007-2010  Warzone 2100 Project
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
#include <sstream>

#include <cstring>
#include <cstdlib>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


using namespace std;

#if !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
    #define WIN32_LEAN_AND_MEAN 1
    #define NOGDI
    #include <windows.h>
    inline void set_env(const char* k, const char* v) { SetEnvironmentVariableA(k, v); };
    #ifdef _MSC_VER
        #define popen  _popen
        #define pclose _pclose
    #endif
#else
    #include <cstdlib>
    #include <climits>
    #include <cerrno>
    #include <unistd.h>
    inline void set_env(const char* k, const char* v) { setenv(k, v, 1); };
#endif

template <typename T>
class assign_once : public T
{
    public:
        assign_once() :
            _assigned(false)
        {}

        assign_once(const T& data) :
            T(data),
            _assigned(false)
        {}

        const assign_once<T>& operator=(const T& data)
        {
            if (!_assigned)
            {
                T::operator=(data);
                _assigned = true;
            }

            return *this;
        }

        const assign_once<T&> set_default(const T& data)
        {
            if (!_assigned)
                T::operator=(data);

            return *this;
        }

    private:
        bool _assigned;
};

struct RevisionInformation
{
    RevisionInformation() :
        wc_modified(false),
        wc_switched(false)
    {}

    assign_once<std::string> low_revision;
    assign_once<std::string> revision;

    assign_once<std::string> date;

    /// working copy root, e.g. trunk, branches/2.0, etc.
    assign_once<std::string> wc_uri;

    bool wc_modified;
    bool wc_switched;
};

/** Abstract base class for classes that extract revision information.
 *  Most of these will extract this revision information from a Subversion
 *  working copy.
 *
 *  This class utilizes the "Chain of Responsibility" pattern to delegate tasks
 *  that the current concrete class cannot perform to another, succesive,
 *  subclass.
 */
class RevisionExtractor
{
    public:
        /** Constructor
         *  \param s successor of this instantation. If this instantation fails
         *           retrieving the revision, the successor pointed to by \c s,
         *           will attempt to do it instead.
         */
        RevisionExtractor(RevisionExtractor* s = NULL) :
            _successor(s)
        {}

        virtual ~RevisionExtractor()
        {}

        /** This function will perform the actual work of extracting the
         *  revision information.
         *
         *  RevisionExtractor::extractRevision can be used by subclasses to
         *  delegate the task to its successor if it cannot succeed itself.
         *
         *  For that purpose simply use a line like:
         *    "return extractRevision(revision, date, wc_uri);"
         */
        virtual bool extractRevision(RevisionInformation& rev_info) = 0;

    private:
        // Declare the pointer itself const; not whatever it points to
        RevisionExtractor* const _successor;
};

bool RevisionExtractor::extractRevision(RevisionInformation& rev_info)
{
    if (_successor)
        return _successor->extractRevision(rev_info);

    return false;
}

class RevSVNVersionQuery : public RevisionExtractor
{
    public:
        RevSVNVersionQuery(const std::string& workingDir, RevisionExtractor* s = NULL) :
            RevisionExtractor(s),
            _workingDir(workingDir)
        {}

        virtual bool extractRevision(RevisionInformation& rev_info);

    private:
        const std::string _workingDir;
};

class RevSVNQuery : public RevisionExtractor
{
    public:
        RevSVNQuery(const std::string& workingDir, RevisionExtractor* s = NULL) :
            RevisionExtractor(s),
            _workingDir(workingDir)
        {}

        virtual bool extractRevision(RevisionInformation& rev_info);

    private:
        const std::string _workingDir;
};

class RevFileParse : public RevisionExtractor
{
    public:
        RevFileParse(const std::string& docFile, RevisionExtractor* s = NULL) :
            RevisionExtractor(s),
            _docFile(docFile)
        {}

        virtual bool extractRevision(RevisionInformation& rev_info);

    private:
        const std::string _docFile;
};

class RevGitSVNQuery : public RevisionExtractor
{
    public:
        RevGitSVNQuery(const std::string& workingDir, RevisionExtractor* s = NULL) :
            RevisionExtractor(s),
            _workingDir(workingDir)
        {}

        virtual bool extractRevision(RevisionInformation& rev_info);

    private:
        bool FindRev(const char* cmd, std::string& str) const;

    private:
        const std::string _workingDir;
};

class RevConfigFile : public RevisionExtractor
{
    public:
        RevConfigFile(const std::string& configFile, RevisionExtractor* s = NULL) :
            RevisionExtractor(s),
            _configFile(configFile)
        {}

        virtual bool extractRevision(RevisionInformation& rev_info);

    private:
        const std::string _configFile;
};

bool WriteOutput(const string& outputFile, const RevisionInformation& rev_info);
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
        outputFile = "autorevision.h";

    RevConfigFile revRetr4(workingDir + "/autorevision.conf");
    RevFileParse revRetr3(workingDir + "/_svn/entries", &revRetr4);
    RevFileParse revRetr2(workingDir + "/.svn/entries", &revRetr3);
    RevGitSVNQuery revRetr2_1(workingDir, &revRetr2);
    RevSVNQuery  revRetr1(workingDir, &revRetr2_1);

    RevSVNVersionQuery revRetr(workingDir, &revRetr1);

    // Strings to extract Subversion information we want into
    RevisionInformation rev_info;

    if (!revRetr.extractRevision(rev_info)
     || rev_info.revision == "")
    {
        cerr << "Error: failed retrieving version information.\n"
             << "Warning: using 0 as revision.\n";

        rev_info.revision = "0";
    }

    if (rev_info.date == "")
        rev_info.date = "0000-00-00 00:00:00";

    WriteOutput(outputFile, rev_info);

    return 0;
}

static bool removeAfterNewLine(string& str)
{
    // Find a newline and erase everything that comes after it
    string::size_type lbreak_pos = str.find_first_of("\r\n");
    if (lbreak_pos != string::npos)
        str.erase(lbreak_pos);

    return !str.empty();
}

bool RevSVNVersionQuery::extractRevision(RevisionInformation& rev_info)
{
    string svncmd("svnversion " + _workingDir);
    set_env("LANG", "C");
    set_env("LC_ALL", "C");
    FILE* svn_version = popen(svncmd.c_str(), "r");

    if (!svn_version)
        return RevisionExtractor::extractRevision(rev_info);

    char buf[1024];
    string line;

    while (fgets(buf, sizeof(buf), svn_version))
    {
        line.assign(buf);
        removeAfterNewLine(line);

        std::string::size_type char_pos = line.find(':');

        if (char_pos != string::npos)
        // If a colon is present svnversion's output is as follows:
        // "XXXX:YYYYMS". X represents the low revision, Y the high revision,
        // M the working copy's modification state, and S the switch state.
        {
            rev_info.low_revision = line.substr(0, char_pos);
            line.erase(0, char_pos + 1);
        }

        if (line.find("exported") != string::npos)
        {
            pclose(svn_version);
            return RevisionExtractor::extractRevision(rev_info);
        }

        char_pos = line.find('M');
        if (char_pos != string::npos)
        {
            rev_info.wc_modified = true;
            line.erase(char_pos, 1);
        }

        char_pos = line.find('S');
        if (char_pos != string::npos)
        {
            rev_info.wc_switched = true;
            line.erase(char_pos, 1);
        }

        rev_info.revision = line;
    }

    pclose(svn_version);

    // The working copy URI still needs to be extracted. "svnversion" cannot
    // help us with that task, so delegate that task to another link in the
    // chain of responsibility.
    return RevisionExtractor::extractRevision(rev_info);
}

bool RevSVNQuery::extractRevision(RevisionInformation& rev_info)
{
    string svncmd("svn info " + _workingDir);
    set_env("LANG", "C");
    set_env("LC_ALL", "C");
    FILE *svn = popen(svncmd.c_str(), "r");

    if(!svn)
        return RevisionExtractor::extractRevision(rev_info);

    char buf[1024];
    string line, wc_repo_root;

    while(fgets(buf, sizeof(buf), svn))
    {
        line.assign(buf);
        if(line.find("Revision: ") != string::npos)
        {
            rev_info.revision = line.substr(strlen("Revision: "));

            removeAfterNewLine(rev_info.revision);
        }
        if(line.find("Last Changed Date: ") != string::npos)
        {
            rev_info.date = line.substr(strlen("Last Changed Date: "), strlen("2006-01-01 12:34:56"));
        }
        if (line.find("Repository Root: ") != string::npos)
        {
            wc_repo_root = line.substr(strlen("Repository Root: "));

            removeAfterNewLine(wc_repo_root);
        }
        if (line.find("URL: ") != string::npos)
        {
            rev_info.wc_uri = line.substr(strlen("URL: "));

            removeAfterNewLine(rev_info.wc_uri);
        }
    }

    pclose(svn);

    if (rev_info.wc_uri.find(wc_repo_root) != string::npos)
    {
        rev_info.wc_uri.erase(0, wc_repo_root.length() + 1); // + 1 to remove the prefixed slash also
    }

    if (rev_info.revision == "")
        return RevisionExtractor::extractRevision(rev_info);

    return true;
}

bool RevFileParse::extractRevision(RevisionInformation& rev_info)
{
    string token[6];

    ifstream inFile(_docFile.c_str());
    if (!inFile || !inFile.is_open())
    {
        cerr << "Warning: could not open input file.\n"
             << "         This does not seem to be a revision controlled project.\n";

        return RevisionExtractor::extractRevision(rev_info);
    }

    unsigned int c = 0;

    // Read in the first entry's data (represents the current directory's entry)
    while(!inFile.eof() && c < 6)
        inFile >> token[c++];

    // The third non-empty line (zero-index) from the start contains the revision number
    rev_info.revision = token[2];

    // The fourth non-empty line from the start contains the working copy URL. The fifth
    // non-empty line contains the repository root, which is the part of the working
    // copy URL we're not interested in.
    rev_info.wc_uri = token[3].substr(token[4].length() + 1); // + 1 to remove the prefixed slash also

    // The sixth non-empty line from the start contains the revision date
    rev_info.date = token[5].substr(0, strlen("2006-01-01T12:34:56"));
    // Set the 11th character from a 'T' to a space (' ')
    rev_info.date[10] = ' ';

    return true;
}

static string GetWorkingDir()
{
#if !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
    // Determine the required buffer size to contain the current directory
    const DWORD curDirSize = GetCurrentDirectoryA(0, NULL);

    // GetCurrentDirectoryA returns zero only on an error
    if (curDirSize == 0)
        return string();

    // Create a buffer of the required size
    // @note: We're using a dynamically allocated array here, instead of a
    //        variably sized array, because MSVC doesn't support variably sized
    //        arrays.
    char* curDir = new char[curDirSize];

    // Retrieve the current directory
    GetCurrentDirectoryA(curDirSize, curDir);

    // Return the current directory as a STL string
    string cwd(curDir);
    delete [] curDir;
    return cwd;
#else
    size_t curDirSize = PATH_MAX;
    char* curDir = new char[curDirSize];

    // Retrieve the current working directory
    if (!getcwd(curDir, curDirSize))
    {
        if (errno == ERANGE)
        {
            delete [] curDir;
            curDirSize *= 2;
            curDir = new char[curDirSize];

               if (!getcwd(curDir, curDirSize))
               {
                    curDir[0] = '\0';
               }
        }
        else
        {
            curDir[0] = '\0';
        }
    }

    string cwd(curDir);
    delete [] curDir;
    return cwd;
#endif
}

static bool ChangeDir(const string& dir)
{
#if !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
    return (SetCurrentDirectoryA(dir.c_str()) != 0);
#else
    return (chdir(dir.c_str()) == 0);
#endif
}

bool RevGitSVNQuery::FindRev(const char* cmd, std::string& str) const
{
    const string cwd(GetWorkingDir());
    if (!ChangeDir(_workingDir))
        return false;

    set_env("LANG", "C");
    set_env("LC_ALL", "C");
    FILE *gitsvn = popen(cmd, "r");

    // Change directories back
    ChangeDir(cwd);

    if (!gitsvn)
        return false;

    char buf[1024];
    const char* retval = fgets(buf, sizeof(buf), gitsvn);
    pclose(gitsvn);

    if (!retval)
        return false;

    str.assign(buf);
    return true;
}

bool RevGitSVNQuery::extractRevision(RevisionInformation& rev_info)
{
    string revision;
    if (!FindRev("git svn find-rev HEAD", revision)
     || !removeAfterNewLine(revision))
    {
        // If we got here it means that the current HEAD of git is no SVN
        // revision, which means that there are changes compared to whatever
        // SVN revision we originate from.
        std::string gitrevision;
        // Retrieve the most recently used git revision that's still from SVN
        if (!FindRev("git rev-list --max-count=1 --grep='git-svn-id: ' HEAD", gitrevision)
        // Match it up with a subversion revision number
         || !FindRev(("git svn find-rev " + gitrevision).c_str(), revision))
            return RevisionExtractor::extractRevision(rev_info);

        removeAfterNewLine(revision);
        rev_info.revision = revision;
        rev_info.wc_modified = true;
    }
    else
    {
        rev_info.revision = revision;
        if (system(NULL))
        {
            // This command will return without success if the working copy is
            // changed, wether checked into index or not.
            rev_info.wc_modified = system("git diff --quiet HEAD");
        }
    }

    return RevisionExtractor::extractRevision(rev_info);
}

bool RevConfigFile::extractRevision(RevisionInformation& rev_info)
{
    ifstream inFile(_configFile.c_str());
    if (!inFile || !inFile.is_open())
        return RevisionExtractor::extractRevision(rev_info);

    bool done_stuff = false;

    while (!inFile.eof())
    {
        // Read a line from the file
        std::string line;
        inFile >> line;

        removeAfterNewLine(line);

        if (line.compare(0, strlen("low_revision="), "low_revision=") == 0)
        {
            rev_info.low_revision = line.substr(strlen("low_revision="));
            done_stuff = true;
        }
        else if (line.compare(0, strlen("revision="), "revision=") == 0)
        {
            rev_info.revision = line.substr(strlen("revision="));
            done_stuff = true;
        }
        else if (line.compare(0, strlen("date="), "date=") == 0)
        {
            rev_info.date = line.substr(strlen("date="));
            done_stuff = true;
        }
        else if (line.compare(0, strlen("wc_uri="), "wc_uri=") == 0)
        {
            rev_info.wc_uri = line.substr(strlen("wc_uri="));
            done_stuff = true;
        }
        else if (line.compare(0, strlen("wc_modified="), "wc_modified=") == 0)
        {
            std::string bool_val = line.substr(strlen("wc_modified="));

            if (bool_val.find("true") != std::string::npos
             || bool_val.find('1') != std::string::npos)
                rev_info.wc_modified = true;
            else
                rev_info.wc_modified = false;

            done_stuff = true;
        }
        else if (line.compare(0, strlen("wc_switched="), "wc_switched=") == 0)
        {
            std::string bool_val = line.substr(strlen("wc_switched="));

            if (bool_val.find("true") != std::string::npos
             || bool_val.find('1') != std::string::npos)
                rev_info.wc_switched = true;
            else
                rev_info.wc_switched = false;

            done_stuff = true;
        }
    }

    if (done_stuff)
        return true;
    else
        return RevisionExtractor::extractRevision(rev_info);
}

bool WriteOutput(const string& outputFile, const RevisionInformation& rev_info)
{
    std::stringstream comment_str;

    comment_str << "/*";
    if (!rev_info.low_revision.empty()
     && rev_info.low_revision != rev_info.revision)
        comment_str << rev_info.low_revision << ":";

    comment_str << rev_info.revision;

    if (rev_info.wc_modified)
        comment_str << "M";

    if (rev_info.wc_switched)
        comment_str << "S";

    comment_str << "*/";

    string comment(comment_str.str());

    // Check whether this file already contains the correct revision date. Don't
    // modify it if it does, we don't want to go messing with it's timestamp for
    // no gain.
    {
        ifstream in(outputFile.c_str());
        if (!in.bad() && !in.eof() && in.is_open())
        {
            string old;
            in >> old;
            if(old == comment)
            {
                if(be_verbose)
                    cout << "Revision unchanged (" << rev_info.revision << "). Skipping.\n"
                         << "old = \"" << old.substr(2, old.length() - 4) << "\"; new = \"" << comment.substr(2, comment.length() - 4) << "\"\n";

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

    header << "\n#define SVN_LOW_REV      " << (rev_info.low_revision.empty() ? rev_info.revision : rev_info.low_revision)
           << "\n#define SVN_LOW_REV_STR \"" << (rev_info.low_revision.empty() ? rev_info.revision : rev_info.low_revision) << "\""
           << "\n#define SVN_REV          " << rev_info.revision
           << "\n#define SVN_REV_STR     \"" << rev_info.revision << "\""
           << "\n#define SVN_DATE        \"" << rev_info.date << "\""
           << "\n#define SVN_URI         \"" << rev_info.wc_uri << "\"\n";

    header << "\n#define SVN_WC_MODIFIED " << rev_info.wc_modified
           << "\n#define SVN_WC_SWITCHED " << rev_info.wc_switched << "\n\n";

    // Open namespace
    if(do_int || do_std || do_wx)
        header << "namespace autorevision\n{\n";

    if(do_int)
        header << "\tSVN_AUTOREVISION_STATIC const unsigned int svn_low_revision = " << rev_info.low_revision << ";\n"
               << "\tSVN_AUTOREVISION_STATIC const unsigned int svn_revision = " << rev_info.revision << ";\n";

    if(do_std)
        header << "\tSVN_AUTOREVISION_STATIC const std::string svn_low_revision_s(\"" << rev_info.low_revision << "\");\n"
               << "\tSVN_AUTOREVISION_STATIC const std::string svn_revision_s(\"" << rev_info.revision << "\");\n"
               << "\tSVN_AUTOREVISION_STATIC const std::string svn_date_s(\"" << rev_info.date << "\");\n"
               << "\tSVN_AUTOREVISION_STATIC const std::string svn_uri_s(\"" << rev_info.wc_uri << "\");\n";
    if(do_cstr)
        header << "\tSVN_AUTOREVISION_STATIC const char svn_low_revision_cstr[] = \"" << rev_info.low_revision << "\";\n"
               << "\tSVN_AUTOREVISION_STATIC const char svn_revision_cstr[] = \"" << rev_info.revision << "\";\n"
               << "\tSVN_AUTOREVISION_STATIC const char svn_date_cstr[] = \"" << rev_info.date << "\";\n"
               << "\tSVN_AUTOREVISION_STATIC const char svn_uri_cstr[] = \"" << rev_info.wc_uri << "\";\n";
    if(do_wx)
    {
        header << "\tSVN_AUTOREVISION_STATIC const wxString svnLowRevision(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << rev_info.low_revision << "\"));\n"

               << "\tSVN_AUTOREVISION_STATIC const wxString svnRevision(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << rev_info.revision << "\"));\n"

               << "\tSVN_AUTOREVISION_STATIC const wxString svnDate(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << rev_info.date << "\"));\n"

               << "\tSVN_AUTOREVISION_STATIC const wxString svnUri(";

        if(do_translate)
            header << "wxT";

        header << "(\"" << rev_info.wc_uri << "\"));\n";
    }

    // Terminate/close namespace
    if(do_int || do_std || do_wx)
        header << "}\n\n";

    header << "\n\n#endif\n";

    return true;
}
