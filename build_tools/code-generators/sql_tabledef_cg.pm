#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for SQL table definitions

sub printStructFieldType
{
    my ($field) = @_;

    $_ = ${$field}{"type"};

    if    (/count/)     { print "INTEGER NOT NULL"; }
    elsif (/string/)    { print "TEXT NOT NULL"; }
    elsif (/real/)      { print "NUMERIC NOT NULL"; }
    elsif (/bool/)      { print "INTEGER NOT NULL"; }
    elsif (/enum/)      { print "INTEGER NOT NULL"; }
    else                { die "UKNOWN TYPE: $_"; }
}

sub printComments
{
    my ($comments, $indent) = @_;

    return unless @{$comments};

    foreach my $comment (@{$comments})
    {
        print "\t" if $indent;
        print "--${comment}\n";
    }
}

sub printStructFields
{
    my ($struct, $enumMap) = @_;
    my @fields = @{${$struct}{"fields"}};
    my @constraints = ();

    while (@fields)
    {
        my $field = shift(@fields);

        printComments ${$field}{"comment"}, 1;

        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enumName = ${$field}{"enum"};
            my $enum = ${$enumMap}{$enumName};
            my @values = @{${$enum}{"values"}};
            my $unique_string = "UNIQUE(";

            while (@values)
            {
                my $value = shift(@values);

                print "\t${$field}{\"name\"}_${$value}{\"name\"} INTEGER NOT NULL";
                print "," if @values or @fields or @constraints or (${$field}{"qualifier"} and ${$field}{"qualifier"} =~ /unique/);
                print "\n";
                $unique_string .= "${$field}{\"name\"}_${$value}{\"name\"}";
                $unique_string .= ", " if @values;
            }

            $unique_string .= ")";
            push @constraints, $unique_string if ${$field}{"qualifier"} and ${$field}{"qualifier"} =~ /unique/;
        }
        else
        {
            print "\t${$field}{\"name\"} ";
            printStructFieldType($field);
            print " UNIQUE" if ${$field}{"qualifier"} and ${$field}{"qualifier"} =~ /unique/;
            print ",\n" if @fields or @constraints;
        }

        print "\n";
    }

    while (@constraints)
    {
        my $constraint = shift(@constraints);

        print "\t${constraint}";
        print ",\n" if @constraints;
        print "\n";
    }
}

sub printStructContent
{
    my ($struct, $name, $structMap, $enumMap, $printFields) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printStructContent($inheritStruct, $name, $structMap, $enumMap, 0);
        }
        elsif (/abstract/)
        {
            print "\t-- Automatically generated ID to link the inheritance hierarchy.\n"
                 ."\tunique_inheritance_id INTEGER PRIMARY KEY ";
            print "AUTOINCREMENT " if $printFields;
            print "NOT NULL,\n\n";
        }
    }

    $$name = ${$struct}{"name"};

    printStructFields($struct, $enumMap) if $printFields;
}

sub printEnum()
{
}

sub printStruct()
{
    my ($struct, $structMap, $enumMap) = @_;
    my $name = ${$struct}{"name"};

    printComments(${$struct}{"comment"}, 0);

    # Start printing the structure
    print "CREATE TABLE `${name}` (\n";

    printStructContent($struct, \$name, $structMap, $enumMap, 1);

    # Finish printing the structure
    print ");\n\n";
}

sub startFile()
{
    my ($name) = @_;

    print "-- This file is generated automatically, do not edit, change the source ($name) instead.\n\n";
}

sub endFile()
{
}

1;
