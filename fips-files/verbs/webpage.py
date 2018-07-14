"""fips verb to build the samples webpage"""

import os
import yaml 
import shutil
import subprocess
import glob
from string import Template

from mod import log, util, project

samples = [
    [ 'ZX Spectrum 128', 'zx128k', ''],
    [ 'Commodore C64', 'c64', ''],
    [ 'Amstrad CPC6128', 'cpc6128', ''],
    [ 'Acorn Atom', 'atom', ''],
    [ 'Robotron Z1013', 'z1013', 'J 300[Enter] => BASIC'],
    [ 'Robotron KC87', 'kc87', 'BASIC[Enter]'],
]
GitHubSamplesURL = 'https://github.com/floooh/chips-test/master/examples'
BuildConfig = 'wasm-ninja-release'

#-------------------------------------------------------------------------------
def deploy_webpage(fips_dir, proj_dir, webpage_dir) :
    """builds the final webpage under under fips-deploy/chips-webpage"""
    ws_dir = util.get_workspace_dir(fips_dir)

    # build the thumbnail gallery
    content = ''
    for sample in samples :
        display_name = sample[0]
        name = sample[1]
        note = sample[2]
        log.info('> adding thumbnail for {}'.format(name))
        img_name = name + '.jpg'
        img_path = proj_dir + '/webpage/' + img_name
        if not os.path.exists(img_path):
            img_name = 'dummy.jpg'
            img_path = proj_dir + 'webpage/dummy.jpg'
        content += '<div class="thumb">\n'
        content += '  <div class="thumb-title">{}</div>\n'.format(display_name)
        content += '  <div class="img-frame"><a href="{}.html"><img class="image" src="{}"></img></a></div>\n'.format(name,img_name)
        content += '  <div class="thumb-bar">{}</div>\n'.format(note)
        content += '</div>\n'

    # populate the html template, and write to the build directory
    with open(proj_dir + '/webpage/index.html', 'r') as f :
        templ = Template(f.read())
    html = templ.safe_substitute(samples=content)
    with open(webpage_dir + '/index.html', 'w') as f :
        f.write(html)

    # and the same with the CSS template
    with open(proj_dir + '/webpage/style.css', 'r') as f :
        templ = Template(f.read())
    css = templ.safe_substitute()
    with open(webpage_dir +'/style.css', 'w') as f :
        f.write(css)

    # copy other required files
    for name in ['dummy.jpg', 'emsc.js', 'favicon.png'] :
        log.info('> copy file: {}'.format(name))
        shutil.copy(proj_dir + '/webpage/' + name, webpage_dir + '/' + name)

    # generate sample HTML pages
    emsc_deploy_dir = '{}/fips-deploy/chips-test/{}'.format(ws_dir, BuildConfig)
    for sample in samples :
        display_name = sample[0]
        name = sample[1]
        log.info('> generate emscripten HTML page: {}'.format(name))
        for ext in ['wasm', 'js'] :
            src_path = '{}/{}.{}'.format(emsc_deploy_dir, name, ext)
            if os.path.isfile(src_path) :
                shutil.copy(src_path, '{}/'.format(webpage_dir))
        with open(proj_dir + '/webpage/emsc.html', 'r') as f :
            templ = Template(f.read())
        src_url = GitHubSamplesURL + name
        html = templ.safe_substitute(name=display_name, prog=name, source=src_url)
        with open('{}/{}.html'.format(webpage_dir, name, name), 'w') as f :
            f.write(html)

    # copy the screenshots
    for sample in samples :
        img_name = sample[1] + '.jpg'
        img_path = proj_dir + '/webpage/' + img_name
        if os.path.exists(img_path):
            log.info('> copy screenshot: {}'.format(img_name))
            shutil.copy(img_path, webpage_dir + '/' + img_name)

    # copy the game files
    for sample in samples:
        src_dir = proj_dir + '/webpage/' + sample[1]
        if os.path.exists(src_dir):
            dst_dir = webpage_dir + '/' + sample[1]
            if os.path.isdir(dst_dir):
                shutil.rmtree(dst_dir)
            shutil.copytree(src_dir, dst_dir)

#-------------------------------------------------------------------------------
def build_deploy_webpage(fips_dir, proj_dir, rebuild) :
    # if webpage dir exists, clear it first
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/chips-webpage'.format(ws_dir)
    if rebuild :
        if os.path.isdir(webpage_dir) :
            shutil.rmtree(webpage_dir)
    if not os.path.isdir(webpage_dir) :
        os.makedirs(webpage_dir)

    # compile samples
    project.gen(fips_dir, proj_dir, BuildConfig)
    project.build(fips_dir, proj_dir, BuildConfig)
    
    # deploy the webpage
    deploy_webpage(fips_dir, proj_dir, webpage_dir)

    log.colored(log.GREEN, 'Generated Samples web page under {}.'.format(webpage_dir))

#-------------------------------------------------------------------------------
def serve_webpage(fips_dir, proj_dir) :
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/chips-webpage'.format(ws_dir)
    p = util.get_host_platform()
    if p == 'osx' :
        try :
            subprocess.call(
                'open http://localhost:8000 ; python {}/mod/httpserver.py'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt :
            pass
    elif p == 'win':
        try:
            subprocess.call(
                'cmd /c start http://localhost:8000 && python {}/mod/httpserver.py'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass
    elif p == 'linux':
        try:
            subprocess.call(
                'xdg-open http://localhost:8000; python {}/mod/httpserver.py'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    if len(args) > 0 :
        if args[0] == 'build' :
            build_deploy_webpage(fips_dir, proj_dir, False)
        elif args[0] == 'rebuild' :
            build_deploy_webpage(fips_dir, proj_dir, True)
        elif args[0] == 'serve' :
            serve_webpage(fips_dir, proj_dir)
        else :
            log.error("Invalid param '{}', expected 'build' or 'serve'".format(args[0]))
    else :
        log.error("Param 'build' or 'serve' expected")

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
             'fips webpage build\n' +
             'fips webpage rebuild\n' +
             'fips webpage serve\n' +
             log.DEF +
             '    build chips samples webpage')

