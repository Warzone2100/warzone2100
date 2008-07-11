#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

my $scriptpath = $0;
$scriptpath =~ s/^(.*\/)?([^\/]+)$/$1/g;

my $out_lang = $scriptpath;
$out_lang .= shift or die "Missing output language";
$out_lang .= "_cg.pm";
require $out_lang or die "Couldn't load $out_lang";

my %enumMap;
my @enumList;
my %structMap;
my @structList;

sub parseEnum
{
    my ($enumName, $comment, $count) = @_;
    my %curEnum = (name => $enumName);
    my @curComment = ();

    @{$curEnum{"comment"}} = @$comment;
    @$comment = ();
    $$count++;

    while (<>)
    {
        chomp;

        if    (/^\s*(\#.*)?$/)
        {
            push @curComment, substr($1, 1) if $1;
        }
        # Parse struct-level qualifiers
        elsif (/^\s*%(.*)\s*$/)
        {
            die "error: Cannot give enum-level qualifiers after defining fields" if exists($curEnum{"values"});

            $_ = $1;

            if    (/^valprefix\s+\"([^\"]*)\"\s*;$/)
            {
                ${$curEnum{"qualifiers"}}{"valprefix"} = $1;
            }
            elsif (/^valsuffix\s+\"([^\"]+)\"\s*;$/)
            {
                ${$curEnum{"qualifiers"}}{"valsuffix"} = $1;
            }
            elsif (/^max\s+\"([^\"]+)\"\s*;$/)
            {
                ${$curEnum{"qualifiers"}}{"max"} = $1;
            }
            else
            {
                die "error: line $count: Unrecognized enum-level specifier: %$_";
            }
        }
        elsif (/^\s*(\w+)\s*$/)
        {
            my %value = (name=>$1);

            @{$value{"comment"}} = @curComment;
            @curComment = ();

            push @{$curEnum{"values"}}, \%value;
        }
        elsif (/^\s*end\s*;\s*$/)
        {
            # Put the enum at the end of the array
            push @enumList, \%curEnum;

            # Also place it in the hash
            $enumMap{$curEnum{"name"}} = \%curEnum;
            return;
        }
        else            { die "Unmatched line: $_\n"; }

        $$count++;
    }
}

sub readTillEnd
{
    my ($output, $count) = @_;

    $$count++;

    while (<>)
    {
        chomp;

        # See if we've reached the end of this block
        if (/^\s*end\s*;\s*$/)
        {
            return;
        }

        push @$output, $_;
        $$count++;
    }
}

sub parseStruct
{
    my ($structName, $comment, $count) = @_;
    my %curStruct = (name => $structName);
    my @curComment = ();

    @{$curStruct{"comment"}} = @$comment;
    @$comment = ();
    $$count++;

    while (<>)
    {
        chomp;

        if    (/^\s*(\#.*)?$/)
        {
            push @curComment, substr($1, 1) if $1;
        }
        elsif (/^\s*end\s*;\s*$/)
        {
            # Put the struct at the end of the array
            push @structList, \%curStruct;

            # Also place it in the hash
            $structMap{$curStruct{"name"}} = \%curStruct;

            return;
        }
        # Parse struct-level qualifiers
        elsif (/^\s*%(.*)\s*$/)
        {
            die "error: Cannot give struct-level qualifiers after defining fields" if exists($curStruct{"fields"});

            $_ = $1;

            if    (/^prefix\s+\"([^\"]+)\"\s*;$/)
            {
                ${$curStruct{"qualifiers"}}{"prefix"} = $1;
            }
            elsif (/^suffix\s+\"([^\"]+)\"\s*;$/)
            {
                ${$curStruct{"qualifiers"}}{"suffix"} = $1;
            }
            elsif (/^macro\s*;$/)
            {
                ${${$curStruct{"qualifiers"}}{"macro"}}{"has"} = 1;
            }
            elsif (/^nomacro\s*;$/)
            {
                ${${$curStruct{"qualifiers"}}{"macro"}}{"has"} = 0;
            }
            elsif (/^macroprefix\s+\"([^\"]+)\"\s*;$/)
            {
                ${${$curStruct{"qualifiers"}}{"macro"}}{"prefix"} = $1;
            }
            elsif (/^macrosuffix\s+\"([^\"]+)\"\s*;$/)
            {
                ${${$curStruct{"qualifiers"}}{"macro"}}{"suffix"} = $1;
            }
            elsif (/^loadFunc\s+\"([^\"]+)\"\s*;$/)
            {
                my %loadFunc = (name=>$1, line=>$$count);

                ${$curStruct{"qualifiers"}}{"loadFunc"} = \%loadFunc;
            }
            elsif (/^abstract\s*;$/)
            {
                die "error: Cannot declare a struct \"abstract\" if it inherits from another" if exists(${$curStruct{"qualifiers"}}{"inherit"});

                ${$curStruct{"qualifiers"}}{"abstract"} = 1;
            }
            elsif (/^inherit\s+(\w+)\s*;$/)
            {
                die "error: structs declared \"abstract\" cannot inherit" if exists(${$curStruct{"qualifiers"}}{"abstract"});
                die "error: Cannot inherit from struct \"$1\" as it isn't (fully) declared yet" unless exists($structMap{$1});

                ${$curStruct{"qualifiers"}}{"inherit"} = \%{$structMap{$1}};
            }
            elsif (/^preLoadTable(\s+maxId)?(\s+rowCount)?\s*$/)
            {
                my %preLoadTable = (line=>$$count);

                push @{$preLoadTable{"parameters"}}, $1 if $1;
                push @{$preLoadTable{"parameters"}}, $2 if $2;

                readTillEnd(\@{$preLoadTable{"code"}}, $count);

                ${$curStruct{"qualifiers"}}{"preLoadTable"} = \%preLoadTable;
            }
            elsif (/^postLoadRow\s+curRow(\s+curId)?\s*$/)
            {
                my %postLoadRow = (line=>$$count);

                push @{$postLoadRow{"parameters"}}, $1 if $1;

                readTillEnd(\@{$postLoadRow{"code"}}, $count);

                ${$curStruct{"qualifiers"}}{"postLoadRow"} = \%postLoadRow;
            }
            else
            {
                die "error: line $count: Unrecognized struct-level specifier: %$_";
            }
        }
        # Parse regular field declarations
        elsif (/^\s*(count|real|bool)\s+(unique\s+)?(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, name=>$3);

            push @{$field{"qualifiers"}}, $2 if $2;

            @{$field{"comment"}} = @curComment;
            @curComment = ();

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse field declarations with "transition" types (types that should be replaced with others eventually)
        elsif (/^\s*([US]D?WORD|[US]BYTE)\s+(unique\s+)?(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, name=>$3);

            push @{$field{"qualifiers"}}, $2 if $2;

            @{$field{"comment"}} = @curComment;
            @curComment = ();

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse field declarations for types that can have optional values.
        # Meaning they can be NULL in C.
        elsif (/^\s*(string|IMD_model)\s+(unique\s+)?(optional\s+)?(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, name=>$4);

            push @{$field{"qualifiers"}}, $2 if $2;
            push @{$field{"qualifiers"}}, $3 if $3;

            @{$field{"comment"}} = @curComment;
            @curComment = ();

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse set and enum field declarations
        elsif (/^\s*(set|enum)\s+(\w+)\s+(unique\s+)?(\w+)\s*;\s*$/)
        {
            die "error: Cannot use enum \"$2\" as it isn't declared yet" unless exists($enumMap{$2});

            my %field = (type=>$1, enum=>$enumMap{$2}, name=>$4);

            push @{$field{"qualifiers"}}, $3 if $3;

            @{$field{"comment"}} = @curComment;
            @curComment = ();

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse C-only fields
        elsif (/^\s*(C-only-field)\s+(.*)\s+(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, ctype=>$2, name=>$3);

            @{$field{"comment"}} = @curComment;
            @curComment = ();

            push @{$curStruct{"fields"}}, \%field;
        }
        else            { die "Unmatched line: $_\n"; }

        $$count++;
    }
}

CG::processCmdLine(\@ARGV);

my @curComment = ();

# Read and parse the file
my $name = $ARGV[0];
my $count = 1;
my $outfile = "";
$outfile = pop(@ARGV) if @ARGV > 1;
while (<>)
{
    chomp;

    # Process blank lines, possibly with comments on them
    if    (/^\s*(\#.*)?$/)
    {
        push @curComment, substr($1, 1) if $1;
    }
    elsif (/^\s*struct\s+(\w+)\s*$/)                                            { parseStruct($1, \@curComment, \$count); }
    elsif (/^\s*enum\s+(\w+)\s*$/)                                              { parseEnum($1, \@curComment, \$count); }
    else            { die "Unmatched line: $_\n"; }

    $count++;
}

my $output = "";

CG::startFile(\$output, $name, $outfile);

# Print all enums
foreach my $enum (@enumList)
{
    CG::printEnum(\$output, $enum);
}

# Print all structs
foreach my $struct (@structList)
{
    CG::printStruct(\$output, $struct, \%structMap, \%enumMap);
}

CG::endFile(\$output, $name);

# Actually print out the output
if ($outfile)
{
    open (OUT, ">$outfile");
    print OUT $output;
    close(OUT);
}
else
{
    print $output;
}
