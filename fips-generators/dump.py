#-------------------------------------------------------------------------------
#   dump.py
#   Dump binary files into C arrays.
#-------------------------------------------------------------------------------

Version = 1

import os.path
import yaml
import genutil

#-------------------------------------------------------------------------------
def get_file_path(filename, file_path) :
    '''
    Returns absolute path to an input file, given file name and 
    another full file path in the same directory.
    '''
    return '{}/{}'.format(os.path.dirname(file_path), filename)

#-------------------------------------------------------------------------------
def get_file_cname(filename) :
    return 'dump_{}'.format(os.path.splitext(filename)[0])

#-------------------------------------------------------------------------------
def gen_header(out_hdr, files) :
    
    with open(out_hdr, 'w') as f:
        f.write('#pragma once\n')
        f.write('// #version:{}#\n'.format(Version))
        f.write('// machine generated, do not edit!\n')
        for file in files :
            file_path = get_file_path(file, out_hdr)
            if os.path.isfile(file_path) :
                file_name = get_file_cname(file)
                file_size = os.path.getsize(file_path)
                f.write('extern unsigned char {}[{}];\n'.format(file_name, file_size))
            else :
                genutil.fmtError("Input file not found: '{}'".format(file_path))

#-------------------------------------------------------------------------------
def gen_source(out_src, files) :

    with open(out_src, 'w') as f:
        f.write('// #version:{}#\n'.format(Version))
        f.write('// machine generated, do not edit!\n')
        f.write('#include "{}.h"\n'.format(os.path.splitext(os.path.basename(out_src))[0]))
        for file in files :
            file_path = get_file_path(file, out_src)
            if os.path.isfile(file_path) :
                with open(file_path, 'rb') as src_file:
                    file_data = src_file.read()
                    file_name = get_file_cname(file)
                    file_size = os.path.getsize(file_path)
                    f.write('unsigned char {}[{}] = {{\n'.format(file_name, file_size))               
                    num = 0
                    for byte in file_data :
                        f.write(hex(ord(byte)) + ', ')
                        num += 1
                        if 0 == num%16:
                            f.write('\n')
                    f.write('\n};\n')
            else :
                genutil.fmtError("Input file not found: '{}'".format(file_path))

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr) :
    if genutil.isDirty(Version, [input], [out_src, out_hdr]) :
        with open(input, 'r') as f :
            desc = yaml.load(f)
        gen_header(out_hdr, desc['files'])
        gen_source(out_src, desc['files'])
