import gdb
import traceback

def dump_hex_integer(output, number):
    output.write(format((number >> 24) & 255, "02X"))
    output.write(" ")
    output.write(format((number >> 16) & 255, "02X"))
    output.write(" ")
    output.write(format((number >> 8) & 255, "02X"))
    output.write(" ")
    output.write(format((number >> 0) & 255, "02X"))

try:
    dmem_ptr = gdb.parse_and_eval("(int*)0xA4000000")

    with open("rcp_dump.txt", "w") as output:
        for i in range(0, 4096, 16):
            index_offset = i >> 2

            output.write(format(i, "03X"))
            
            output.write(" ")

            dump_hex_integer(output, int(dmem_ptr[index_offset + 0]))
            output.write(" ")
            dump_hex_integer(output, int(dmem_ptr[index_offset + 1]))
            output.write(" ")
            dump_hex_integer(output, int(dmem_ptr[index_offset + 2]))
            output.write(" ")
            dump_hex_integer(output, int(dmem_ptr[index_offset + 3]))

            output.write("     \n")

except:
    print("An error happened")
    print(traceback.format_exc())
    