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

        printComments ${$field}{"comment"}, 1;

        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enumName = ${$field}{"enum"};
            my $enum = ${$enumMap}{$enumName};
            my @values = @{${$enum}{"values"}};

            while (@values)
            {
                my $value = shift(@values);

                print "\t`$structName`.`${$field}{\"name\"}_${$value}{\"name\"}`";
                print ",\n" if @values;
            }
        }
        else
        {
            print "\t`$structName`.`${$field}{\"name\"}`";
        }

        print ",\n\n" if @fields;
    }
}

sub printStructContent
{
    my ($struct, $structMap, $enumMap, $first) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printStructContent($inheritStruct, $structMap, $enumMap, 0);
        }
        elsif (/abstract/)
        {
            print "`${$struct}{\"name\"}`.`unique_inheritance_id`,\n\n";
        }
    }

    printStructFields($struct, $enumMap);
    print ",\n" unless $first;
    print "\n";
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

sub printEnum()
{
}

sub printStruct()
{
    my ($struct, $structMap, $enumMap) = @_;

    printComments(${$struct}{"comment"}, 0);

    # Start printing the select statement
    print "SELECT ";

    printStructContent($struct, $structMap, $enumMap, 1);
    print "FROM ";

    printBaseStruct($struct, $structMap);
    printStructJoins($struct, $structMap);

    print ";\n\n";
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
