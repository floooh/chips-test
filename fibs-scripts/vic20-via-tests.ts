// helper command to run VIC20 VIA tests
import { Configurer, Project, log } from 'jsr:@floooh/fibs@^1';

export function addVic20TestCommand(c: Configurer) {
    c.addCommand({ name: 'vic20-via-tests', help, run });
}

function help() {
    log.helpCmd([
        'vic20-via-tests',
        'vic20-via-tests [via1 via2 ...]',
    ], 'run vic20 via tests');
}

async function run(p: Project, args: string[]): Promise<void> {
    const testDir = 'tests/vice-tests/VIC20/'
    const allTests = [
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
    ];
    let tests;
    if (args.length === 1) {
        tests = allTests;
    } else {
        tests = allTests.filter((t) => {
            const filename = t.match(/viavarious\/(.+)\.prg$/)?.[1];
            return filename && args.slice(1).some((a) => filename === a);
        });
    }
    for (const test of tests) {
        const c = p.activeConfig();
        const path = `${p.dir()}/${testDir}/${test}`;
        await c.runner.run(p, c, p.target('vic20-ui'), { args: ['exp=ram8k', `file=${path}`] });
    }
}