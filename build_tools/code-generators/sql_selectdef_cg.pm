#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for SQL table definitions

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
    my $structName = ${$struct}{"name"};

    while (@fields)
    {
        my $field = shift(@fields);

        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enumName = ${$field}{"enum"};
            my $enum = ${$enumMap}{$enumName};
            my @values = @{${$enum}{"values"}};

            while (@values)
            {
                my $value = shift(@values);

                print "`$structName`.`${$field}{\"name\"}_${$value}{\"name\"}`";
                print ", " if @values;
            }
        }
        else
        {
            print "`$structName`.`${$field}{\"name\"}`";
        }

        print ", " if @fields;
    }
}

sub printStruct
{
    my ($struct, $structMap, $enumMap, $first) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printStruct($inheritStruct, $structMap, $enumMap, 0);
        }
        elsif (/abstract/)
        {
            print "`${$struct}{\"name\"}`.`unique_inheritance_id`, ";
        }
    }

    printStructFields($struct, $enumMap);
    print ", " unless $first;
}

sub printBaseStruct
{
    my ($struct, $structMap) = @_;
    my $is_base = 1;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printBaseStruct($inheritStruct, $structMap);
            $is_base = 0;
        }
        elsif (/abstract/)
        {
            print "`${$struct}{\"name\"}`";
            $is_base = 0;
        }
    }

    print "`${$struct}{\"name\"}`" if $is_base;
}

sub printStructJoins
{
    my ($struct, $structMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printStructJoins($inheritStruct, $structMap);
            print " INNER JOIN `${$struct}{\"name\"}` ON `${inheritName}`.`unique_inheritance_id` = `${$struct}{\"name\"}`.`unique_inheritance_id`";
        }
    }
}

sub printEnums()
{
}

sub printStructs()
{
    my ($structList, $structMap, $enumMap) = @_;
    my @structs = @{$structList};

    while (@structs)
    {
        my $struct = shift(@structs);

        printComments(${$struct}{"comment"}, 0);

        # Start printing the select statement
        print "SELECT ";

        printStruct($struct, $structMap, $enumMap, 1);
        print " FROM ";

        printBaseStruct($struct, $structMap);
        printStructJoins($struct, $structMap);

        print ";\n";
        print "\n" if @structs;
    }
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
