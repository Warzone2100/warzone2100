#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for C struct definitions

my $filename = "";
my $outfile = "";
my $startTpl = "";

sub printStructFieldType
{
    my ($output, $field) = @_;

    $_ = ${$field}{"type"};

    if    (/count/)     { $$output .= "unsigned int     "; }
    elsif (/string/)    { $$output .= "const char*      "; }
    elsif (/real/)      { $$output .= "float            "; }
    elsif (/bool/)      { $$output .= "bool             "; }
    elsif (/set/)       { $$output .= "bool             "; }
    elsif (/enum/)      { $$output .= "${${$field}{\"enum\"}}{\"name\"} "; }
    elsif (/IMD_model/) { $$output .= "iIMDShape*       "; }
    elsif (/C-only-field/) { $$output .= "${$field}{\"ctype\"} "; }
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

sub getStructName
{
    my ($name, $struct, $structMap, $prefix, $suffix) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        $$prefix = ${${$struct}{"qualifiers"}}{$_} if /prefix/ and not $$prefix;
        $$suffix = ${${$struct}{"qualifiers"}}{$_} if /suffix/ and not $$suffix;

        getStructName($name, ${${$struct}{"qualifiers"}}{"inherit"}, $structMap, $prefix, $suffix) if /inherit/;
    }

    $$name = ${$struct}{"name"};
}

sub printStructContent
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritName = ${$inheritStruct}{"name"};

            $$output .= "\t/* BEGIN of inherited \"$inheritName\" definition */\n";
            printStructContent($output, $inheritStruct, $structMap, $enumMap);
            $$output .= "\t/* END of inherited \"$inheritName\" definition */\n";
        }
    }

    printStructFields($output, $struct, $enumMap);
}

sub printEnum()
{
    my ($output, $enum) = @_;

    printComments($output, ${$enum}{"comment"}, 0);

    # Start printing the enum
    $$output .= "typedef enum ${$enum}{\"name\"}\n{\n";

    my @values = @{${$enum}{"values"}};

    my $valprefix = "";
    $valprefix = ${${$enum}{"qualifiers"}}{"valprefix"} if exists(${${$enum}{"qualifiers"}}{"valprefix"});
    my $valsuffix = "";
    $valsuffix = ${${$enum}{"qualifiers"}}{"valsuffix"} if exists(${${$enum}{"qualifiers"}}{"valsuffix"});

    $valprefix = "${$enum}{\"name\"}_" if not exists(${${$enum}{"qualifiers"}}{"valprefix"}) and not exists(${${$enum}{"qualifiers"}}{"valsuffix"});

    while (@values)
    {
        my $value = shift(@values);
        my $name = ${$value}{"name"};

        printComments($output, ${$value}{"comment"}, 1);

        $$output .= "\t${valprefix}${name}${valsuffix},\n";

        $$output .= "\n" if @values or exists(${${$enum}{"qualifiers"}}{"max"});
    }

    if (exists(${${$enum}{"qualifiers"}}{"max"}))
    {
        $$output .= "\t/**\n"
                  . "\t * The number of enumerators in this enum.\n"
                  . "\t */\n"
                  . "\t${${$enum}{\"qualifiers\"}}{\"max\"},\n";
    }

    # Finish printing the enum
    $$output .= "} ${$enum}{\"name\"};\n\n";
}

sub printStruct()
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    my $name;
    my $prefix = "";
    my $suffix = "";

    printComments($output, ${$struct}{"comment"}, 0);

    getStructName(\$name, $struct, $structMap, \$prefix, \$suffix);

    # Start printing the structure
    $$output .= "typedef struct ${prefix}${name}${suffix}\n{\n";

    printStructContent($output, $struct, $structMap, $enumMap);

    # Finish printing the structure
    $$output .= "} ${prefix}${name}${suffix};\n\n";

    return unless exists(${${$struct}{"qualifiers"}}{"loadFunc"});

    $$output .= "/* Forward declaration to allow pointers to this type */\n"
              . "struct sqlite3;\n"
              . "\n"
              . "/** Load the contents of the ${$struct}{\"name\"} table from the given SQLite database.\n"
              . " *\n"
              . " *  \@param db represents the database to load from\n"
              . " *\n"
              . " *  \@return true if we succesfully loaded all available rows from the table,\n"
              . " *          false otherwise.\n"
              . " */\n"
              . "extern bool\n"
              . "#line ${${${$struct}{\"qualifiers\"}}{\"loadFunc\"}}{\"line\"} \"$filename\"\n"
              . "${${${$struct}{\"qualifiers\"}}{\"loadFunc\"}}{\"name\"}\n";

    my $count = $$output =~ s/\n/\n/sg;
    $count += 2;
    $$output .= "#line $count \"$outfile\"\n"
              . "\t(struct sqlite3* db);\n\n";
}

sub printHdrGuard
{
    my ($output, $name) = @_;

    $name =~ tr/\./_/;
    $name =~ tr/-/_/;
    $name = uc($name);

    $$output .= "__INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_${name}_H__";
}

sub processCmdLine()
{
    my ($argv) = @_;

    $startTpl = shift(@$argv) if @$argv > 1;
}

sub startFile()
{
    my ($output, $name, $outputfile) = @_;

    $filename = $name;
    if ($outputfile)
    {
        $outfile = $outputfile;
    }
    else
    {
        $outfile = $name;
        # Replace the extension with ".h" so that we can use it to #include the header
        $outfile =~ s/\.[^.]*$/.h/;
    }

    $$output .= "/* This file is generated automatically, do not edit, change the source ($name) instead. */\n\n";

    $$output .= "#ifndef ";
    printHdrGuard($output, $outfile);
    $$output .= "\n";

    $$output .= "#define ";
    printHdrGuard($output, $outfile);
    $$output .= "\n\n";

    return unless $startTpl;

    $$output .= "#line 1 \"$startTpl\"\n";
    open (TEMPL, $startTpl);
    while (<TEMPL>)
    {
        s/\$name\b/$name/g;
        s/\$header\b/$outfile/g;
        $$output .= $_;
    }
    close (TEMPL);

    my $count = $$output =~ s/\n/\n/sg;
    $count += 2;
    $$output .= "#line $count \"$outfile\"\n"
              . "\n";
}

sub endFile()
{
    my ($output, $name) = @_;

    $$output .= "#endif // ";
    printHdrGuard($output, $name);
    $$output .= "\n";
}

1;
