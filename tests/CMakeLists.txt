fips_begin_app(z80-test cmdline)
    fips_files(z80-test.c)
fips_end_app()

fips_begin_app(z80-zex cmdline)
    fips_files(z80-zex.c)
    fips_generate(FROM zex-dump.yml TYPE dump)
fips_end_app()

