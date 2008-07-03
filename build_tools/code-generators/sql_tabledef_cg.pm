#!/usr/bin/perl -w

package CG;
use strict;

# Code generator for SQL table definitions

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
    print "CREATE TABLE `${name}` (\n";

    # Process struct qualifiers
    foreach (@qualifiers)
    {
        if    (/^prefix\s+\"([^\"]+)\"$/) {}
        elsif (/^abstract$/)
        {
            print "\t-- Automatically generated ID to link the inheritance hierarchy.\n"
                 ."\tunique_inheritance_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n\n";
        }
        else
        {
            die "Unknown qualifier: `$_'\n";
        }
    }

    # Print fields
    while (@fields)
    {
        print pop(@fields);

        # Seperate field defintions by commas and blank lines
        print ",\n" if @fields;

        # Make sure to terminate lines of field definitions
        print "\n";
    }

    print ");\n";
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
    if    (/count/)     { $type = "INTEGER NOT NULL"; }
    elsif (/string/)    { $type = "TEXT NOT NULL"; }
    elsif (/real/)      { $type = "NUMERIC NOT NULL"; }
    elsif (/bool/)      { $type = "INTEGER NOT NULL"; }
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

    while (@curComment)
    {
        $fieldDecl .= "\t--" . shift(@curComment) . "\n";
    }
    
    $fieldDecl .= "\t${name} ${type}";

    push @fieldDecls, $fieldDecl;
    $nestFieldCount[@nestFieldCount - 1]++;
}

sub pushComment()
{
    push @curComment, substr($_[0], 1);
}

1;
