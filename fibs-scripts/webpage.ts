// fibs command to build and serve the emulators webpage
// deno-lint-ignore-file no-unversioned-import
import { Configurer, log, proj, Project, util } from "jsr:@floooh/fibs";
import { green } from "jsr:@std/fmt/colors";
import { dirname } from "jsr:@std/path";

export function addWebpageCommand(c: Configurer) {
    c.addCommand({ name: "webpage", help, run });
}

function help() {
    log.helpCmd([
        "webpage build",
        "webpage serve",
    ], "build or serve webpage");
}

async function run(p: Project, args: string[]): Promise<void> {
    const subcmd = args[1];
    const configName = "emsc-ninja-release";
    const config = p.config(configName);
    const srcDir = p.distDir(configName);
    const dstDir = `${p.fibsDir()}/webpage`;
    if (subcmd === "build") {
        if (util.dirExists(dstDir)) {
            if (Deno.env.get('CI') || log.ask(`Ok to delete directory ${dstDir}?`, false)) {
                Deno.removeSync(dstDir, { recursive: true });
            }
        }
        util.ensureDir(dstDir);
        await proj.generate(config);
        await proj.build({});
        await deployWebpage(p, srcDir, dstDir);
        log.info(`\n${green("Done.")} (webpage available in ${dstDir})`);
    } else if (subcmd === "serve") {
        const emsc = p.importModule("extras", "emscripten.ts");
        emsc.emrun(p, { cwd: dstDir, file: "index.html" });
    } else {
        throw new Error("subcommand 'build' or 'serve' expected (run 'fibs help webpage')");
    }
}

async function deployWebpage(p: Project, srcDir: string, dstDir: string): Promise<void> {
    const webpageDir = `${p.dir()}/webpage`;
    const emus = JSON.parse(Deno.readTextFileSync(`${webpageDir}/emulators.json`)) as Emu[];

    // everything that's deployed is derived from emulators.json:
    //  - the emscripten programs to generate html pages for (from 'wasm')
    //  - which of those also need a debug-UI version (from 'debug')
    //  - the asset files to deploy (from 'screenshot', 'params' and 'help')
    const progs = new Set<string>();
    const uiProgs = new Set<string>();
    const assets = new Set<string>();
    for (const emu of emus) {
        assets.add(emu.screenshot);
        if (emu.help !== undefined) {
            assets.add(emu.help);
        }
        for (const value of Object.values(emu.params)) {
            // params are either file references or plain emulator options
            if (util.fileExists(`${webpageDir}/${value}`)) {
                assets.add(value);
            }
        }
        const prog = progName(emu);
        if (prog === undefined) {
            // an externally hosted emulator (the visual*remix simulators), only linked, not deployed
            continue;
        }
        progs.add(prog);
        if (emu.debug !== false) {
            uiProgs.add(prog);
        }
    }

    // drop debug UIs without a build target (e.g. the lc80 has its UI built in)
    for (const prog of uiProgs) {
        if (!util.fileExists(`${srcDir}/${prog}-ui.js`)) {
            log.warn(`no '${prog}-ui' build found, disabling debug UI for '${prog}' entries`);
            uiProgs.delete(prog);
        }
    }

    // copy the static files of the webpage
    for (const file of siteFiles) {
        log.info(`> copy file: ${file}`);
        Deno.copyFileSync(`${webpageDir}/${file}`, `${dstDir}/${file}`);
    }

    // write emulators.json with the screenshots pointing to the converted webp
    // files, and with the debug flag fixed up to what has actually been built
    log.info('> write emulators.json');
    const deployedEmus = emus.map((emu) => {
        const prog = progName(emu);
        return {
            ...emu,
            screenshot: toWebpExt(emu.screenshot),
            debug: (prog !== undefined) && (emu.debug !== false) && uiProgs.has(prog),
        };
    });
    Deno.writeTextFileSync(`${dstDir}/emulators.json`, JSON.stringify(deployedEmus, null, 2));

    // generate an html page per emscripten program and copy the build artefacts
    const emscTmpl = Deno.readTextFileSync(`${webpageDir}/emsc.html`);
    for (const prog of [...progs].sort()) {
        for (const name of uiProgs.has(prog) ? [prog, `${prog}-ui`] : [prog]) {
            log.info(`> generate html page for: ${name}`);
            for (const ext of ['js', 'wasm']) {
                const srcPath = `${srcDir}/${name}.${ext}`;
                if (util.fileExists(srcPath)) {
                    Deno.copyFileSync(srcPath, `${dstDir}/${name}.${ext}`);
                } else {
                    log.warn(`build artefact not found: ${srcPath}`);
                }
            }
            Deno.writeTextFileSync(`${dstDir}/${name}.html`, emscTmpl.replaceAll('${prog}', name));
        }
    }

    // deploy the asset files (screenshots are converted from jpg to webp)
    for (const asset of [...assets].sort()) {
        const srcPath = `${webpageDir}/${asset}`;
        const dstPath = `${dstDir}/${toWebpExt(asset)}`;
        util.ensureDir(dirname(dstPath));
        if (!util.fileExists(srcPath)) {
            log.warn(`asset file not found: ${srcPath}`);
        } else if (asset.endsWith('.jpg')) {
            await cwebp(srcPath, dstPath);
        } else {
            log.info(`> copy ${srcPath} => ${dstPath}`);
            Deno.copyFileSync(srcPath, dstPath);
        }
    }
}

// the emscripten program of an emulator entry (undefined for externally hosted emulators)
function progName(emu: Emu): string | undefined {
    return /^https?:\/\//.test(emu.wasm) ? undefined : emu.wasm.replace(/\.html$/, '');
}

function toWebpExt(path: string): string {
    return path.replace(/\.jpg$/, '.webp');
}

async function cwebp(srcPath: string, dstPath: string): Promise<void> {
    log.info(`> cwebp ${srcPath} => ${dstPath}`)
    await util.runCmd('cwebp', {
        args: ['-quiet', '-q', '80', srcPath, '-o', dstPath ],
        showCmd: false,
    });
}

// an entry in webpage/emulators.json, this is the source of truth for what's deployed
type Emu = {
    id: string;
    type: string;
    title: string;
    description: string;
    system: string;
    systemName: string;
    screenshot: string;             // path of the thumbnail jpg (deployed as webp)
    wasm: string;                   // path of the emulator page, or url of an external emulator
    params: Record<string, string>; // url args for the emulator page
    help?: string;                  // path of a markdown help file
    debug?: boolean;                // false: no debug UI version exists
};

// the static files of the webpage (everything else is derived from emulators.json)
const siteFiles = [
    'index.html',
    'player.html',
    'site.js',
    'style.css',
    'favicon.svg',
    'favicon-32.png',
    'apple-touch-icon.png',
];
