#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for C struct definitions

sub printStructFieldType
{
    my ($output, $field) = @_;

    $_ = ${$field}{"type"};

    if    (/count/)     { $$output .= "unsigned int     "; }
    elsif (/string/)    { $$output .= "const char*      "; }
    elsif (/real/)      { $$output .= "float            "; }
    elsif (/bool/)      { $$output .= "bool             "; }
    elsif (/set/)       { $$output .= "bool             "; }
    elsif (/enum/)      { $$output .= "${$field}{\"enum\"} "; }
    elsif (/IMD_model/) { $$output .= "iIMDShape*       "; }
    else                { die "UKNOWN TYPE: $_"; }
}

sub preProcessField
{
    my ($field, $comments) = @_;

    if    (grep(/unique/, @{${$field}{"qualifiers"}}))
    {
        # Separate this notice from the rest of the comment if there's any
        push @{$comments}, "" if @{$comments};

        push @{$comments}, " Unique across all instances";
    }

    if    (grep(/optional/, @{${$field}{"qualifiers"}}))
    {
        # Separate this notice from the rest of the comment if there's any
        push @{$comments}, "" if @{$comments};

        push @{$comments}, " This field is optional and can be NULL to indicate that it has no value";
    }

    $_ = ${$field}{"type"};
    $_ = "" unless $_;

    if (/set/)
    {
        # Separate this notice from the rest of the comment if there's any
        push @{$comments}, "" if @{$comments};

        push @{$comments}, " \@see ${$field}{\"enum\"} for the meaning of index values";
    }
}

sub postProcessField
{
    my ($output, $field, $enumMap) = @_;
    $_ = ${$field}{"type"};
    $_ = "" unless $_;

    if (/set/)
    {
        my $enum = ${$field}{"enum"};
        my $enumSize = @{${$enum}{"values"}};

        $$output .= "\[${enumSize}]" if (/set/);
    }
}

sub printComments
{
    my ($output, $comments, $indent) = @_;

    return unless @{$comments};

    $$output .= "\t" if $indent;
    $$output .= "/**\n";

    foreach my $comment (@{$comments})
    {
        $$output .= "\t" if $indent;
        $$output .= " *${comment}\n";
    }

    $$output .= "\t" if $indent;
    $$output .= " */\n";
}

sub printStructFields
{
    my $output = $_[0];
    my @fields = @{${$_[1]}{"fields"}};
    my $enumMap = $_[2];

    while (@fields)
    {
        my $field = shift(@fields);
        my @comments = @{${$field}{"comment"}};

        preProcessField($field, \@comments);

        printComments($output, \@comments, 1);

        $$output .= "\t";
        printStructFieldType($output, $field);
        $$output .= ${$field}{"name"};

        postProcessField($output, $field, $enumMap);
        $$output .= ";\n";

        $$output .= "\n" if @fields;
    }
}

sub printStructContent
{
    my ($output, $struct, $name, $prefix, $structMap, $enumMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        $$prefix = ${${$struct}{"qualifiers"}}{$_} if /prefix/ and not $$prefix;

        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritName = ${$inheritStruct}{"name"};

            $$output .= "\t/* BEGIN of inherited \"$inheritName\" definition */\n";
            printStructContent($output, $inheritStruct, $name, $prefix, $structMap, $enumMap);
            $$output .= "\t/* END of inherited \"$inheritName\" definition */\n";
        }
    }

    $$name = ${$struct}{"name"};

    printStructFields($output, $struct, $enumMap);
}

sub printEnum()
{
    my ($output, $enum) = @_;

    printComments($output, ${$enum}{"comment"}, 0);

    # Start printing the enum
    $$output .= "typedef enum\n{\n";

    my @values = @{${$enum}{"values"}};

    while (@values)
    {
        my $value = shift(@values);
        my $name = ${$value}{"name"};

        printComments($output, ${$value}{"comment"}, 1);

        $$output .= "\t${$enum}{\"name\"}_${name},\n";

        $$output .= "\n" if @values;
    }

    # Finish printing the enum
    $$output .= "} ${$enum}{\"name\"};\n\n";
}

sub printStruct()
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    my $name;
    my $prefix = "";

    printComments($output, ${$struct}{"comment"}, 0);

    # Start printing the structure
    $$output .= "typedef struct\n{\n";

    printStructContent($output, $struct, \$name, \$prefix, $structMap, $enumMap);

    # Finish printing the structure
    $$output .= "} ${prefix}${name};\n\n";
}

sub printHdrGuard
{
    my ($output, $name) = @_;

    $name =~ s/\./_/g;
    $name = uc($name);

    $$output .= "__INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_${name}_H__";
}

sub processCmdLine()
{
}

sub startFile()
{
    my ($output, $name) = @_;

    $$output .= "/* This file is generated automatically, do not edit, change the source ($name) instead. */\n\n";

    $$output .= "#ifndef ";
    printHdrGuard($output, $name);
    $$output .= "\n";

    $$output .= "#define ";
    printHdrGuard($output, $name);
    $$output .= "\n\n";
}

sub endFile()
{
    my ($output, $name) = @_;

    $$output .= "#endif // ";
    printHdrGuard($output, $name);
    $$output .= "\n";
}

1;
