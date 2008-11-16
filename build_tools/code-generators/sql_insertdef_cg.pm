#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for SQL table insertions

my $filename = "";
my $outfile = "";
my $dataDir = "";

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
    my ($output, $struct, $first, $values) = @_;

    foreach my $field (@{${$struct}{"fields"}})
    {
        if (my $CSV = ${$field}{"CSV"})
        {
            $CSV = @$values - 1 if $CSV eq "last" and $values;

            $$output .= ",\n" unless $first;
            $first = 0;

            if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
            {
                if ($values)
                {
                }
                else
                {
                    my $enum = ${$field}{"enum"};
                    my @values = @{${$enum}{"values"}};

                    while (@values)
                    {
                        my $value = shift(@values);

                        $$output .= "\t`${$field}{\"name\"}_${$value}{\"name\"}`";
                    }
                }
            }
            elsif (${$field}{"type"} and ${$field}{"type"} =~ /C-only-field/)
            {
                # Ignore: this is a user defined field type, the user code (%postLoadRow) can deal with it
            }
            else
            {
                if ($values)
                {
                    my $value = ${$values}[$CSV-1];

                    if    (!$value && grep(/optional/, @{${$field}{"qualifiers"}}))
                    {
                        $value = 'NULL';
                    }
                    elsif ($value =~ /^\s*(\d+)\s*$/)
                    {
                        $value = $1;
                    }
                    else
                    {
                        $value = "'$value'";
                    }

                    $$output .= "\t$value";
                }
                else
                {
                    $$output .= "\t`${$field}{\"name\"}`";
                }
            }
        }
    }
}

sub printBaseStruct
{
    my ($outstr, $struct) = @_;
    my $is_base = 1;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};

            printBaseStruct($outstr, $inheritStruct);
            $is_base = 0;
        }
    }

    $$outstr .= "`${$struct}{\"name\"}`" if $is_base;
}

sub getUniqueBaseField
{
    my ($unique_field, $struct) = @_;
    my $is_base = 1;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};

            getUniqueBaseField($unique_field, $inheritStruct);
            $is_base = 0;
        }
    }

    if ($is_base)
    {
        foreach my $field (@{${$struct}{"fields"}})
        {
            if (my $CSV = ${$field}{"CSV"})
            {
                $$unique_field = $field if grep(/unique/, @{${$field}{"qualifiers"}});
                last;
            }
        }
    }
}

sub printStructContent
{
    my ($output, $struct, $structMap, $values) = @_;
    my $abstract = 0;
    my $inherit = 0;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};

            printStructContent($output, $inheritStruct, $structMap, $values);

            $inherit = 1;
        }
        elsif (/abstract/)
        {
            $abstract = 1;
        }
    }

    my $name = ${$struct}{"name"};

    $$output .= "INSERT INTO `$name` (\n";
    $$output .= "\t`unique_inheritance_id`" if $inherit;

    printStructFields($output, $struct, !$inherit, 0);
    $$output .= "\n)\n";

    my $unique_field = 0;
    getUniqueBaseField(\$unique_field, $struct);

    if    ($abstract)
    {
        $$output .= "VALUES (\n";
    }
    elsif ($inherit)
    {
        $$output .= "SELECT\n\t";
        if ($unique_field)
        {
            printBaseStruct($output, $struct);
            $$output .= ".`unique_inheritance_id`";
        }
        else
        {
            $$output .= "MAX(";
            printBaseStruct($output, $struct);
            $$output .= ".`unique_inheritance_id`)";
        }
    }

    printStructFields($output, $struct, !$inherit, $values);

    if ($inherit)
    {
        $$output .= "\nFROM ";
        printBaseStruct($output, $struct);

        if ($unique_field)
        {
            my $CSV = ${$unique_field}{"CSV"};
            $CSV = @$values - 1 if $CSV eq "last";
            my $value = ${$values}[$CSV-1];
            $value =~ s/'/''/g;

            $$output .= "\nWHERE ";
            printBaseStruct($output, $struct);
            $$output .= ".`${$unique_field}{\"name\"}`='$value'";
        }
    }
    else
    {
        $$output .= "\n)";
    }

    $$output .= ";\n";
}

sub printEnum()
{
}

sub printStruct()
{
    my ($output, $struct, $structMap, $enumMap) = @_;
    my $name = ${$struct}{"name"};

    if (${${$struct}{"qualifiers"}}{"csv-file"})
    {
        my $file = "$dataDir/${${$struct}{\"qualifiers\"}}{\"csv-file\"}";
        printComments($output, ${$struct}{"comment"}, 0);

        open (CSV, "<$file") or die "Can't open CSV file $file";

        $$output .= "-- Data for table `$name`, extracted from $file\n"
                  . "\n";

        while (<CSV>)
        {
            chomp;

            my @values = /([^,]*),?/g or die "$file:$.: Invalid CSV data";

            s/'/''/g foreach (@values);

            printStructContent($output, $struct, $structMap, \@values);
            $$output .= "\n";
        }
        close CSV;
    }
}

sub processCmdLine()
{
    my ($argv) = @_;

    $dataDir = shift(@$argv) if @$argv > 1;
}

sub startFile()
{
    my ($output, $name, $outputfile) = @_;

    $$output .= "-- This file is generated automatically, do not edit, change the source ($name) instead.\n"
              . "\n"
              . "BEGIN TRANSACTION;\n"
              . "\n";

    $filename = $name;
    if ($outputfile)
    {
        $outfile = $outputfile;
    }
    else
    {
        $outfile = $name;
        $outfile =~ s/\.[^.]*$/.data.sql/;
    }
}

sub endFile()
{
    my ($output, $name) = @_;

    $$output .= "COMMIT TRANSACTION;\n";
}

1;
