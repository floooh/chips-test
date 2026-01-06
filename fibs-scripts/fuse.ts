// a fibs code generator to turn fuse test files into a C header
import { Configurer, Project, Config, Target, Schema, log, util } from 'jsr:@floooh/fibs@^1';
import { dirname } from 'jsr:@std/path@^1';

export function addFuseJob(c: Configurer) {
    c.addJob({ name: 'fuse', help, validate, build });
}

type Args = {
    dir?: string,
    fuse_input_file: string,
    fuse_expected_file: string,
    outHeader: string,
};

const schema: Schema = {
    dir: { type: 'string', optional: true, desc: 'base dir of files to embed (default: target source dir)' },
    fuse_input_file: { type: 'string', optional: false, desc: 'the fuse tests.in file path' },
    fuse_expected_file: { type: 'string', optional: false, desc: 'the fuse tests.expected file path' },
    outHeader: { type: 'string', optional: false, desc: 'path of generated header file' },
};

function help() {
    log.helpJob('fuse', 'generate C header from fuse test files', schema);
}

function validate(args: unknown) {
    return util.validate(args, schema);
}

function tokenize(s: string): string[] {
    return s.trim().split(/\s+/).filter((t) => t.length > 0);
}

function build(_p: Project, _c: Config, t: Target, args: unknown) {
    const { dir = t.dir, fuse_input_file, fuse_expected_file, outHeader } = util.safeCast<Args>(args, schema);
    return {
        name: 'fuse',
        inputs: [`${dir}/${fuse_input_file}`, `${dir}/${fuse_expected_file}`],
        outputs: [`${dir}/${outHeader}`],
        addOutputsToTargetSources: true,
        args: { dir, fuse_input_file, fuse_expected_file, outHeader },
        func: async (inputs: string[], outputs: string[], _args: Args): Promise<void> => {
            if (!util.dirty(inputs, outputs)) {
                return;
            }
            util.ensureDir(dirname(outputs[0]));
            let str = '// machine generated, do not edit!\n';
            const items = [
                { file: inputs[0], cname: 'fuse_input' },
                { file: inputs[1], cname: 'fuse_expected' },
            ];
            const l = (s: string) => str += s;
            for (const item of items) {
                log.info(`# fuse ${item.file} => ${outputs[0]}`);
                const data = await Deno.readTextFile(item.file);
                const lines = data.split(/\r?\n/);
                let num_tests = 0;
                let tok: string[];
                l(`fuse_test_t ${item.cname}[] = {\n`);
                for (let li = 0; li < lines.length;) {
                    const nextLine = (): string => lines[li++] ?? '';
                    let line = nextLine();
                    if (line === '') {
                        continue;
                    }
                    l('  {\n');
                    num_tests += 1;
                    // description
                    str += `    .desc = "${line}",\n`
                    // optional events start with spaces
                    line = nextLine();
                    if (line[0] === ' ') {
                        let num_events = 0;
                        l('    .events = {\n');
                        while (line[0] === ' ') {
                            tok = tokenize(line);
                            if (tok.length === 4) {
                                l(`      { .tick=${tok[0]}, .type=EVENT_${tok[1]}, .addr=0x${tok[2]}, .data=0x${tok[3]} },\n`);
                                num_events += 1;
                            }
                            line = nextLine();
                        }
                        l('    },\n');
                        l(`    .num_events = ${num_events},\n`);
                    }
                    // 16-bit registers
                    tok = tokenize(line);
                    l(`    .state = {\n`);
                    if (tok.length === 12) {
                        l(`      .af=0x${tok[0]}, .bc=0x${tok[1]}, .de=0x${tok[2]}, .hl=0x${tok[3]},\n`);
                        l(`      .af_=0x${tok[4]}, .bc_=0x${tok[5]}, .de_=0x${tok[6]}, .hl_=0x${tok[7]},\n`);
                        l(`      .ix=0x${tok[8]}, .iy=0x${tok[9]}, .sp=0x${tok[10]}, .pc=0x${tok[11]},\n`);
                    }
                    // additional registers and flags
                    tok = tokenize(nextLine());
                    if (tok.length === 7) {
                        l(`      .i=0x${tok[0]}, .r=0x${tok[1]}, .iff1=${tok[2]}, .iff2=${tok[3]}, .im=${tok[4]}, .halted=${tok[5]}, .ticks=${tok[6]}\n`);
                    }
                    l('    },\n');
                    // optional memory chunks
                    let num_chunks = 0;
                    tok = tokenize(nextLine());
                    if (tok.length > 1) {
                        l(`    .chunks = {\n`);
                        while (tok.length > 0) {
                            if (tok[0] !== '-1') {
                                l(`      { .addr=0x${tok[0]}, .bytes = { `);
                                let i = 1;
                                while (tok[i] !== '-1') {
                                    l(`0x${tok[i]},`);
                                    i += 1;
                                }
                                l(`}, .num_bytes=${i-1}, },\n`);
                                num_chunks += 1;
                            }
                            tok = tokenize(nextLine());
                        }
                        l('    },\n');
                    }
                    l(`    .num_chunks = ${num_chunks},\n`);
                    l(`  },\n`);
                }
                l(`};\n`);
                l(`const int ${item.cname}_num = ${num_tests};\n`);
            }
            await Deno.writeTextFile(outputs[0], str);
        }
    }
}
