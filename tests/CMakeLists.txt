include_directories(../examples/roms)

fips_begin_app(z80-fuse cmdline)
    fips_files(z80-fuse.c)
    fips_dir(fuse)
    fips_generate(FROM fuse.yml TYPE fuse HEADER fuse.h)
fips_end_app()
