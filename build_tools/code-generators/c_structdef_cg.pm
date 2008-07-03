#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for C struct definitions

sub printStructFieldType
{
    my $field = $_[0];

    $_ = ${$field}{"type"};

    if    (/count/)     { print "unsigned int     "; }
    elsif (/string/)    { print "const char*      "; }
    elsif (/real/)      { print "float            "; }
    elsif (/bool/)      { print "bool             "; }
    elsif (/set/)       { print "bool             "; }
    elsif (/enum/)      { print "${$field}{\"enum\"} "; }
    else                { die "UKNOWN TYPE: $_"; }
}

sub preProcessField
{
    my ($field, $comments) = @_;
    $_ = ${$field}{"qualifier"};
    $_ = "" unless $_;

    if    (/unique/)
    {
        # Separate this notice from the rest of the comment if there's any
        push @{$comments}, "" if @{$comments};

        push @{$comments}, " Unique across all instances";
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
    my ($field, $enumMap) = @_;
    $_ = ${$field}{"type"};
    $_ = "" unless $_;

    if (/set/)
    {
        my $enumName = ${$field}{"enum"};
        my $enum = ${$enumMap}{$enumName};
        my $enumSize = @{${$enum}{"values"}};

        print "\[${enumSize}]" if (/set/);
    }
}

sub printComments
{
    my ($comments, $indent) = @_;

    return unless @{$comments};

    print "\t" if $indent;
    print "/**\n";

    foreach my $comment (@{$comments})
    {
        print "\t" if $indent;
        print " *${comment}\n";
    }

    print "\t" if $indent;
    print " */\n";
}

sub printStructFields
{
    my @fields = @{${$_[0]}{"fields"}};
    my $enumMap = $_[1];

    while (@fields)
    {
        my $field = shift(@fields);
        my @comments = @{${$field}{"comment"}};

        preProcessField $field, \@comments;

        printComments \@comments, 1;

        print "\t";
        printStructFieldType $field;
        print ${$field}{"name"};

        postProcessField $field, $enumMap;
        print ";\n";

        print "\n" if @fields;
    }
}

sub printStruct
{
    my ($struct, $name, $prefix, $structMap, $enumMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        $$prefix = ${${$struct}{"qualifiers"}}{$_} if /prefix/ and not $$prefix;

        if (/inherit/)
        {
            my $inheritName = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritStruct = ${$structMap}{$inheritName};

            print "\t/* BEGIN of inherited \"$inheritName\" definition */\n";
            printStruct($inheritStruct, $name, $prefix, $structMap, $enumMap);
            print "\t/* END of inherited \"$inheritName\" definition */\n\n";
        }
    }

    $$name = ${$struct}{"name"};

    printStructFields($struct, $enumMap);
}

sub printEnums()
{
    foreach my $enum (@{$_[0]})
    {
        printComments ${$enum}{"comment"}, 0;

        print "typedef enum\n{\n";

        my @values = @{${$enum}{"values"}};

        while (@values)
        {
            my $value = shift(@values);
            my $name = ${$value}{"name"};

            printComments ${$value}{"comment"}, 1;

            print "\t${$enum}{\"name\"}_${name},\n";

            print "\n" if @values;
        }

        print "} ${$enum}{\"name\"};\n\n";
    }
}

sub printStructs()
{
    my ($structList, $structMap, $enumMap) = @_;

    my @structs = @{$structList};

    while (@structs)
    {
        my $struct = shift(@structs);
        my $name;
        my $prefix = "";

        printComments ${$struct}{"comment"}, 0;

        # Start printing the structure
        print "typedef struct\n{\n";

        printStruct($struct, \$name, \$prefix, $structMap, $enumMap);

        $name = $prefix . $name;

        print "} ${name};\n";
        print "\n" if @structs;
    }
}

1;
