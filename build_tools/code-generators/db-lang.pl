#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

my $out_lang = shift or die "Missing output language";
require $out_lang or die "Couldn't load $out_lang";

my %enumMap;
my @enumList;
my %structMap;
my @structList;

sub parseEnum
{
    my ($enumName, $comment) = @_;
    my %curEnum = (name => $enumName);
    my @curComment = ();

    @{$curEnum{"comment"}} = @$comment;
    @$comment = ();

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
                die "error: $ARGV:$.: Unrecognized enum-level specifier: %$_";
            }
        }
        elsif (/^\s*(\w+)\s*$/)
        {
            my %value = (name=>$1, line=>$.);

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
    }
    continue
    {
        # Reset the line number if we've reached the end of the file
        close ARGV if eof;
    }
}

sub readTillEnd
{
    my ($output) = @_;

    while (<>)
    {
        chomp;

        # See if we've reached the end of this block
        if (/^\s*end\s*;\s*$/)
        {
            return;
        }

        push @$output, $_;
    }
    continue
    {
        # Reset the line number if we've reached the end of the file
        close ARGV if eof;
    }
}

sub parseStruct
{
    my ($structName, $comment) = @_;
    my %curStruct = (name => $structName);
    my @curComment = ();

    my $curCSVfield = 0;

    @{$curStruct{"comment"}} = @$comment;
    @$comment = ();

LINE:
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
            $_ = $1;

            # See if it's really a field-level qualifier
            if    (/csv-field\s+(\d+|last)\s*;/)
            {
                $curCSVfield = $1;
                next LINE;
            }
            else
            {
                die "error: Cannot give struct-level qualifiers after defining fields" if exists($curStruct{"fields"});
            }

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
                my %loadFunc = (name=>$1, line=>$.);

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
            elsif (/^fetchRowById\s+Row\s+Id\s*$/)
            {
                my %fetchRowById = (line=>$.);

                readTillEnd(\@{$fetchRowById{"code"}});

                ${$curStruct{"qualifiers"}}{"fetchRowById"} = \%fetchRowById;
            }
            elsif (/^preLoadTable(\s+maxId)?(\s+rowCount)?\s*$/)
            {
                my %preLoadTable = (line=>$.);

                push @{$preLoadTable{"parameters"}}, $1 if $1;
                push @{$preLoadTable{"parameters"}}, $2 if $2;

                readTillEnd(\@{$preLoadTable{"code"}});

                ${$curStruct{"qualifiers"}}{"preLoadTable"} = \%preLoadTable;
            }
            elsif (/^postLoadRow\s+curRow(\s+curId)?(\s+rowNum)?\s*$/)
            {
                my %postLoadRow = (line=>$.);

                push @{$postLoadRow{"parameters"}}, $1 if $1;
                push @{$postLoadRow{"parameters"}}, $2 if $2;

                readTillEnd(\@{$postLoadRow{"code"}});

                ${$curStruct{"qualifiers"}}{"postLoadRow"} = \%postLoadRow;
            }
            elsif (/^csv-file\s+"((?:[^"]+|\\")+)"\s*;$/)
            {
                $1 =~ s/\\"/"/g;

                ${$curStruct{"qualifiers"}}{"csv-file"} = $1;
            }
            else
            {
                die "error: line $ARGV:$.: Unrecognized struct-level specifier: %$_";
            }
        }
        # Parse regular field declarations
        elsif (/^\s*(count|real|bool)\s+(unique\s+)?(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, name=>$3, line=>$.);

            push @{$field{"qualifiers"}}, $2 if $2;

            @{$field{"comment"}} = @curComment;
            @curComment = ();
            $field{"CSV"} = $curCSVfield;
            $curCSVfield = 0;

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse field declarations with "transition" types (types that should be replaced with others eventually)
        elsif (/^\s*([US]D?WORD|[US]BYTE)\s+(unique\s+)?(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, name=>$3, line=>$.);

            push @{$field{"qualifiers"}}, $2 if $2;

            @{$field{"comment"}} = @curComment;
            @curComment = ();
            $field{"CSV"} = $curCSVfield;
            $curCSVfield = 0;

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse field declarations for types that can have optional values.
        # Meaning they can be NULL in C.
        elsif (/^\s*(string|IMD_model)\s+(unique\s+)?(optional\s+)?(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, name=>$4, line=>$.);

            push @{$field{"qualifiers"}}, $2 if $2;
            push @{$field{"qualifiers"}}, $3 if $3;

            @{$field{"comment"}} = @curComment;
            @curComment = ();
            $field{"CSV"} = $curCSVfield;
            $curCSVfield = 0;

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse set and enum field declarations
        elsif (/^\s*(set|enum)\s+(\w+)\s+(unique\s+)?(\w+)\s*;\s*$/)
        {
            die "error: Cannot use enum \"$2\" as it isn't declared yet" unless exists($enumMap{$2});

            my %field = (type=>$1, enum=>$enumMap{$2}, name=>$4, line=>$.);

            push @{$field{"qualifiers"}}, $3 if $3;

            @{$field{"comment"}} = @curComment;
            @curComment = ();
            $field{"CSV"} = $curCSVfield;
            $curCSVfield = 0;

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse struct reference declarations
        elsif (/^\s*(struct)\s+(\w+)\s+(unique\s+)?(optional\s+)?(\w+)\s*;\s*$/)
        {
            die "error: $ARGV:$.: Cannot use struct \"$2\" as it isn't declared yet" unless exists($structMap{$2});
            die "error: $ARGV:$.: Cannot use struct \"$2\" as it doesn't have a fetchRowById function declared" unless exists(${${$structMap{$2}}{"qualifiers"}}{"fetchRowById"});

            my %field = (type=>$1, struct=>$structMap{$2}, name=>$5, line=>$.);

            push @{$field{"qualifiers"}}, $3 if $3;
            push @{$field{"qualifiers"}}, $4 if $4;

            @{$field{"comment"}} = @curComment;
            @curComment = ();
            $field{"CSV"} = $curCSVfield;
            $curCSVfield = 0;

            push @{$curStruct{"fields"}}, \%field;
        }
        # Parse C-only fields
        elsif (/^\s*(C-only-field)\s+(.*)\s+(\w+)\s*;\s*$/)
        {
            my %field = (type=>$1, ctype=>$2, name=>$3, line=>$.);

            @{$field{"comment"}} = @curComment;
            @curComment = ();
            $field{"CSV"} = $curCSVfield;
            $curCSVfield = 0;

            push @{$curStruct{"fields"}}, \%field;
        }
        else            { die "Unmatched line: $_\n"; }
    }
    continue
    {
        # Reset the line number if we've reached the end of the file
        close ARGV if eof;
    }
}

CG::processCmdLine(\@ARGV);

my @curComment = ();

# Read and parse the file
my $name = $ARGV[0];
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
    elsif (/^\s*struct\s+(\w+)\s*$/)                                            { parseStruct($1, \@curComment); }
    elsif (/^\s*enum\s+(\w+)\s*$/)                                              { parseEnum($1, \@curComment); }
    else            { die "Unmatched line: $_\n"; }
}
continue
{
    # Reset the line number if we've reached the end of the file
    close ARGV if eof;
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
