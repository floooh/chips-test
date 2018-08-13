#-------------------------------------------------------------------------------
#   fuse.py
#
#   Convert FUSE test files into C header.
#-------------------------------------------------------------------------------

Version = 2

import os.path
import yaml
import genutil

#-------------------------------------------------------------------------------
def gen_header(desc, out_hdr):
    with open(out_hdr, 'w') as outp:
        outp.write('// #version:{}#\n'.format(Version))
        outp.write('// machine generated, do not edit!\n')
        for item in desc['files']:
            inp_path = os.path.dirname(out_hdr) + '/' + item['file']
            inp_name = item['name']
            outp.write('fuse_test_t {}[] = {{\n'.format(inp_name))
            with open(inp_path, 'r') as inp:
                line = inp.readline()
                num_tests = 0
                while line:
                    outp.write('  {\n')
                    num_tests += 1
                    # desc (1 byte)
                    val = line.split()[0]
                    outp.write('    .desc = "{}",\n'.format(val))
                    # optional events start with spaces
                    line = inp.readline()
                    if line[0] == ' ':
                        num_events = 0
                        outp.write('    .events = {\n')
                        while line[0] == ' ':
                            tok = line.split()
                            if len(tok)==4:
                                outp.write('      {{ .tick={}, .type=EVENT_{}, .addr=0x{}, .data=0x{} }},\n'.format(
                                    tok[0], tok[1], tok[2], tok[3]
                                ))
                                num_events += 1
                            line = inp.readline()
                        outp.write('    },\n')
                        outp.write('    .num_events = {},\n'.format(num_events));
                    # 16-bit registers
                    tok = line.split()
                    outp.write('    .state = {\n')
                    if len(tok) == 12:
                        outp.write('      .af=0x{}, .bc=0x{}, .de=0x{}, .hl=0x{},\n'.format(tok[0], tok[1], tok[2], tok[3]))
                        outp.write('      .af_=0x{}, .bc_=0x{}, .de_=0x{}, .hl_=0x{},\n'.format(tok[4], tok[5], tok[6], tok[7]))
                        outp.write('      .ix=0x{}, .iy=0x{}, .sp=0x{}, .pc=0x{},\n'.format(tok[8], tok[9], tok[10], tok[11]))
                    # additional registers and flags
                    tok = inp.readline().split()
                    if len(tok) == 7:
                        outp.write('      .i=0x{}, .r=0x{}, .iff1={}, .iff2={}, .im={}, .halted={}, .ticks={}\n'.format(
                            tok[0], tok[1], tok[2], tok[3], tok[4], tok[5], tok[6]
                        ))
                    outp.write('    },\n')
                    # optional memory chunks
                    num_chunks = 0
                    tok = inp.readline().split()
                    if len(tok) > 1:
                        outp.write('    .chunks = {\n')
                        while len(tok) > 0:
                            if tok[0] != '-1':
                                outp.write('      { ')
                                outp.write('.addr=0x{}, .bytes = {{ '.format(tok[0]))
                                i = 1
                                while tok[i] != '-1':
                                    outp.write('0x{},'.format(tok[i]))
                                    i += 1
                                outp.write('}}, .num_bytes={}, '.format(i-1))
                                outp.write('},\n')
                                num_chunks += 1
                            tok = inp.readline().split()
                        outp.write('    },\n')
                    outp.write('    .num_chunks = {},\n'.format(num_chunks))
                    outp.write('  },\n')
                    line = inp.readline()
            outp.write('};\n')
            outp.write('const int {}_num = {};\n'.format(inp_name, num_tests))

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr) :
    if genutil.isDirty(Version, [input], [out_hdr]) :
        with open(input, 'r') as f:
            desc = yaml.load(f)
        gen_header(desc, out_hdr)
