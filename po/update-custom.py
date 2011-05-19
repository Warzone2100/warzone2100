#!/usr/bin/env python
import json
import codecs
import polib
import glob
import re

from iniparse import INIConfig

from os import makedirs
from os.path import exists as pathexists, dirname

def getPOFiles():
    pofiles = {}

    files = glob.glob('*.po')
    files.sort()

    for f in files:
        pofiles[f.split(".")[0]] = polib.pofile(f)

    return pofiles


def pullMac(pFile):
    result = {}

    with codecs.open(pFile["file"], "r+b", "utf-8") as fp:
        content = fp.read()

        for (string, quoted) in pFile["strings"]:
            if not quoted:
                m = re.search('%s\s+=\s+"(.*)"\s?;' % string, content)
            else:
                m = re.search('"%s"\s+=\s+"(.*)"\s?;' % string, content)

            if m:
                result[string] = m.group(1)

    return result

def pushMac(pFile, pofiles):
    english = pullMac(pFile)

    for lang, po in pofiles.iteritems():
        path = pFile["pushpath"] % lang

        if not pathexists(path):
            makedirs(dirname(path))

        with codecs.open(path, "w", "utf-8") as fp:
            for (string, quoted) in pFile["strings"]:
                poEntry = po.find(english[string])

                if (poEntry and
                  not "fuzzy" in poEntry.flags and
                  poEntry.msgstr != ""):
                    translated = poEntry.msgstr
                else:
                    translated = english[string]

                if not quoted:
                    fp.write("%s = \"%s\";\n" % (string, translated))
                else:
                    fp.write("\"%s\" = \"%s\";\n" % (string, translated))


def pullINI(pFile):
    result = {}

    with codecs.open(pFile["file"], "r", "utf-8") as fp:
        ini = INIConfig(fp)

        for (section, entry) in pFile["strings"]:
            result["%s_%s" % (section, entry)] = ini[section][entry]

    return result


def pushINI(pFile, pofiles):
    with codecs.open(pFile["file"], "r", "utf-8") as fp:
        ini = INIConfig(fp)

        for (section, entry) in pFile["strings"]:
            value = ini[section][entry]

            for lang, po in pofiles.iteritems():
                poEntry = po.find(value)

                if (poEntry and
                  not "fuzzy" in poEntry.flags and
                  poEntry.msgstr != ""):
                    ini[section]["%s[%s]" % (entry, lang)] = poEntry.msgstr

        with codecs.open(pFile["file"], "w+b", "utf-8") as wFp:
            wFp.write(unicode(ini))


if __name__ == '__main__':
    import sys

    if (len(sys.argv) != 2 or
      not sys.argv[1].lower() in ('pull', 'push', 'filelist')):
        print "Usage: %s <push|pull|filelist>" % sys.argv[0]
        exit(1)

    with open("custom/files.js", "r+b") as fp:
        files = json.load(fp, "utf-8")

        if (sys.argv[1].lower() == 'pull'):
            for pFile in files:
                print "Reading %s:%s" % (pFile["type"], pFile["file"])

                strings = {}
                if pFile["type"] == "ini":
                    strings = pullINI(pFile)
                elif pFile["type"] == "mac":
                    strings = pullMac(pFile)

                if strings:
                    with codecs.open("custom/%s" % pFile["output"], "w", "utf-8") as sFp:
                        sFp.write("%s%s%s" % ("_(\"", "\")\n_(\"".join(strings.values()), "\")\n"))

        elif (sys.argv[1].lower() == 'push'):
            pofiles = getPOFiles()

            for pFile in files:
                if not pFile["push"]:
                    continue

                if pFile["type"] == "ini":
                    pushINI(pFile, pofiles)
                elif pFile['type'] == "mac":
                    pushMac(pFile, pofiles)


        elif (sys.argv[1].lower() == "filelist"):
            for pFile in files:
                print "po/custom/%s" % pFile["output"]

        else:
            print "Unknown command \"%s\"" % sys.argv[1]
            exit(1)
