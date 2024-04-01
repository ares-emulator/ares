import sys


class Instruction:
    def __init__(self, opcode, address, algrithm):
        self.opcode = opcode
        self.address = address
        self.algrithm = algrithm


class InstructionBuilder:
    def __init__(self, i_prev, i_next):
        self.i_prev = i_prev
        self.i_next = i_next

    def __str__(self):
        return "InstructionBuilder(%s, %s)" % (self.i_prev, self.i_next)

    # format out
    def build(self, opcode, address, algrithm):
        instruction = []

        instruction.append("    case %s:" % opcode)

        for line in address:
            instruction.append("    " + line)

        for line in self.i_prev:
            instruction.append("    " + line)

        for line in algrithm:
            instruction.append("    " + line)

        for line in self.i_next:
            instruction.append("    " + line)

        instruction.append("        break;")

        return instruction


def load_algrithms(filename):
    """ Load algrithms from file """
    algrithms = []
    algrithms_dict = {}

    try:
        f = open(filename, "r")
    except Exception:
        sys.exit(1)

    for line in f:
        if line.startswith("#"):
            continue
        line = line.rstrip()
        if not line:
            continue
        if line.startswith(" ") or line.startswith("\t"):
            algrithms[-1][1].append(line)
        else:
            algrithms.append((line, []))

    for algrithm in algrithms:
        algrithms_dict[algrithm[0]] = algrithm[1]

    return algrithms_dict


def load_address(filename):
    """ Load address from file """
    address = []
    address_dict = {}

    try:
        f = open(filename, "r")
    except Exception:
        sys.exit(1)

    for line in f:
        if line.startswith("#"):
            continue
        line = line.rstrip()
        if not line:
            continue
        if line.startswith(" ") or line.startswith("\t"):
            address[-1][1].append(line)
        else:
            address.append((line, []))

    for address in address:
        address_dict[address[0]] = address[1]

    return address_dict


def load_instructions(filename):
    """ Load instructions from file """
    instructions = []
    instruction_builders = {}

    try:
        f = open(filename, "r")
    except Exception:
        sys.exit(1)

    for line in f:
        if line.startswith("#"):
            continue
        line = line.rstrip()
        if not line:
            continue

        if line.startswith(" ") or line.startswith("\t"):
            instructions[-1][1].append(line)
        else:
            instructions.append((line, []))

    for instruction in instructions:
        is_algrithm = False
        instruction_prev = []
        instruction_next = []
        for line in instruction[1]:
            if line == "    ALU();":
                is_algrithm = True
                continue

            if is_algrithm:
                instruction_next.append(line)
            else:
                instruction_prev.append(line)

        instruction_builders[instruction[0]] = InstructionBuilder(
                instruction_prev,
                instruction_next)

    return instruction_builders


def load_opcodes(filename):
    opcodes = []

    try:
        f = open(filename, "r")
    except Exception:
        sys.exit(1)

    for line in f:
        if line.startswith("#"):
            continue
        line = line.rstrip()
        if not line:
            continue

        opcodes.append(line.split(' '))

    return opcodes


def main(argv):
    if len(argv) != 6:
        print("Usage: %s <address.lst> <algrithm.lst> <instruction.lst> <opcode.lst> <output>" % argv[0])
        return 1

    address = load_address(argv[1])
    algorithm = load_algrithms(argv[2])
    instruction_builders = load_instructions(argv[3])
    opcodes = load_opcodes(argv[4])

    try:
        f = open(argv[5], "w")
    except Exception:
        sys.exit(1)

    f.write("auto MOS6502::instruction() -> void {\n")
    f.write("    n8 code = opcode();\n")
    f.write("    switch (code) {\n")
    for opcode in opcodes:
        instruction = instruction_builders[opcode[2]].build(
                opcode[0],
                address[opcode[3]],
                algorithm[opcode[1]])

        for line in instruction:
            f.write(line + "\n")
        f.write("\n")
    f.write("    }\n")
    f.write("}\n")

    f.flush()
    f.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
