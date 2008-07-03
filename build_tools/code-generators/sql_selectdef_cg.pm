#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for SQL table definitions

sub printComments
{
    my ($output, $comments, $indent) = @_;

    return unless @{$comments};

    foreach my $comment (@{$comments})
    {
        $$output .= "\t" if $indent;
        $$output .= "--${comment}\n";
    }
}

sub printStructFields
{
    my ($output, $struct, $enumMap) = @_;
    my @fields = @{${$struct}{"fields"}};
    my $structName = ${$struct}{"name"};

    while (@fields)
    {
        my $field = shift(@fields);

        printComments($output, ${$field}{"comment"}, 1);

        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enumName = ${$field}{"enum"};
            my $enum = ${$enumMap}{$enumName};
            my @values = @{${$enum}{"values"}};

            while (@values)
            {
                my $value = shift(@values);

                $$output .= "\t`$structName`.`${$field}{\"name\"}_${$value}{\"name\"}`";
                $$output .= ",\n" if @values;
            }
        }
        else
        {
            $$output .= "\t`$structName`.`${$field}{\"name\"}`";
        }

        $$output .= ",\n\n" if @fields;
    }
}

sub printStructContent
{
    my ($output, $struct, $structMap, $enumMap, $first) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printStructContent($output, $inheritStruct, $structMap, $enumMap, 0);
        }
    }

    printStructFields($output, $struct, $enumMap);
    $$output .= ",\n" unless $first;
    $$output .= "\n";
}

sub printBaseStruct
{
    my ($outstr, $struct, $structMap) = @_;
    my $is_base = 1;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printBaseStruct($outstr, $inheritStruct, $structMap);
            $is_base = 0;
        }
        elsif (/abstract/)
        {
            $$outstr .= "`${$struct}{\"name\"}`";
            $is_base = 0;
        }
    }

    $$outstr .= "`${$struct}{\"name\"}`" if $is_base;
}

sub printStructJoins
{
    my ($outstr, $struct, $structMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            printStructJoins($outstr, $inheritStruct, $structMap);
            $$outstr .= " INNER JOIN `${$struct}{\"name\"}` ON `${inheritName}`.`unique_inheritance_id` = `${$struct}{\"name\"}`.`unique_inheritance_id`";
        }
    }
}

sub printEnum()
{
}

sub printStruct()
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    printComments($output, ${$struct}{"comment"}, 0);

    # Start printing the select statement
    $$output .= "SELECT\n";

    printStructContent($output, $struct, $structMap, $enumMap, 1);
    $$output .= "FROM ";

    printBaseStruct($output, $struct, $structMap);
    printStructJoins($output, $struct, $structMap);

    $$output .= ";\n\n";
}

sub startFile()
{
    my ($output, $name) = @_;

    $$output .= "-- This file is generated automatically, do not edit, change the source ($name) instead.\n\n";
}

sub endFile()
{
}

1;
