#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

my $out_lang = shift or die "Missing output language";
$out_lang .= "_cg.pm";
require $out_lang or die "Couldn't load $out_lang";

my %enumMap;
my @enumList;
my %structMap;
my @structList;

sub parseEnum
{
    my %curEnum = (name => $_[0]);
    my @curComment = ();

    @{$curEnum{"comment"}} = @{$_[1]};
    @{$_[1]} = ();

    while (<>)
    {
        chomp;

        if    (/^\s*(\#.*)?$/)
        {
            push @curComment, substr($1, 1) if $1;
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
    }
}

sub parseStruct
{
    my %curStruct = (name => $_[0]);
    my @curComment = ();

    @{$curStruct{"comment"}} = @{$_[1]};
    @{$_[1]} = ();

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
        elsif (/^\s*%(.*)\s*;\s*$/)
        {
            $_ = $1;

            if    (/^prefix\s+\"([^\"]+)\"$/)
            {
                ${$curStruct{"qualifiers"}}{"prefix"} = $1;
            }
            elsif (/^abstract$/)
            {
                ${$curStruct{"qualifiers"}}{"abstract"} = 1;
            }
            elsif (/^inherit\s+(\w+)$/)
            {
                ${$curStruct{"qualifiers"}}{"inherit"} = $1;
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
            my %field = (type=>$1, enum=>$2, name=>$4);

            push @{$field{"qualifiers"}}, $3 if $3;

            @{$field{"comment"}} = @curComment;
            @curComment = ();

            push @{$curStruct{"fields"}}, \%field;
        }
        else            { die "Unmatched line: $_\n"; }
    }
}

my @curComment = ();

# Read and parse the file
my $name = $ARGV[0];
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

my $output = "";

CG::startFile(\$output, $name);

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
print $output;
