include_directories(../examples/roms)

fips_begin_app(chips-test cmdline)
    fips_vs_warning_level(3)
    fips_files(
        chips-test.c 
        kbd-test.c
        mem-test.c
        fdd-test.c
        upd765-test.c
        ay38910-test.c
        i8255-test.c 
        mc6847-test.c 
        mc6845-test.c
        m6569-test.c
        m6581-test.c
        z80ctc-test.c
        z80pio-test.c
        m6502dasm-test.c
        z80dasm-test.c
        z80-int.c
        z80-test.c
        m6502-test.c
        m6502-perfect.c
    )
    fips_dir(perfect6502)
    fips_files(
        netlist_6502.h
        netlist_sim.c netlist_sim.h
        perfect6502.c perfect6502.h
        types.h
    )
    fips_dir(disks)
    fips_generate(FROM fdd-test.yml TYPE dump HEADER fdd-test.h)
fips_end_app()

fips_begin_app(z80-zex cmdline)
    fips_vs_warning_level(3)
    fips_files(z80-zex.c)
    fips_dir(roms)
    fips_generate(FROM zex-dump.yml TYPE dump HEADER zex-dump.h)
fips_end_app()

fips_begin_app(m6502-nestest cmdline)
    fips_vs_warning_level(3)
    fips_files(m6502-nestest.c)
    fips_dir(nestest)
    fips_generate(FROM nestest.log.txt TYPE nestestlog HEADER nestestlog.h)
    fips_generate(FROM dump.yml TYPE dump HEADER dump.h)
fips_end_app()

fips_begin_app(m6502-wltest cmdline)
    fips_vs_warning_level(3)
    fips_files(m6502-wltest.c)
    fips_dir(testsuite-2.15/bin)
    fips_generate(FROM dump.yml TYPE dump HEADER dump.h)
fips_end_app()

fips_begin_app(z80-wait cmdline)
    fips_vs_warning_level(3)
    fips_files(z80-wait.c)
fips_end_app()

fips_begin_app(z80-fuse cmdline)
    fips_vs_warning_level(3)
    fips_files(z80-fuse.c)
    fips_dir(fuse)
    fips_generate(FROM fuse.yml TYPE fuse HEADER fuse.h)
fips_end_app()

fips_begin_app(c64-bench cmdline)
    fips_vs_warning_level(3)
    fips_files(c64-bench.c)
    fips_deps(roms)
fips_end_app()
