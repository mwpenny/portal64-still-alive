################################################
## Generate C file containing binary resource ##
################################################

string(MAKE_C_IDENTIFIER "${INPUT_FILE}" SYMBOL_NAME)
set(SYMBOL_NAME "_binary_${SYMBOL_NAME}")

file(READ "${INPUT_FILE}" HEX_FILE_CONTENT HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " HEX_FILE_CONTENT "${HEX_FILE_CONTENT}")

string(CONCAT C_FILE_CONTENT
    "#include <stddef.h>\n"
    "const char ${SYMBOL_NAME}[] = { ${HEX_FILE_CONTENT} };\n"
    "const size_t ${SYMBOL_NAME}_size = sizeof(${SYMBOL_NAME});\n"
)
file(WRITE "${OUTPUT_FILE}" "${C_FILE_CONTENT}")
