fips_begin_lib(atom-roms)
    fips_generate(FROM atom-roms.yml TYPE dump SOURCE atom-roms.c HEADER atom-roms.h)
fips_end_lib()

fips_begin_lib(c64-roms)
    fips_generate(FROM c64-roms.yml TYPE dump SOURCE c64-roms.c HEADER c64-roms.h)
fips_end_lib()

fips_begin_lib(cpc-roms)
    fips_generate(FROM cpc-roms.yml TYPE dump SOURCE cpc-roms.c HEADER cpc-roms.h)
fips_end_lib()

fips_begin_lib(kc85-roms)
    fips_generate(FROM kc85-roms.yml TYPE dump SOURCE kc85-roms.c HEADER kc85-roms.h)
fips_end_lib()

fips_begin_lib(z1013-roms)
    fips_generate(FROM z1013-roms.yml TYPE dump SOURCE z1013-roms.c HEADER z1013-roms.h)
fips_end_lib()

fips_begin_lib(z9001-roms)
    fips_generate(FROM z9001-roms.yml TYPE dump SOURCE z9001-roms.c HEADER z9001-roms.h)
fips_end_lib()

fips_begin_lib(zx-roms)
    fips_generate(FROM zx-roms.yml TYPE dump SOURCE zx-roms.c HEADER zx-roms.h)
fips_end_lib()
