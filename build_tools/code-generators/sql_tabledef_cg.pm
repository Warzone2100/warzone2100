#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for SQL table definitions

my $filename = "";
my $outfile = "";

sub printStructFieldType
{
    my ($output, $field) = @_;

    $_ = $field->{"type"};

    if    (/count/)     { $$output .= "INTEGER NOT NULL"; }
    elsif (/([US]D?WORD|[US]BYTE)/) { $$output .= "INTEGER NOT NULL"; } # "transition" type
    elsif (/real/)      { $$output .= "NUMERIC NOT NULL"; }
    elsif (/bool/)      { $$output .= "INTEGER NOT NULL"; }
    elsif (/enum/)      { $$output .= "TEXT NOT NULL"; }
    elsif (/struct/)
    {
        $$output .= "INTEGER";
        $$output .= " NOT NULL" unless grep(/optional/, @{$field->{"qualifiers"}});
    }
    elsif (/string/)
    {
        $$output .= "TEXT";
        $$output .= " NOT NULL" unless grep(/optional/, @{$field->{"qualifiers"}});
    }
    elsif (/IMD_model/)
    {
        $$output .= "TEXT";
        $$output .= " NOT NULL" unless grep(/optional/, @{$field->{"qualifiers"}});
    }
    elsif (/C-only-field/)
    {
        #Ignore: this field type is called C-only-field for a reason
    }
    else
    {
        die "error:$filename:$field->{line}: UKNOWN TYPE: $_";
    }
}

sub printComments
{
    my ($output, $comments, $indent) = @_;

    return unless @{$comments};

    foreach my $comment (@{$comments})
    {
        $$output .= "\t" if $indent;
        $$output .= "--$comment\n";
    }
}

sub printStructFields
{
    my ($output, $struct, $enumMap) = @_;
    my @fields = @{$struct->{"fields"}};
    my @constraints = ();

    while (@fields)
    {
        my $field = shift(@fields);

        printComments($output, $field->{"comment"}, 1) unless ($field->{"type"} and $field->{"type"} =~ /C-only-field/);

        if ($field->{"type"} and $field->{"type"} =~ /set/)
        {
            my $enum = $field->{"enum"};
            my @values = @{$enum->{"values"}};
            my $unique_string = "UNIQUE(";

            while (@values)
            {
                my $value = shift(@values);

                $$output .= "\t$field->{name}_$value->{name} INTEGER NOT NULL";
                $$output .= "," if @values or @fields or @constraints or grep(/unique/, @{$field->{"qualifiers"}});
                $$output .= "\n";
                $unique_string .= "$field->{name}_$value->{name}";
                $unique_string .= ", " if @values;
            }

            $unique_string .= ")";
            push @constraints, $unique_string if grep(/unique/, @{$field->{"qualifiers"}});
        }
        elsif ($field->{"type"} and $field->{"type"} =~ /C-only-field/)
        {
            # Ignore: this is a user defined field type, the user code (%postLoadRow) can deal with it
        }
        else
        {
            $$output .= "\t$field->{name} ";
            printStructFieldType($output, $field);
            $$output .= " UNIQUE" if grep(/unique/, @{$field->{"qualifiers"}});
            $$output .= ",\n" if (@fields and $fields[0]->{"type"} ne "C-only-field") or @constraints;
        }

        $$output .= "\n" unless ($field->{"type"} and $field->{"type"} =~ /C-only-field/);
    }

    while (@constraints)
    {
        my $constraint = shift(@constraints);

        $$output .= "\t$constraint";
        $$output .= ",\n" if @constraints;
        $$output .= "\n";

    }
}

sub printStructViewFields
{
    my ($output, $struct, $enumMap) = @_;
    my @fields = @{$struct->{"fields"}};
    my $structName = $struct->{"name"};

    my $first = 1;
FIELD:
    while (@fields)
    {
        my $field = shift(@fields);

        # Ignore: this is a user defined field type, the user code (%postLoadRow) can deal with it
        next FIELD if $field->{"type"} and $field->{"type"} =~ /C-only-field/;

        $$output .= ",\n\n" unless $first;
        $first = 0;

        printComments($output, $field->{"comment"}, 1);

        if ($field->{"type"} and $field->{"type"} =~ /set/)
        {
            my $enum = $field->{"enum"};
            my @values = @{$enum->{"values"}};

            while (@values)
            {
                my $value = shift(@values);

                $$output .= "\t`$structName`.`$field->{name}_$value->{name}` AS `$field->{name}_$value->{name}`";
                $$output .= ",\n" if @values;
            }
        }
        else
        {
            $$output .= "\t`$structName`.`$field->{name}` AS `$field->{name}`";
        }
    }
}

sub printStructContent
{
    my ($output, $struct, $name, $structMap, $enumMap, $printFields) = @_;

    foreach (keys %{$struct->{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = $struct->{"qualifiers"}{"inherit"};

            printStructContent($output, $inheritStruct, $name, $structMap, $enumMap, 0);
        }
        elsif (/abstract/)
        {
            $$output .= "\t-- Automatically generated ID to link the inheritance hierarchy.\n"
                     .  "\tunique_inheritance_id INTEGER PRIMARY KEY ";
            $$output .= "AUTOINCREMENT " if $printFields;
            $$output .= "NOT NULL,\n\n";
        }
    }

    $$name = ${$struct}{"name"};

    printStructFields($output, $struct, $enumMap) if $printFields;
}

sub printStructViewContent
{
    my ($output, $struct, $structMap, $enumMap, $first) = @_;

    foreach (keys %{$struct->{"qualifiers"}})
    {
        if    (/inherit/)
        {
            my $inheritStruct = $struct->{"qualifiers"}{"inherit"};

            printStructViewContent($output, $inheritStruct, $structMap, $enumMap, 0);
        }
        elsif (/abstract/)
        {
            my $tableName = $struct->{"name"};

            $$output .= "\t-- Automatically generated ID to link the inheritance hierarchy.\n"
                      . "\t`$tableName`.unique_inheritance_id AS unique_inheritance_id,\n";
        }
    }

    printStructViewFields($output, $struct, $enumMap);
    $$output .= "," unless $first;
    $$output .= "\n";
    $$output .= "\n" unless $first;
}

sub printBaseStruct
{
    my ($outstr, $struct) = @_;
    my $is_base = 1;

    foreach (keys %{$struct->{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = $struct->{"qualifiers"}{"inherit"};

            printBaseStruct($outstr, $inheritStruct);
            $is_base = 0;
        }
    }

    $$outstr .= "`$struct->{name}`" if $is_base;
}

sub printStructJoins
{
    my ($outstr, $struct, $structMap) = @_;

    foreach (keys %{$struct->{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = $struct->{"qualifiers"}{"inherit"};
            my $inheritName = $inheritStruct->{"name"};

            printStructJoins($outstr, $inheritStruct, $structMap);
            $$outstr .= " INNER JOIN `$struct->{name}` ON `$inheritName`.`unique_inheritance_id` = `$struct->{name}`.`unique_inheritance_id`";
        }
    }
}

sub printEnum()
{
}

sub printStruct()
{
    my ($output, $struct, $structMap, $enumMap) = @_;
    my $name = $struct->{"name"};

    printComments($output, $struct->{"comment"}, 0);

    # Start printing the structure
    $$output .= "CREATE TABLE `${name}` (\n";

    printStructContent($output, $struct, \$name, $structMap, $enumMap, 1);

    # Finish printing the structure
    $$output .= ");\n\n";

    # Start printing the VIEW to make SELECTing on this table easier
    $$output .= "CREATE VIEW `${name}S` AS SELECT\n";

    printStructViewContent($output, $struct, $structMap, $enumMap, 1);
    $$output .= "\tFROM ";

    printBaseStruct($output, $struct);
    printStructJoins($output, $struct, $structMap);

    $$output .= ";\n\n";
}

sub processCmdLine()
{
}

sub startFile()
{
    my ($output, $name, $outputfile) = @_;

    $$output .= "-- This file is generated automatically, do not edit, change the source ($name) instead.\n\n";

    $filename = $name;
    if ($outputfile)
    {
        $outfile = $outputfile;
    }
    else
    {
        $outfile = $name;
        $outfile =~ s/\.[^.]*$/.schema.sql/;
    }
}

sub endFile()
{
}

1;
