import gdb
import os
import traceback

def dump_hex_integer(output, number):
    output.write(format((number >> 24) & 0xff, "02X"))
    output.write(" ")
    output.write(format((number >> 16) & 0xff, "02X"))
    output.write(" ")
    output.write(format((number >> 8) & 0xff, "02X"))
    output.write(" ")
    output.write(format((number >> 0) & 0xff, "02X"))

def dump_mem(mem, size, output):
    for i in range(0, size, 16):
        index_offset = i >> 2

        output.write(format(i, "03X"))
        output.write(" ")

        dump_hex_integer(output, int(mem[index_offset + 0]))
        output.write(" ")
        dump_hex_integer(output, int(mem[index_offset + 1]))
        output.write(" ")
        dump_hex_integer(output, int(mem[index_offset + 2]))
        output.write(" ")
        dump_hex_integer(output, int(mem[index_offset + 3]))
        output.write("\n")


class DumpRcpState(gdb.Command):
    """Write RCP memory to a file. Specify custom path as optional argument."""

    RSP_DMEM_START      = 0xa4000000
    RSP_DMEM_SIZE       = 4096
    DEFAULT_OUTPUT_FILE = "rcp_dump.txt"

    def __init__(self):
        super().__init__("dump_rcp", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        output_path = os.path.abspath(argument or self.DEFAULT_OUTPUT_FILE)

        try:
            with open(output_path, "w") as output:
                dmem = gdb.parse_and_eval(f"(int*){self.RSP_DMEM_START}")
                dump_mem(dmem, self.RSP_DMEM_SIZE, output)
                print(f"Wrote to {output_path}")
        except:
            print("Error dumping RCP state")
            print(traceback.format_exc())

DumpRcpState()
