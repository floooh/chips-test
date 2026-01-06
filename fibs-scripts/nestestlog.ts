// a fibs code generator to turn a NESTest log file into a C header
import { Configurer, Project, Config, Target, Schema, log, util } from 'jsr:@floooh/fibs@^1';
import { dirname } from 'jsr:@std/path@^1';

export function addNesTestLogJob(c: Configurer) {
    c.addJob({ name: 'nestestlog', help, validate, build });
};

type Args = {
    dir?: string;
    input: string;
    outHeader: string;
};

const schema: Schema = {
    dir: { type: 'string', optional: true, desc: 'base dir of files to embed (default: target source dir)' },
    input: { type: 'string', optional: false, desc: 'the input nestestlog.txt file' },
    outHeader: { type: 'string', optional: false, desc: 'path of generated header file' },
};

function help() {
    log.helpJob('nestestlog', 'generate C header from nestest log file', schema);
}

function validate(args: unknown) {
    return util.validate(args, schema);
}

function build(_p: Project, _c: Config, t: Target, args: unknown) {
    const { dir = t.dir, input, outHeader } = util.safeCast<Args>(args, schema);
    return {
        name: 'nestestlog',
        inputs: [`${dir}/${input}`],
        outputs: [`${dir}/${outHeader}`],
        addOutputsToTargetSources: true,
        args: { dir, input, outHeader },
        func: async (inputs: string[], outputs: string[], _args: Args): Promise<void> => {
            if (!util.dirty(inputs, outputs)) {
                return;
            }
            log.info(`# nestestlog ${inputs[0]} => ${outputs[0]}`);
            const data = await Deno.readTextFile(inputs[0])
            const lines = data.split(/\r?\n/);
            util.ensureDir(dirname(outputs[0]));
            let str = '';
            str += '// machine generated, do not edit!\n';
            str += '#include <stdint.h>\n';
            str += 'typedef struct {\n';
            str += '    const char* desc;\n';
            str += '    uint16_t PC;\n';
            str += '    uint8_t A,X,Y,P,S;\n';
            str += '} cpu_state;\n';
            str += 'cpu_state state_table[] = {\n';
            for (const line of lines) {
                if (line.trim().length > 0) {
                    const desc = line.slice(16, 48);
                    const pc = line.slice(0, 4);
                    const a = line.slice(50, 52);
                    const x = line.slice(55, 57);
                    const y = line.slice(60, 62);
                    const p = line.slice(65, 67);
                    const s = line.slice(71, 73);
                    str += `  { "${desc}", 0x${pc}, 0x${a}, 0x${x}, 0x${y}, 0x${p}, 0x${s} },\n`;
                }
            }
            str += '};\n';
            await Deno.writeTextFile(outputs[0], str);
        },
    }
}