import os

from mod import log, util, project, config

TestDir = '/tests/vice-tests/VIC20/'

Tests = [
    'viavarious/via1.prg',
    'viavarious/via2.prg',
    'viavarious/via3.prg',
    'viavarious/via3a.prg',
    'viavarious/via4.prg',
    'viavarious/via4a.prg',
    'viavarious/via5.prg',
    'viavarious/via5a.prg',
    'viavarious/via9.prg',
    'viavarious/via10.prg',
    'viavarious/via11.prg',
    'viavarious/via12.prg',
    'viavarious/via13.prg',
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
            project.run(fips_dir, proj_dir, cfg, 'vic20-ui', ['exp=ram8k', 'file={}'.format(path)], None)

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
        'fips vic20-via-tests\n' +
        'fips vic20-via-tests via1 via2 ...' +
        log.DEF +
        '    run all or selected VIC-20 VIA tests from the VICE test bench')

