#!/usr/bin/perl -w

my $out_lang = shift or die "Missing output language";
$out_lang .= "_cg.pm";
require $out_lang or die "Couldn't load $out_lang";

# Read and parse the file
my $name;
while (<>)
{
    chomp;

    if    (/^\s*(\#.*)?$/)                                                      { CG::blankLine(); if ($1) { CG::pushComment($1); } }
    elsif (/^\s*struct\s+(\w+)\s*$/)                                            { CG::beginStructDef($1); }
    elsif (/^\s*end\s+struct\s*;\s*$/)                                          { CG::endStructDef()      }
    elsif (/^\s*%(.*)\s*;\s*$/)                                                 { CG::addStructQualifier($1); }
    elsif (/^\s*(count|string|real|bool)\s+(unique\s+|set\s+)?(\w+)\s*;\s*$/)   { CG::fieldDeclaration($1, $2, $3); }
    else            { print "Unmatched line: $_\n"; }
}
