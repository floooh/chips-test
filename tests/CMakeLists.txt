include_directories(../examples/roms)

fips_begin_app(m6502-nestest cmdline)
    fips_files(m6502-nestest.c)
    fips_dir(nestest)
    fips_generate(FROM nestest.log.txt TYPE nestestlog HEADER nestestlog.h)
    fipsutil_embed(dump.yml dump.h)
fips_end_app()

fips_begin_app(z80-fuse cmdline)
    fips_files(z80-fuse.c)
    fips_dir(fuse)
    fips_generate(FROM fuse.yml TYPE fuse HEADER fuse.h)
fips_end_app()
