// helper command to run C64 tests
// deno-lint-ignore-file no-unversioned-import
import { Configurer, Project, log } from 'jsr:@floooh/fibs';

export function addC64TestCommand(c: Configurer) {
    c.addCommand({ name: 'c64-wlspecial-tests', help, run });
}

function help() {
    log.helpCmd([
        'c64-wlspecial-tests',
        'c64-wlspecial-tests [irg imr ...]',
    ], 'run all or selected C64 tests');
}

async function run(p: Project, args: string[]): Promise<void> {
    const testDir = 'tests/testsuite-2.15/bin/';
    const allTests = [
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
    ];
    let tests;
    if (args.length === 1) {
        tests = allTests;
    } else {
        tests = allTests.filter((t) => args.slice(1).includes(t));
    }
    log.info(`# running tests ${tests.join(' ')}`);
    for (const test of tests) {
        const c = p.activeConfig();
        const path = `${p.dir()}/${testDir}/${test}`;
        await c.runner.run(p, c, p.target('c64-ui'), { args: [`file=${path}`, 'input=RUN\r']});
    }
}
