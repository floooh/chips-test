import os

from mod import log, util, project, config

TestDir = '/tests/testsuite-2.15/bin/'

Tests = [
    'branchwrap',
    'cia1pb6',
    'cia1pb7',
    'cia1tab',
    'cia1tb123',
    'cia2pb6',
    'cia2pb7',
    'cia2tb123',
    'cntdef',
    'cnto2',
    'cpuport',
    'cputiming',
    'flipos',
    'icr01',
    'imr',
    'irq',
    'loadth',
    'mmu',
    'mmufetch',
    'nmi',
    'oneshot',
    'trap1',
    'trap2',
    'trap3',
    'trap4',
    'trap5',
    'trap6',
    'trap7',
    'trap8',
    'trap9',
    'trap10',
    'trap11',
    'trap12',
    'trap13',
    'trap14',
    'trap15',
    'trap16',
    'trap17',
]

#-------------------------------------------------------------------------------
def match_test(test_name, whitelist):
    if not whitelist:
        return True
    else:
        for substr in whitelist:
            if substr in test_name:
                return True
    return False

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    cfg = config.get_default_config();
    filt = args
    for test in Tests:
        if match_test(test, args):
            path = proj_dir + TestDir + test
            print("> running test '{}':".format(test))
            project.run(fips_dir, proj_dir, cfg, 'c64-ui', ['file={}'.format(path), 'input=RUN\r'], None)

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
        'fips c64-wlspecial-tests\n' +
        'fips c64-wlspecial-tests irq imr ...\n' +
        log.DEF +
        '    run all or selected C64 tests')

