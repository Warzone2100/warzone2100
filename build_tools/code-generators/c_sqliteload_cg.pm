#!/usr/bin/perl -w
# vim: set et sts=4 sw=4:

package CG;
use strict;

# Code generator for C code to load contents of the SQL tables from an SQLite
# database into the C struct definitions.

my $filename = "";
my $outfile = "";

sub printComments
{
    my ($output, $comments, $indent) = @_;

    return unless @{$comments};

    $$output .= "$indent/*${$comments}[0]\n";

    for (my $i = 1; $i < @{$comments}; $i++)
    {
        my $comment = ${$comments}[$i];
        $$output .= "$indent *${comment}\n";
    }

    $$output .= "$indent */\n";
}

sub printSqlComments
{
    my ($output, $comments, $indent) = @_;

    return unless @{$comments};

    foreach (@{$comments})
    {
        my $comment = $_;
        $comment =~ s/\"/\\\"/g;
        $$output .= "$indent\"--${comment}\\n\"\n";
    }
}

sub printEnum()
{
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

    $$name = "${$prefix}${$struct}{\"name\"}${$suffix}";
}

sub printFuncHeader
{
    my ($output, $struct, $structName) = @_;

    $$output .= "/** Load the contents of the ${$struct}{\"name\"} table from the given SQLite database.\n"
              . " *\n"
              . " *  \@param db represents the database to load from\n"
              . " *\n"
              . " *  \@return true if we succesfully loaded all available rows from the table,\n"
              . " *          false otherwise.\n"
              . " */\n"
              . "bool\n"
              . "#line ${${${$struct}{\"qualifiers\"}}{\"loadFunc\"}}{\"line\"} \"$filename\"\n"
              . "${${${$struct}{\"qualifiers\"}}{\"loadFunc\"}}{\"name\"}\n";

    my $line = $$output =~ s/\n/\n/sg;
    $line += 2;
    $$output .= "#line $line \"$outfile\"\n"
              . "\t(sqlite3* db)\n";
}

sub printFuncFooter
{
    my ($output) = @_;

    $$output .= "\n"
              . "\tretval = true;\n"
              . "\n"
              . "in_statement_err:\n"
              . "\tsqlite3_finalize(stmt);\n"
              . "\n"
              . "\treturn retval;\n";
}

sub printColNums
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if    (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};

            printColNums($output, $inheritStruct, $structMap, $enumMap);
        }
        elsif (/abstract/)
        {
            $$output .= "\t\tint unique_inheritance_id;\n";
        }
    }

    foreach my $field (@{${$struct}{"fields"}})
    {
        my $fieldName = ${$field}{"name"};

        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enum = ${$field}{"enum"};

            foreach my $value (@{${$enum}{"values"}})
            {
                $$output .= "\t\tint ${$field}{\"name\"}_${$value}{\"name\"};\n";
            }
        }
        else
        {
            $$output .= "\t\tint $fieldName;\n";
        }
    }
}

sub printSqlQueryVariables
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    $$output .= "\tbool retval = false;\n"
              . "\tsqlite3_stmt* stmt;\n"
              . "\tint rc;\n"
              . "\tstruct\n"
              . "\t{\n";
              
    printColNums($output, $struct, $structMap, $enumMap);

    $$output .= "\t} cols;\n"
              . "\n";
}

sub printStructFields
{
    my ($output, $struct, $enumMap, $indent) = @_;
    my @fields = @{${$struct}{"fields"}};
    my $structName = ${$struct}{"name"};

    while (@fields)
    {
        my $field = shift(@fields);

        printSqlComments($output, ${$field}{"comment"}, $indent);

        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enum = ${$field}{"enum"};
            my @values = @{${$enum}{"values"}};

            while (@values)
            {
                my $value = shift(@values);

                $$output .= "$indent\"`$structName`.`${$field}{\"name\"}_${$value}{\"name\"}` AS `${$field}{\"name\"}_${$value}{\"name\"}`";
                $$output .= ",\\n\"\n" if @values;
            }
        }
        else
        {
            $$output .= "$indent\"`$structName`.`${$field}{\"name\"}` AS `${$field}{\"name\"}`";
        }

        $$output .= ",\\n\"\n\n" if @fields;
    }
}

sub printStructContent
{
    my ($output, $struct, $structMap, $enumMap, $indent, $first) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if    (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};

            printStructContent($output, $inheritStruct, $structMap, $enumMap, $indent, 0);
        }
        elsif (/abstract/)
        {
            my $tableName = ${$struct}{"name"};

            $$output .= "$indent\"-- Automatically generated ID to link the inheritance hierarchy.\\n\"\n"
                      . "$indent\"$tableName.unique_inheritance_id,\\n\"\n";
        }
    }

    printStructFields($output, $struct, $enumMap, $indent);
    $$output .= "," unless $first;
    $$output .= "\\n\"\n";
    $$output .= "\n" unless $first;
}

sub printBaseStruct
{
    my ($outstr, $struct, $structMap) = @_;
    my $is_base = 1;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};

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
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritName = ${$inheritStruct}{"name"};

            printStructJoins($outstr, $inheritStruct, $structMap);
            $$outstr .= " INNER JOIN `${$struct}{\"name\"}` ON `${inheritName}`.`unique_inheritance_id` = `${$struct}{\"name\"}`.`unique_inheritance_id`";
        }
    }
}

sub printParameterLoadCode
{
    my ($output, $struct, $structMap, $parameter) = @_;
    my $tableName = ${$struct}{"name"};

    $$output .= "\t\t/* Prepare this SQL statement for execution */\n"
              . "\t\tif (!prepareStatement(db, &stmt, \"SELECT ";
              
    $_ = $parameter;
    if    (/\bmaxId\b/)     { $$output .= "MAX"; }
    elsif (/\browCount\b/)  { $$output .= "COUNT"; }
    else                    { die "UKNOWN PARAMETER TYPE: $_"; }

    $$output .= "(`$tableName`.unique_inheritance_id) ";

    printBaseStruct($output, $struct, $structMap);
    printStructJoins($output, $struct, $structMap);

    $$output .= ";\"))\n"
              . "\t\t\treturn false;\n"
              . "\n"
              . "\t\t/* Execute and process the results of the above SQL statement */\n"
              . "\t\tif (sqlite3_step(stmt) != SQLITE_ROW\n"
              . "\t\t || sqlite3_data_count(stmt) != 1)\n"
              . "\t\t\tgoto in_statement_err;\n"
              . "\t\t";
    
    if    (/\bmaxId\b/)     { $$output .= "MAX_ID_VAR"; }
    elsif (/\browCount\b/)  { $$output .= "ROW_COUNT_VAR"; }
    else                    { die "UKNOWN PARAMETER TYPE: $_"; }

    $$output .= " = sqlite3_column_int(stmt, 0);\n"
              . "\t\tsqlite3_finalize(stmt);\n"
              . "\n";
}

sub printPreLoadTableCode
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    return unless exists(${${$struct}{"qualifiers"}}{"preLoadTable"});

    my $line = ${${${$struct}{"qualifiers"}}{"preLoadTable"}}{"line"};

    $$output .= "\t{\n";

    foreach (@{${${${$struct}{"qualifiers"}}{"preLoadTable"}}{"parameters"}})
    {
        if    (/\bmaxId\b/)
        {
            $$output .= "\t\tint MAX_ID_VAR;\n";
        }
        elsif (/\browCount\b/)
        {
            $$output .= "\t\tunsigned int ROW_COUNT_VAR;\n";
        }
        else
        {
            die "UKNOWN PARAMETER TYPE: $_";
        }
    }

    $$output .= "\n" if (@{${${${$struct}{"qualifiers"}}{"preLoadTable"}}{"parameters"}});

    foreach (@{${${${$struct}{"qualifiers"}}{"preLoadTable"}}{"parameters"}})
    {
        printParameterLoadCode($output, $struct, $structMap, $_);
    }

    $$output .= "#line " . ($line + 1) . " \"$filename\"\n";
    foreach (@{${${${$struct}{"qualifiers"}}{"preLoadTable"}}{"code"}})
    {
        s/^        //g;
        s/    /\t/g;
        s/\$maxId\b/MAX_ID_VAR/g;
        s/\$rowCount\b/ROW_COUNT_VAR/g;

        s/\bABORT\b/return false/g;

        $$output .= "\t\t$_\n";
    }

    $line = $$output =~ s/\n/\n/sg;
    $line += 2;
    $$output .= "#line $line \"$outfile\"\n";

    $$output .= "\t}\n"
              . "\n";
}

sub printSqlFetchColName
{
    my ($output, $colname) = @_;

    $$output .= "\tcols.$colname = getColNumByName(stmt, \"$colname\");\n"
              . "\t\tASSERT(cols.$colname != -1, \"Column $colname not found in result set!\");\n"
              . "\t\tif (cols.$colname == -1)\n"
              . "\t\t\tgoto in_statement_err;\n";
}

sub printSqlColNameFetchCode
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if    (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritName = ${$inheritStruct}{"name"};

            $$output .= "\t/* BEGIN of inherited \"$inheritName\" definition */\n";
            printSqlColNameFetchCode($output, $inheritStruct, $structMap, $enumMap);
            $$output .= "\t/* END of inherited \"$inheritName\" definition */\n";
        }
        elsif (/abstract/)
        {
            printSqlFetchColName($output, "unique_inheritance_id");
        }
    }

    foreach my $field (@{${$struct}{"fields"}})
    {
        if (${$field}{"type"} and ${$field}{"type"} =~ /set/)
        {
            my $enum = ${$field}{"enum"};

            foreach my $value (@{${$enum}{"values"}})
            {
                printSqlFetchColName($output, "${$field}{\"name\"}_${$value}{\"name\"}");
            }
        }
        else
        {
            printSqlFetchColName($output, ${$field}{"name"});
        }
    }
}

sub printStartSelectQuery
{
    my ($output, $structName, $struct, $structMap, $enumMap) = @_;

    $$output .= "\t/* Prepare the query to start fetching all rows */\n"
              . "\tif (!prepareStatement(db, &stmt,\n"
              . "\t                          ";
    my $indent;
    $indent   = "\t                          ";

    # Start printing the select statement
    $$output .= "\"SELECT\\n\"\n";

    printStructContent($output, $struct, $structMap, $enumMap, $indent . "    ", 1);
    $$output .= "${indent}\"FROM ";

    printBaseStruct($output, $struct, $structMap);
    printStructJoins($output, $struct, $structMap);

    $$output .= ";";

    $$output .= "\"))\n"
              . "\t\treturn false;\n"
              . "\n"
              . "\t/* Fetch the first row */\n"
              . "\tif ((rc = sqlite3_step(stmt) != SQLITE_ROW))\n"
              . "\t{\n"
              . "\t\t/* Apparently we fetched no rows at all, this is a non-failure, terminal condition. */\n"
              . "\t\tsqlite3_finalize(stmt);\n"
              . "\t\treturn true;\n"
              . "\t}\n"
              . "\n"
              . "\t/* Fetch and cache column numbers */\n";

    printSqlColNameFetchCode($output, $struct, $structMap, $enumMap);
    $$output .= "\n";
}

sub printRowProcessCode
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    foreach (keys %{${$struct}{"qualifiers"}})
    {
        if (/inherit/)
        {
            my $inheritStruct = ${${$struct}{"qualifiers"}}{"inherit"};
            my $inheritName = ${$inheritStruct}{"name"};

            $$output .= "\t\t/* BEGIN of inherited \"$inheritName\" definition */\n";
            printRowProcessCode($output, $inheritStruct, $structMap, $enumMap);
            $$output .= "\t\t/* END of inherited \"$inheritName\" definition */\n";
        }
    }

    foreach my $field (@{${$struct}{"fields"}})
    {
        printComments($output, ${$field}{"comment"}, "\t\t");

        my $fieldName = ${$field}{"name"};
        $_ = ${$field}{"type"};

        if    (/set/)
        {
            my $enum = ${$field}{"enum"};

            for (my $i = 0; $i < @{${$enum}{"values"}}; $i++)
            {
                my $value = ${${$enum}{"values"}}[$i];
                my $valueName = ${$value}{"name"};

                $$output .= "\t\tstats->$fieldName\[$i] = sqlite3_column_int(stmt, cols.${fieldName}_${valueName}) ? true : false;\n";
            }
        }
        elsif (/count/)
        {
            $$output .= "\t\tstats->$fieldName = sqlite3_column_int(stmt, cols.$fieldName);\n";
        }
        elsif (/real/)
        {
            $$output .= "\t\tstats->$fieldName = sqlite3_column_double(stmt, cols.$fieldName);\n";
        }
        elsif (/bool/)
        {
            $$output .= "\t\tstats->$fieldName = sqlite3_column_int(stmt, cols.$fieldName) ? true : false;\n";
        }
        elsif (/enum/)
        {
            my $enum = ${$field}{"enum"};
            my $enumSize = @{${$enum}{"values"}};

            $$output .= "\t\tstats->$fieldName = sqlite3_column_int(stmt, cols.$fieldName);\n"
                      . "\t\tASSERT(stats->$fieldName < $enumSize, \"Enum out of range (%u), maximum is $enumSize\", stats->$fieldName);\n";
        }
        elsif (/count/)
        {
            $$output .= "\t\tstats->$fieldName = sqlite3_column_int(stmt, cols.$fieldName);\n";
        }
        elsif (/string/)
        {
            my $indent = "\t\t";

            if (grep(/optional/, @{${$field}{"qualifiers"}}))
            {
                $indent .= "\t";
                $$output .= "\t\tif (sqlite3_column_type(stmt, cols.$fieldName) != SQLITE_NULL)\n"
                          . "\t\t{\n";
            }

            $$output .= "${indent}stats->$fieldName = strdup((const char*)sqlite3_column_text(stmt, cols.$fieldName));\n"
                      . "${indent}if (stats->$fieldName == NULL)\n"
                      . "${indent}{\n"
                      . "${indent}\tdebug(LOG_ERROR, \"Out of memory\");\n"
                      . "${indent}\tabort();\n"
                      . "${indent}\tgoto in_statement_err;\n"
                      . "${indent}}\n";

            if (grep(/optional/, @{${$field}{"qualifiers"}}))
            {
                $$output .= "\t\t}\n"
                          . "\t\telse\n"
                          . "\t\t{\n"
                          . "\t\t\tstats->$fieldName = NULL;\n"
                          . "\t\t}\n";
            }
        }
        elsif (/IMD_model/)
        {
            my $indent = "\t\t";

            if (grep(/optional/, @{${$field}{"qualifiers"}}))
            {
                $indent .= "\t";
                $$output .= "\t\tif (sqlite3_column_type(stmt, cols.$fieldName) != SQLITE_NULL)\n"
                          . "\t\t{\n";
            }

            $$output .= "${indent}stats->$fieldName = (iIMDShape *) resGetData(\"IMD\", (const char*)sqlite3_column_text(stmt, cols.$fieldName));\n"
                      . "${indent}if (stats->$fieldName == NULL)\n"
                      . "${indent}{\n"
                      . "${indent}\tdebug(LOG_ERROR, \"Cannot find the IMD for field \\\"$fieldName\\\" of record num %u\", (unsigned int)sqlite3_column_int(stmt, cols.unique_inheritance_id));\n"
                      . "${indent}\tabort();\n"
                      . "${indent}\tgoto in_statement_err;\n"
                      . "${indent}}\n";

            if (grep(/optional/, @{${$field}{"qualifiers"}}))
            {
                $$output .= "\t\t}\n"
                          . "\t\telse\n"
                          . "\t\t{\n"
                          . "\t\t\tstats->$fieldName = NULL;\n"
                          . "\t\t}\n";
            }
        }
        elsif (/C-only-field/)
        {
            # Ignore: this is a user defined field type, the user code (%postLoadRow) can deal with it
        }
        else
        {
            die "UKNOWN TYPE: $_";
        }

        $$output .= "\n";
    }
}

sub printLoadFunc
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    return unless exists(${${$struct}{"qualifiers"}}{"loadFunc"});

    my $structName = "";
    my $prefix = "";
    my $suffix = "";
    getStructName(\$structName, $struct, $structMap, \$prefix, \$suffix);

    # Open the function
    $$output .= "\n";
    printFuncHeader($output, $struct, $structName);
    $$output .= "{\n";

    printSqlQueryVariables($output, $struct, $structMap, $enumMap);

    printPreLoadTableCode($output, $struct, $structMap, $enumMap);

    printStartSelectQuery($output, $structName, $struct, $structMap, $enumMap);

    $$output .= "\twhile (rc == SQLITE_ROW)\n"
              . "\t{\n"
              . "\t\t$structName sStats, * const stats = &sStats;\n"
              . "\n"
              . "\t\tmemset(stats, 0, sizeof(*stats));\n"
              . "\n";

    printRowProcessCode($output, $struct, $structMap, $enumMap);

    if (exists(${${$struct}{"qualifiers"}}{"postLoadRow"}))
    {
        $$output .= "\t\t{\n";
        $$output .= "\t\t\tconst int CUR_ROW_ID = sqlite3_column_int(stmt, cols.unique_inheritance_id);\n" if grep(/curId/, @{${${${$struct}{"qualifiers"}}{"postLoadRow"}}{"parameters"}});
        $$output .= "\n";

        my $line = ${${${$struct}{"qualifiers"}}{"postLoadRow"}}{"line"};

        $$output .= "#line " . ($line + 1) . " \"$filename\"\n";
        foreach (@{${${${$struct}{"qualifiers"}}{"postLoadRow"}}{"code"}})
        {
            s/^        //g;
            s/    /\t/g;
            s/\$curId\b/CUR_ROW_ID/g;
            s/\$curRow\b/stats/g;

            s/\bABORT\b/goto in_statement_err/g;

            $$output .= "\t\t\t$_\n";
        }

        $line = $$output =~ s/\n/\n/sg;
        $line += 2;
        $$output .= "#line $line \"$outfile\"\n";

        $$output .= "\t\t}\n"
                  . "\n";
    }

    $$output .= "\t\t/* Retrieve the next row */\n"
              . "\t\trc = sqlite3_step(stmt);\n"
              . "\t}\n";

    # Close the function
    printFuncFooter($output);
    $$output .= "}\n";    
}

sub printStruct()
{
    my ($output, $struct, $structMap, $enumMap) = @_;

    printLoadFunc($output, $struct, $structMap, $enumMap);
}

my $startTpl = "";

sub processCmdLine()
{
    my ($argv) = @_;

    $startTpl = shift(@$argv) if @$argv > 1;
}

sub startFile()
{
    my ($output, $name, $outputfile) = @_;

    $$output .= "/* This file is generated automatically, do not edit, change the source ($name) instead. */\n"
              . "\n"
              . "#include \"lib/sqlite3/sqlite3.h\"\n"
              . "\n";

    $filename = $name;
    if ($outputfile)
    {
        $outfile = $outputfile;
    }
    else
    {
        $outfile = $name;
        $outfile =~ s/\.[^.]*$/.c/;
    }

    return unless $startTpl;

    # Replace the extension with ".h" so that we can use it to #include the header
    my $header = $outfile;
    $header =~ s/\.[^.]*$/.h/;

    $$output .= "#line 1 \"$startTpl\"\n";
    open (TEMPL, $startTpl);
    while (<TEMPL>)
    {
        s/\$name\b/$name/g;
        s/\$header\b/$header/g;
        $$output .= $_;
    }
    close (TEMPL);

    my $count = $$output =~ s/\n/\n/sg;
    $count += 2;
    $$output .= "#line $count \"$outfile\"\n";
}

sub endFile()
{
}

1;
