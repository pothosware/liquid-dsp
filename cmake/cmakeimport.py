import os
import sys
import re

this_file = os.path.abspath(__file__)
this_dir = os.path.dirname(this_file)
makefile_in = os.path.join(this_dir, '..', 'makefile.in')

if __name__ == "__main__":

    output = list()

    multiline = False

    for line in open(makefile_in).readlines():
        if line.endswith('\n'): line = line[:-1] #remove newline

        if multiline:
            multiline = line.endswith('\\')
            if multiline: line = line[:-1] #remove backslash
            else: line += ")"
            output.append(line)
            continue

        if not line.strip():
            output.append(line)
            continue

        if line.startswith('#'):
            output.append(line)
            continue

        var_assign_match = re.match(r"(\w+)\s+(.?)=\s+(.*)", line)

        if var_assign_match:
            var_name, sym, contents = var_assign_match.groups()

            multiline = contents.endswith('\\')
            if multiline:
                contents = contents[:-1] #remove backslash
                if sym == "+": output.append("list(APPEND %s %s"%(var_name, contents))
                else: output.append("set(%s %s"%(var_name, contents))
            else:
                if sym == "+": output.append("list(APPEND %s %s)"%(var_name, contents))
                else: output.append("set(%s %s)"%(var_name, contents))

    output = "\n".join(output)

    #fix $(variable) sub format to cmake
    output = re.sub(r"\$\((\w+)\)", r"${\1}", output)

    #fix @variable@ sub format to cmake
    output = re.sub(r"@(\w+)@", r"${\1}", output)

    #replace .o with source .c files
    output = re.sub(r"(\w+)\.o", r"\1.c", output)

    if len(sys.argv) > 1:
        open(sys.argv[1], 'w').write(output)
