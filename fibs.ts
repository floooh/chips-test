// deno-lint-ignore no-unversioned-import
import { Configurer, Builder } from 'jsr:@floooh/fibs';
import { addNesTestLogJob } from './fibs-scripts/nestestlog.ts';
import { addFuseJob } from './fibs-scripts/fuse.ts';
import { addVic20TestCommand } from './fibs-scripts/vic20-via-tests.ts';
import { addC64TestCommand } from './fibs-scripts/c64-wlspecial-tests.ts';
import { addWebpageCommand } from './fibs-scripts/webpage.ts';

export function configure(c: Configurer) {
    c.addImport({
        name: 'chips',
        url: 'https://github.com/floooh/chips',
    });
    c.addImport({
        name: 'dcimgui',
        url: 'https://github.com/floooh/dcimgui',
        files: ['fibs-docking.ts'],
    });
    c.addImport({
        name: 'libs',
        url: 'https://github.com/floooh/fibs-libs',
        files: ['sokol.ts'],
    });
    c.addImport({
        name: 'platforms',
        url: 'https://github.com/floooh/fibs-platforms',
        files: ['emscripten.ts'],
    });
    c.addImport({
        name: 'utils',
        url: 'https://github.com/floooh/fibs-utils',
        files: ['stdoptions.ts', 'sokolshdc.ts', 'embedfiles.ts'],
    });
    addVic20TestCommand(c);
    addC64TestCommand(c);
    addNesTestLogJob(c);
    addFuseJob(c);
    addWebpageCommand(c);
}

export function build(b: Builder) {
    b.addCmakeVariable('CMAKE_CXX_STANDARD', '20');
    if (b.isMsvc()) {
        b.addCompileOptions([
            '/wd4244',      // conversion from X to Y, possible loss of data
            '/wd4267',      // ditto
            '/wd4324',      // structure was padded due to alignment specifier
            '/wd4200',      // non-standard extension used: zero sized array
            '/wd4201',      // non-standard extension used: nameless struct/union
            '/wd4702',      // unreachable code
        ]);
    }
    // an interface lib for the chips headers
    b.addTarget('chips', 'interface', (t) => {
        t.setDir(b.importDir('chips'));
        t.addIncludeDirectories(['.']);
    });
    addCommon(b);
    addRoms(b);
    addEmulators(b);
    if (b.isLinux() || b.isMacOS()) {
        addAsciiEmulators(b);
    }
    if (!b.isEmscripten()) {
        addTests(b);
        addTools(b);
    }
}

function addEmulators(b: Builder) {
    const dir = 'examples/emus';

    // regular emulators
    const emus = ['z1013', 'z9001', 'atom', 'c64', 'vic20', 'zx', 'cpc', 'bombjack', 'pacman', 'pengo'];
    for (const emu of emus) {
        // emulator without UI
        b.addTarget(emu, 'windowed-exe', (t) => {
            t.setDir(dir);
            t.addSources([`${emu}.c`]);
            t.addDependencies(['common', 'roms']);
        });
        // emulator with UI
        b.addTarget(`${emu}-ui`, 'windowed-exe', (t) => {
            t.setDir(dir);
            t.addSources([`${emu}.c`, `${emu}-ui-impl.cc`]);
            t.addCompileDefinitions({ CHIPS_USE_UI: '1' });
            t.addDependencies(['ui', 'roms']);
        });
    }

    // special cases
    const kc85Models = [
        { name: 'kc852', def: 'CHIPS_KC85_TYPE_2' },
        { name: 'kc853', def: 'CHIPS_KC85_TYPE_3' },
        { name: 'kc854', def: 'CHIPS_KC85_TYPE_4' },
    ];
    for (const kc85Model of kc85Models) {
        b.addTarget(kc85Model.name, 'windowed-exe', (t) => {
            t.setDir(dir);
            t.addSources(['kc85.c']);
            t.addCompileDefinitions({[kc85Model.def]: '1'});
            t.addDependencies(['common', 'roms']);
        });
        b.addTarget(`${kc85Model.name}-ui`, 'windowed-exe', (t) => {
            t.setDir(dir);
            t.addSources(['kc85.c', 'kc85-ui-impl.cc']);
            t.addCompileDefinitions({
                [kc85Model.def]: '1',
                CHIPS_USE_UI: '1'
            });
            t.addDependencies(['ui', 'roms']);
        });
    }
    b.addTarget('lc80', 'windowed-exe', (t) => {
        t.setDir(dir);
        t.addSources(['lc80.c', 'lc80-ui-impl.cc']);
        t.addDependencies(['ui', 'roms']);
    });
}

function addAsciiEmulators(b: Builder) {
    // NOTE: these emulators use sokol_arg.h and sokol_time.h,
    // but don't link with sokol since this would also
    // bring in 3D API and window system dependencies
    const dir = 'examples/ascii';
    const libs = ['curses'];
    const deps = ['chips', 'keybuf', 'roms'];
    const incl = [b.importDir('sokol')];
    b.addTarget('kc85-ascii', 'plain-exe', (t) => {
        t.setDir(dir);
        t.addSources(['kc85-ascii.c']);
        t.addLibraries(libs);
        t.addDependencies(deps);
        t.addIncludeDirectories(incl);
    });
    b.addTarget('c64-ascii', 'plain-exe', (t) => {
        t.setDir(dir);
        t.addSources(['c64-ascii.c']);
        t.addLibraries(libs);
        t.addDependencies(deps);
        t.addIncludeDirectories(incl);
    });
    b.addTarget('c64-sixel', 'plain-exe', (t) => {
        t.setDir(dir);
        t.addSources(['c64-sixel.c']);
        t.addLibraries(libs);
        t.addDependencies(deps);
        t.addIncludeDirectories(incl);
    });
    b.addTarget('c64-kitty', 'plain-exe', (t) => {
        t.setDir(dir);
        t.addSources(['c64-kitty.c']);
        t.addLibraries(libs);
        t.addDependencies(deps);
        t.addIncludeDirectories(incl);
    });
}

function addCommon(b: Builder) {
    const dir = 'examples/common';
    b.addTarget('keybuf', 'lib', (t) => {
        t.setDir(dir);
        t.addSources(['keybuf.c', 'keybuf.h']);
        t.addIncludeDirectories({ dirs: ['.'], scope: 'interface'});
    });
    b.addTarget('webapi', 'lib', (t) => {
        t.setDir(dir);
        t.addSources(['webapi.c', 'webapi.h']);
        t.addIncludeDirectories({ dirs: ['.'], scope: 'interface'});
        t.addDependencies(['chips']);
    });
    b.addTarget('common', 'lib', (t) => {
        t.setDir(dir);
        t.addSources([
            'common.h',
            'sokol.c',
            'clock.c', 'clock.h',
            'fs.c', 'fs.h',
            'gfx.c', 'gfx.h',
            'prof.c', 'prof.h',

        ]);
        t.addJob({ job: 'sokolshdc', args: { src: 'shaders.glsl', outDir: t.buildDir() } });
        t.addIncludeDirectories({ dirs: [t.buildDir()], scope: 'private'});
        t.addDependencies(['keybuf', 'webapi', 'sokol']);
    });
    b.addTarget('ui', 'lib', (t) => {
        t.setDir(dir);
        t.addSources([ 'ui.cc', 'ui.h' ]);
        t.addDependencies(['imgui-docking', 'common']);
    });
}

function addTools(b: Builder) {
    const dir = 'tools';
    const type = 'plain-exe';
    b.addTarget({ name: 'prgmerge', type, dir, sources: ['prgmerge.c', 'getopt.c', 'getopt.h'] });
    b.addTarget({ name: 'png2bits', type, dir, sources: ['png2bits.c', 'getopt.c', 'getopt.h', 'stb_image.h'] });
}

function addRoms(b: Builder) {
    const roms = {
        atom: [
            'abasic.ic20',
            'afloat.ic21',
            'dosrom.u15'
        ],
        bombjack: [
            'bombjack/01_h03t.bin',
            'bombjack/02_p04t.bin',
            'bombjack/03_e08t.bin',
            'bombjack/04_h08t.bin',
            'bombjack/05_k08t.bin',
            'bombjack/06_l08t.bin',
            'bombjack/07_n08t.bin',
            'bombjack/08_r08t.bin',
            'bombjack/09_j01b.bin',
            'bombjack/10_l01b.bin',
            'bombjack/11_m01b.bin',
            'bombjack/12_n01b.bin',
            'bombjack/13.1r',
            'bombjack/14_j07b.bin',
            'bombjack/15_l07b.bin',
            'bombjack/16_m07b.bin'
        ],
        c1541: [
            '1541_c000_325302_01.bin',
            '1541_e000_901229_06aa.bin'
        ],
        c64: [
            'c64_basic.bin',
            'c64_char.bin',
            'c64_kernalv3.bin',
        ],
        cp4: [
            'cp4_basic.318006_01.bin',
            'cp4_kernal.318004_05.bin',
            'cp4_3_plus_1.317053_01.bin',
            'cp4_3_plus_1.317054_01.bin',
            'cp4_pla.251641_02.bin'
        ],
        cpc: [
            'cpc464_os.bin',
            'cpc464_basic.bin',
            'cpc6128_basic.bin',
            'cpc6128_os.bin',
            'cpc6128_amsdos.bin',
            'kcc_os.bin',
            'kcc_bas.bin',
        ],
        kc85: [
            'caos22.852',
            'caos31.853',
            'caos42c.854',
            'caos42e.854',
            'basic_c0.853'
        ],
        lc80: [
            'lc80_2k.bin',
        ],
        pacman: [
            'pacman/82s123.7f',
            'pacman/82s126.1m',
            'pacman/82s126.3m',
            'pacman/82s126.4a',
            'pacman/pacman.5e',
            'pacman/pacman.5f',
            'pacman/pacman.6e',
            'pacman/pacman.6f',
            'pacman/pacman.6h',
            'pacman/pacman.6j',
        ],
        pengo: [
            'pengo/ep1640.92',
            'pengo/ep1695.105',
            'pengo/ep5120.8',
            'pengo/ep5121.7',
            'pengo/ep5122.15',
            'pengo/ep5123.14',
            'pengo/ep5124.21',
            'pengo/ep5125.20',
            'pengo/ep5126.32',
            'pengo/ep5127.31',
            'pengo/pr1633.78',
            'pengo/pr1634.88',
            'pengo/pr1635.51',
            'pengo/pr1636.70',
        ],
        vic20: [
            'vic20_basic_901486_01.bin',
            'vic20_characters_901460_03.bin',
            'vic20_kernal_901486_07.bin',
        ],
        z1013: [
            'z1013_font.bin',
            'z1013_mon_a2.bin',
            'z1013_mon202.bin',
        ],
        z9001: [
            'kc87_os_2.bin',
            'kc87_font_2.bin',
            'z9001_basic.bin',
            'z9001_os12_1.bin',
            'z9001_os12_2.bin',
            'z9001_font.bin',
            'z9001_basic_507_511.bin',
        ],
        zx: [
            'amstrad_zx128k_0.bin',
            'amstrad_zx128k_1.bin',
            'amstrad_zx48k.bin',
        ],
    };

    b.addTarget('roms', 'interface', (t) => {
        t.setDir('examples/roms');
        t.addIncludeDirectories(['.']);
        for (const [name, files] of Object.entries(roms)) {
            t.addJob({
                job: 'embedfiles',
                args: {
                    outHeader: `${name}-roms.h`,
                    prefix: 'dump_',
                    asConst: false,
                    files,
                }
            })
        }
    });
}

function addTests(b: Builder) {
    const dir = 'tests';
    const type = 'plain-exe';
    b.addTarget({ name: 'z80-test', dir, type, sources: ['z80-test.c'], deps: ['chips'] });
    b.addTarget({ name: 'z80-int', dir, type, sources: ['z80-int.c'], deps: ['chips'] });
    b.addTarget({ name: 'm6502-int', dir, type, sources: ['m6502-int.c'], deps: ['chips'] });
    b.addTarget({ name: 'z80-timing', dir, type, sources: ['z80-timing.c'], deps: ['chips'] });
    b.addTarget('chips-test', type, (t) => {
        t.setDir(dir);
        t.addSources([
            'chips-test.c',
            'kbd-test.c',
            'mem-test.c',
            'fdd-test.c',
            'upd765-test.c',
            'ay38910-test.c',
            'i8255-test.c',
            'mc6847-test.c',
            'mc6845-test.c',
            'm6569-test.c',
            'm6581-test.c',
            'z80ctc-test.c',
            'z80pio-test.c',
            'm6502dasm-test.c',
            'z80dasm-test.c',
            'm6502-test.c',
        ]);
        t.addJob({
            job: 'embedfiles',
            args: {
                dir: 'disks',
                outHeader: 'fdd-test.h',
                prefix: 'dump_',
                asConst: false,
                files: ['boulderdash_cpc.dsk', 'dtc_cpc.dsk' ],
            }
        });
        t.addDependencies(['chips']);
    });
    b.addTarget('z80-zex', type, (t) => {
        t.setDir(dir);
        t.addSources(['z80-zex.c']);
        t.addJob({
            job: 'embedfiles',
            args: {
                dir: 'roms',
                outHeader: 'zex-dump.h',
                prefix: 'dump_',
                asConst: false,
                files: ['zexall.com', 'zexdoc.com'],
            }
        });
        t.addIncludeDirectories([b.importDir('sokol')]);
        t.addDependencies(['chips']);
    });
    b.addTarget('c64-bench', type, (t) => {
        t.setDir(dir);
        t.addSources(['c64-bench.c']);
        t.addIncludeDirectories([b.importDir('sokol')]);
        t.addDependencies(['chips', 'roms']);
    });
    b.addTarget('m6502-perfect', type, (t) => {
        t.setDir(dir);
        t.addSources([
            'm6502-perfect.c',
            'perfect6502/netlist_6502.h',
            'perfect6502/netlist_sim.c',
            'perfect6502/netlist_sim.h',
            'perfect6502/perfect6502.c',
            'perfect6502/perfect6502.h',
            'perfect6502/types.h'
        ]);
        t.addDependencies(['chips']);
    });
    b.addTarget('m6502-wltest', type, (t) => {
        t.setDir(dir);
        t.addSources(['m6502-wltest.c']);
        t.addIncludeDirectories([b.importDir('sokol')]);
        t.addDependencies(['chips']);
        t.addJob({
            job: 'embedfiles',
            args: {
                dir: `testsuite-2.15/bin`,
                outHeader: 'dump.h',
                prefix: 'dump_',
                list: true,
                asConst: false,
                // deno-fmt-ignore
                files: [
                    '_start', 'adca', 'adcax', 'adcay', 'adcb', 'adcix', 'adciy', 'adcz', 'adczx', 'alrb', 'ancb', 'anda',
                    'andax', 'anday', 'andb', 'andix', 'andiy', 'andz', 'andzx', 'aneb', 'arrb', 'asla', 'aslax', 'asln',
                    'aslz', 'aslzx', 'asoa', 'asoax', 'asoay', 'asoix', 'asoiy', 'asoz', 'asozx', 'axsa', 'axsix', 'axsz',
                    'axszy', 'bccr', 'bcsr', 'beqr', 'bita', 'bitz', 'bmir', 'bner', 'bplr', 'branchwrap', 'brkn', 'bvcr',
                    'bvsr', 'cia1pb6', 'cia1pb7', 'cia1ta', 'cia1tab', 'cia1tb', 'cia1tb123',  'cia2pb6', 'cia2pb7',
                    'cia2ta', 'cia2tb', 'cia2tb123', 'clcn', 'cldn', 'clin', 'clvn', 'cmpa', 'cmpax', 'cmpay', 'cmpb',
                    'cmpix', 'cmpiy', 'cmpz', 'cmpzx', 'cntdef', 'cnto2', 'cpuport', 'cputiming', 'cpxa', 'cpxb', 'cpxz',
                    'cpya', 'cpyb', 'cpyz', 'dcma', 'dcmax', 'dcmay', 'dcmix', 'dcmiy', 'dcmz', 'dcmzx', 'deca', 'decax',
                    'decz', 'deczx', 'dexn', 'deyn', 'eora', 'eorax', 'eoray', 'eorb', 'eorix', 'eoriy', 'eorz', 'eorzx',
                    'finish', 'flipos', 'icr01', 'imr', 'inca', 'incax', 'incz', 'inczx', 'insa', 'insax', 'insay', 'insix',
                    'insiy', 'insz', 'inszx', 'inxn', 'inyn', 'irq', 'jmpi', 'jmpw', 'jsrw', 'lasay', 'laxa', 'laxay',
                    'laxix', 'laxiy', 'laxz', 'laxzy', 'ldaa', 'ldaax', 'ldaay', 'ldab', 'ldaix', 'ldaiy', 'ldaz', 'ldazx',
                    'ldxa', 'ldxay', 'ldxb', 'ldxz', 'ldxzy', 'ldya', 'ldyax', 'ldyb', 'ldyz', 'ldyzx', 'loadth', 'lsea',
                    'lseax', 'lseay', 'lseix', 'lseiy', 'lsez', 'lsezx', 'lsra', 'lsrax', 'lsrn', 'lsrz', 'lsrzx', 'lxab',
                    'mmu', 'mmufetch', 'nmi', 'nopa', 'nopax', 'nopb', 'nopn', 'nopz', 'nopzx', 'oneshot', 'oraa', 'oraax',
                    'oraay', 'orab', 'oraix', 'oraiy', 'oraz', 'orazx', 'phan', 'phpn', 'plan', 'plpn', 'rlaa', 'rlaax',
                    'rlaay', 'rlaix', 'rlaiy', 'rlaz', 'rlazx', 'rola', 'rolax', 'roln', 'rolz', 'rolzx', 'rora', 'rorax',
                    'rorn', 'rorz', 'rorzx', 'rraa', 'rraax', 'rraay', 'rraix', 'rraiy', 'rraz', 'rrazx', 'rtin', 'rtsn',
                    'sbca', 'sbcax', 'sbcay', 'sbcb', 'sbcb_eb', 'sbcix', 'sbciy', 'sbcz', 'sbczx', 'sbxb', 'secn', 'sedn',
                    'sein', 'shaay', 'shaiy', 'shsay', 'shxay', 'shyax', 'staa', 'staax', 'staay', 'staix', 'staiy', 'staz',
                    'stazx', 'stxa', 'stxz', 'stxzy', 'stya', 'styz', 'styzx', 'taxn', 'tayn', 'trap1', 'trap10', 'trap11',
                    'trap12', 'trap13', 'trap14', 'trap15', 'trap16', 'trap17', 'trap2', 'trap3', 'trap4', 'trap5', 'trap6',
                    'trap7', 'trap8', 'trap9', 'tsxn', 'txan', 'txsn', 'tyan',
                ],
            },
        })
    });
    b.addTarget('m6502-nestest', type, (t) => {
        t.setDir(dir);
        t.addSources(['m6502-nestest.c']);
        t.addDependencies(['chips']);
        t.addJob({
            job: 'nestestlog',
            args: {
                dir: 'nestest',
                input: 'nestest.log.txt',
                outHeader: 'nestestlog.h',
            }
        });
        t.addJob({
            job: 'embedfiles',
            args: {
                dir: 'nestest',
                outHeader: 'dump.h',
                files: ['nestest.nes'],
                prefix: 'dump_',
                asConst: false,
            }
        });
    });
    b.addTarget('z80-fuse', type, (t) => {
        t.setDir(dir);
        t.addSources(['z80-fuse.c']);
        t.addDependencies(['chips']);
        t.addJob({
            job: 'fuse',
            args: {
                dir: 'fuse',
                fuse_input_file: 'tests.in',
                fuse_expected_file: 'tests.expected',
                outHeader: 'fuse.h',
            }
        });
    });
}
