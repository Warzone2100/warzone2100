#!/usr/bin/perl -w

package CG;
use strict;

# Code generator for C struct definitions

my @structQualifierCount;
my @structQualifiers;
my @nestFieldCount;
my @fieldDecls;
my @nestNames;
my @curComment;

sub blankLine()
{
    print "\n";
}

sub beginStructDef()
{
    push @nestNames, $_[0];
    push @nestFieldCount, 0;
    push @structQualifierCount, 0;
}

sub endStructDef()
{
    my $name = pop(@nestNames);
    my $fieldCount = pop(@nestFieldCount);
    my $qualifierCount = pop(@structQualifierCount);

    # Fetch qualifier list from stack and reverse it
    my @qualifiers;
    while ($qualifierCount)
    {
        push @qualifiers, pop(@structQualifiers);

        $qualifierCount--;
    }

    # Fetch field list from stack and reverse it
    my @fields;
    while ($fieldCount)
    {
        push @fields, pop(@fieldDecls);

        $fieldCount--;
    }

    # Start printing the structure
    print "typedef struct\n{\n";

    # Process struct qualifiers
    foreach (@qualifiers)
    {
        if    (/^prefix\s+\"([^\"]+)\"$/)
        {
            $name = $1 . $name;
        }
        elsif (/^abstract$/) {}
        else
        {
            die "Unknown qualifier: `$_'\n";
        }
    }

    # Print fields
    while (@fields)
    {
        print pop(@fields) . "\n";

        # Seperate field defintions by blank lines
        print "\n" if @fields;
    }

    print "} ${name};\n";
}

sub addStructQualifier()
{
    push @structQualifiers, $_[0];
    $structQualifierCount[@structQualifierCount - 1]++;
}

sub fieldDeclaration()
{
    my ($type, $qualifier, $name) = @_;

    my $fieldDecl = "";

    $_ = $type;
    if    (/count/)     { $type = "unsigned int     "; }
    elsif (/string/)    { $type = "const char*      "; }
    elsif (/real/)      { $type = "float            "; }
    elsif (/bool/)      { $type = "bool             "; }
    else                { die "UKNOWN TYPE: $_"; }

    my $set = "";

    if ($qualifier)
    {
        foreach ($qualifier)
        {
            if    (/set/)
            {
                $set = "\[]";
            }
            elsif (/unique/)
            {
                # Separate this notice from the rest of the comment if there's any
                push @curComment, "" if @curComment;
                
                push @curComment, " Unique across all instances";
            }
            else
            {
                die "UNKNOWN QUALIFIER: $_";
            }
        }
    }

    # If there's a comment, "open" it
    $fieldDecl .= "\t/**" if @curComment;

    while (@curComment)
    {
        $fieldDecl .= shift(@curComment) . "\n";

        if (@curComment)
        {
            $fieldDecl .= "\t * ";
        }
        else
        {
            $fieldDecl .= "\t */\n";
        }
    }
    
    $fieldDecl .= "\t${type}${name}${set};";

    push @fieldDecls, $fieldDecl;
    $nestFieldCount[@nestFieldCount - 1]++;
}

sub pushComment()
{
    push @curComment, substr($_[0], 1);
}

1;
