cmake_minimum_required(VERSION 3.16...3.31)

# Intercept and execute the gettext compilation command safely
execute_process(
    COMMAND ${MSGFMT_EXECUTABLE} -c --statistics -o ${OUTPUT_FILE} ${INPUT_FILE}
    ERROR_VARIABLE ERROR_OUTPUT
    OUTPUT_VARIABLE STANDARD_OUTPUT
    RESULT_VARIABLE REG_RESULT
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
)

# Analyze failures to surface context snippets
if(NOT REG_RESULT EQUAL 0)

    # Isolate line markers (e.g., ":18475:")
    if(ERROR_OUTPUT MATCHES ":([0-9]+):")
        set(LINE_NUM ${CMAKE_MATCH_1})
        
        # Read the entire source file while preserving newlines
        file(READ ${INPUT_FILE} PO_CONTENT)

        # Protect existing semicolons so CMake doesn't treat them as list dividers
        string(REPLACE ";" "[[SEMICOLON_PLACEHOLDER]]" SAFE_CONTENT "${PO_CONTENT}")

        # Turn newlines into CMake list delimiters
        string(REPLACE "\r\n" "\n" SAFE_CONTENT "${SAFE_CONTENT}") # Handle Windows (CRLF) line endings safely
        string(REPLACE "\n" ";" PO_LINES "${SAFE_CONTENT}")
        
        # Shift to 0-based indexing for CMake lists
        math(EXPR START_INDEX "${LINE_NUM} - 1")
        set(SNIPPET "")
        set(MSGID_LINE "")
        
        # Scan backwards to wrap the entire broken chunk structure
        foreach(I RANGE ${START_INDEX} 0 -1)
            list(GET PO_LINES ${I} LINE_CONTENT)

            # Restore the original semicolons for visual output
            string(REPLACE "[[SEMICOLON_PLACEHOLDER]]" ";" LINE_CONTENT "${LINE_CONTENT}")

            string(PREPEND SNIPPET "  | ${LINE_CONTENT}\n")
            
            # Stop scanning when we identify the root block start line
            if(LINE_CONTENT MATCHES "^msgid" AND I LESS START_INDEX)
                set(MSGID_LINE "${LINE_CONTENT}")
                break()
            endif()
        endforeach()
        
        # Route warning messages cleanly depending on the execution runner host environment
        if($ENV{GITHUB_ACTIONS})
            # 1. Output a rich, structured log warning to the GitHub Actions terminal runner console
            message(WARNING "\n### [Translation Failure Context]\n```text\n${SNIPPET}```\n")
            
            # 2. Extract a single-line summary of the msgfmt validation failure
            string(REPLACE "\n" " " CONDENSED_ERR "${ERROR_OUTPUT}")
            if(CONDENSED_ERR MATCHES "([^/]+:[0-9]+: .+)")
                set(CLEAN_REASON "${CMAKE_MATCH_1}")
            else()
                set(CLEAN_REASON "Invalid msgid/msgstr structure match format error detected.")
            endif()
            
            # 3. Print the native GitHub Workflow tracking token directly to stdout
            # This surfaces the exact line directly in the PR's "Files Changed" tab automatically
            execute_process(COMMAND ${CMAKE_COMMAND} -E echo "::error file=${INPUT_FILE},line=${LINE_NUM},title=Translation Format Mismatch::${CLEAN_REASON} - context: ${MSGID_LINE}")
        else()
            # Standard local local workflow output decoration styling
            message(WARNING "\n======================================================================\n"
                            "TRANSLATION ERROR CONTEXT (${INPUT_FILE}):\n"
                            "======================================================================\n"
                            "${SNIPPET}"
                            "======================================================================\n")
        endif()
    endif()

    # Gracefully drop the compilation graph out to block invalid distribution bundles
    message(FATAL_ERROR "msgfmt failed processing translation templates.")
endif()
