fips_begin_app(prgmerge cmdline)
    fips_files(prgmerge.c getopt.c getopt.h)
fips_end_app()

fips_begin_app(png2bits cmdline)
    fips_files(png2bits.c getopt.c getopt.h stb_image.h)
fips_end_app()
