"""fips verb to build the samples webpage"""

import os
import yaml 
import shutil
import subprocess
import glob
from string import Template

from mod import log, util, project

systems = [ 'kc85', 'kc85-ui', 'zx', 'zx-ui', 'c64', 'c64-ui', 'cpc', 'cpc-ui', 'atom', 'atom-ui', 'z1013', 'z1013-ui', 'z9001', 'z9001-ui', 'bombjack' ]
items = [
    { 'type':'emu',  'title':'KC85/2', 'system':'kc85',             'url':'kc85.html?type=kc85_2', 'img':'kc85/kc85_2.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC85/3', 'system':'kc85',             'url':'kc85.html?type=kc85_3', 'img':'kc85/kc85_3.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC85/4', 'system':'kc85',             'url':'kc85.html?type=kc85_4', 'img':'kc85/kc85_4.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC Compact',      'system':'cpc',     'url':'cpc.html?type=kccompact', 'img':'cpc/kccompact.jpg', 'note':'' },
    { 'type':'emu',  'title':'Amstrad CPC464',  'system':'cpc',     'url':'cpc.html?type=cpc464', 'img':'cpc/cpc464.jpg', 'note':'' },
    { 'type':'emu',  'title':'Amstrad CPC6128', 'system':'cpc',     'url':'cpc.html', 'img':'cpc/cpc6128.jpg', 'note':'' },
    { 'type':'emu',  'title':'ZX Spectrum 48k', 'system':'zx',      'url':'zx.html?type=zx48k', 'img':'zx/zx48k.jpg', 'note':'' },
    { 'type':'emu',  'title':'ZX Spectrum 128', 'system':'zx',      'url':'zx.html?type=zx128', 'img':'zx/zx128.jpg', 'note':'' },
    { 'type':'emu',  'title':'Commodore C64',   'system':'c64',     'url':'c64.html',     'img':'c64/c64.jpg',       'note':'' },
    { 'type':'emu',  'title':'Acorn Atom',      'system':'atom',    'url':'atom.html',    'img':'atom/atom.jpg', 'note':'' },
    { 'type':'emu',  'title':'Robotron Z1013',  'system':'z1013',   'url':'z1013.html',   'img':'z1013/z1013.jpg', 'note':'J 300[Enter] => BASIC' },
    { 'type':'emu',  'title':'Robotron Z9001',  'system':'z9001',   'url':'z9001.html',   'img':'z9001/z9001.jpg', 'note':'(with BASIC and RAM modules)' },
    { 'type':'emu',  'title':'Robotron KC87',   'system':'z9001',   'url':'z9001.html?type=kc87', 'img':'z9001/kc87.jpg',     'note':'BASIC[Enter]' },
    { 'type':'demo', 'title':'FORTH (KC85/4)',  'system':'kc85',    'url':'kc85.html?type=kc85_4&mod=m026&mod_image=kc85/forth.853&input=SWITCH%208%20C1%0AFORTH%0A', 'img':'kc85/forth.jpg', 'note':'' },
    { 'type':'demo', 'title':'FORTH (Z1013)',   'system':'z1013',   'url':'z1013.html?type=z1013_64&file=z1013/z1013_forth.z80', 'img':'z1013/z1013_forth.jpg', 'note':''},
    { 'type':'demo', 'title':'ASMDEV (KC85/4)', 'system':'kc85',    'url':'kc85.html?type=kc85_4&mod=m027&mod_image=kc85/develop.853&input=SWITCH%208%20C1%0AMENU%0A', 'img':'kc85/development.jpg', 'note':''},
    { 'type':'demo', 'title':'DTC (CPC)',        'system':'cpc',     'url':'cpc.html?disc=cpc/dtc_cpc.dsk&input=run%22-DTC%0A', 'img':'cpc/dtc.jpg', 'note':'by Arkos/Overlanders' },
    { 'type':'demo', 'title':'Tire Au Flan (CPC)',  'system':'cpc',     'url':'cpc.html?disc=cpc/tireauflan_cpc.dsk&input=run%22TAFAPLIB%0A', 'img':'cpc/tireauflan.jpg', 'note':'press SPACE!' },
    { 'type':'demo', 'title':'Wolfenstrad (CPC)',   'system':'cpc',     'url':'cpc.html?disc=cpc/wolfenstrad.dsk&input=run%22-WOLF%0A', 'img':'cpc/wolfenstrad.jpg', 'note':'by Dirty Minds' },
    { 'type':'demo', 'title':"Byte'98 (CPC)",       'system':'cpc',     'url':"cpc.html?disc=cpc/byte98.dsk&input=run%22-BYTE'98%0A", 'img':'cpc/byte98.jpg', 'note':'by mortel/Overlanders' },
    { 'type':'demo', 'title':'Ecole Buissonniere',  'system':'cpc',     'url':'cpc.html?file=cpc/ecole_buissonniere.sna', 'img':'cpc/ecole_buissonniere.jpg', 'note':'by MadRam/OVL (CPC)'},
    { 'type':'demo', 'title':'Serious (KC85/4)',    'system':'kc85',    'url':'kc85.html?type=kc85_4&snapshot=kc85/serious.kcc', 'img':'kc85/serious.jpg', 'note':'by Moods Plateau'},
    { 'type':'game', 'title':'Digger (KC85/3)',     'system':'kc85',    'url':'kc85.html?type=kc85_3&mod=m022&snapshot=kc85/digger3.tap', 'img':'kc85/digger_3.jpg', 'note':'' },
    { 'type':'demo', 'title':'Jungle (KC85/3)',     'system':'kc85',    'url':'kc85.html?type=kc85_3&mod=m022&snapshot=kc85/jungle.kcc', 'img':'kc85/jungle.jpg', 'note':'' },
    { 'type':'demo', 'title':'Pengo (KC85/3)',      'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/pengo.kcc', 'img':'kc85/pengo.jpg', 'note':'' },
    { 'type':'demo', 'title':'House (KC85/3)',      'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/house.kcc', 'img':'kc85/house.jpg', 'note':'' },
    { 'type':'demo', 'title':'Cave (KC85/3)',       'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/cave.kcc', 'img':'kc85/cave.jpg', 'note':'' },
    { 'type':'demo', 'title':'Cave (KC85/3)',       'system':'kc85',    'url':'kc85.html?type=kc85_3&mod=m022&snapshot=kc85/cave2.853', 'img':'kc85/cave2.jpg', 'note':'better alt. version' },
    { 'type':'demo', 'title':'Labyrinth (KC85/3)',  'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/labyrinth.kcc', 'img':'kc85/labyrinth.jpg', 'note':'' },
    { 'type':'demo', 'title':'Pacman (KC85/3)',     'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/pacman.kcc&input=PACMAN%0A', 'img':'kc85/pacman.jpg', 'note':'' },
    { 'type':'demo', 'title':'Ladder (KC85/3)',     'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/ladder-3.kcc&input=LADDER%0A', 'img':'kc85/ladder.jpg', 'note':'' },
    { 'type':'demo', 'title':'Chess (KC85/4)',      'system':'kc85',    'url':'kc85.html?type=kc85_4&snapshot=kc85/chess.kcc&input=CHESS%0A', 'img':'kc85/chess.jpg', 'note':'' },
    { 'type':'demo', 'title':'Enterprise (KC85/4)', 'system':'kc85',    'url':'kc85.html?type=kc85_4&snapshot=kc85/enterpri.tap', 'img':'kc85/enterprise.jpg', 'note':'' },
    { 'type':'demo', 'title':'Mad Breakin (KC85/3)', 'system':'kc85',    'url':'kc85.html?type=kc85_3&mod=m022&snapshot=kc85/breakin.853', 'img':'kc85/breakin.jpg', 'note':'' },
    { 'type':'game', 'title':'Tetris (KC85/4)',     'system':'kc85',    'url':'kc85.html?type=kc85_4&snapshot=kc85/tetris.kcc', 'img':'kc85/tetris.jpg', 'note':'' },
    { 'type':'game', 'title':'Boulderdash (C64)',   'system':'c64',     'url':'c64.html?tape=c64/boulderdash_c64.tap&joystick=digital_1&tape_sound=yes', 'img':'c64/boulderdash_c64.jpg', 'note':'press F1 on start screen'},
    { 'type':'game', 'title':'Boulderdash (CPC)',   'system':'cpc',     'url':'cpc.html?file=cpc/boulder_dash.sna&joystick=true', 'img':'cpc/boulderdash_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Boulderdash (ZX48k)',    'system':'zx',   'url':'zx.html?file=zx/boulderdash_zx.z80&joystick=kempston&type=zx48k', 'img':'zx/boulderdash_zx.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Boulderdash (KC85/3)',   'system':'kc85', 'url':'kc85.html?type=kc85_3&mod=m022&snapshot=kc85/boulder3.tap', 'img':'kc85/boulderdash_3.jpg', 'note':''},
    { 'type':'game', 'title':'Boulderdash (KC85/4)',   'system':'kc85', 'url':'kc85.html?type=kc85_4&snapshot=kc85/bd2010.kcc', 'img':'kc85/bd2010.jpg', 'note':''},
    { 'type':'game', 'title':'Boulderdash (Z1013)',    'system':'z1013',    'url':'z1013.html?file=z1013/boulderdash_1_0.z80', 'img':'z1013/boulderdash_z1013.jpg', 'note':''},
    { 'type':'game', 'title':'Bomb Jack (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/bomb_jack.sna&joystick=true', 'img':'cpc/bombjack_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Bomb Jack (ZX48k)',   'system':'zx',      'url':'zx.html?file=zx/bombjack_zx.z80&joystick=kempston&type=zx48k', 'img':'zx/bombjack_zx.jpg', 'note':'select Kempston joystick!' },
    { 'type':'emu',  'title':'Bomb Jack (Arcade)',  'system':'bombjack','url':'bombjack.html', 'img':'bombjack/bombjack.jpg', 'note':'1 for coins, Enter to start'},
    { 'type':'game', 'title':'Zaxxon (C64)',        'system':'c64',     'url':'c64.html?tape=c64/zaxxon_c64.tap&joystick=digital_2&tape_sound=yes', 'img':'c64/zaxxon_c64.jpg', 'note':''},
    { 'type':'game', 'title':'Dig Dug (C64)',       'system':'c64',     'url':'c64.html?tape=c64/digdug_c64.tap&joystick=digital_1&tape_sound=yes', 'img':'c64/digdug_c64.jpg', 'note':''},
    { 'type':'game', 'title':'Cybernoid (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/cybernoid.sna&joystick=true', 'img':'cpc/cybernoid_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Arkanoid (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/arkanoid.sna&joystick=true', 'img':'cpc/arkanoid_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Arkanoid RoD (ZX128)',   'system':'zx',   'url':'zx.html?file=zx/arkanoid_zx128k.z80&joystick=sinclair1', 'img':'zx/arkanoid_zx128k.jpg', 'note':''},
    { 'type':'game', 'title':'Batty (ZX48K)',       'system':'zx',      'url':'zx.html?type=zx48k&file=zx/batty.z80&joystick=kempston', 'img':'zx/batty.jpg', 'note':''},
    { 'type':'demo', 'title':'Breakout (KC85/3)',      'system':'kc85',    'url':'kc85.html?type=kc85_3&snapshot=kc85/breakout.kcc&input=BREAKOUT%0A', 'img':'kc85/breakout.jpg', 'note':'' },
    { 'type':'game', 'title':'Silkworm (ZX128)',       'system':'zx',      'url':'zx.html?file=zx/silkworm_zx128k.z80&joystick=kempston', 'img':'zx/silkworm_zx128k.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Great Escape (ZX128)',   'system':'zx',     'url':'zx.html?file=zx/the_great_escape.z80&joystick=kempston', 'img':'zx/the_great_escape.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Flying Shark (ZX48K)',   'system':'zx',      'url':'zx.html?type=zx48k&file=zx/flying_shark.z80&joystick=kempston', 'img':'zx/flying_shark.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Chase HQ (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/chase_hq.sna&joystick=true', 'img':'cpc/chasehq_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Chase HQ (ZX128)',    'system':'zx',      'url':'zx.html?file=zx/chase_hq.z80&joystick=kempston', 'img':'zx/chase_hq.jpg', 'note':''},
    { 'type':'game', 'title':'Crazy Cars (CPC)',     'system':'cpc',    'url':'cpc.html?type=cpc464&tape=cpc/crazycar.tap&joystick=true', 'img':'cpc/crazycar.jpg', 'note':'long loading time'},
    { 'type':'game', 'title':"Overlander (CPC)",      'system':'cpc',   'url':'cpc.html?type=cpc464&tape=cpc/overland.tap&joystick=true', 'img':'cpc/overland.jpg', 'note':'long load time'},
    { 'type':'game', 'title':"Ghosts'n'Goblins (CPC)", 'system':'cpc',  'url':'cpc.html?file=cpc/ghosts_n_goblins.sna&joystick=true', 'img':'cpc/ghostsngoblins_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Fruity Frank (CPC)",  'system':'cpc',     'url':'cpc.html?file=cpc/fruity_frank.sna&joystick=true', 'img':'cpc/fruityfrank_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"1943 (CPC)",          'system':'cpc',     'url':'cpc.html?file=cpc/1943.sna&joystick=true', 'img':'cpc/1943_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Dragon Ninja (CPC)",  'system':'cpc',     'url':'cpc.html?file=cpc/dragon_ninja.sna&joystick=true', 'img':'cpc/dragonninja_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Gryzor (CPC)",        'system':'cpc',     'url':'cpc.html?file=cpc/gryzor.sna&joystick=true', 'img':'cpc/gryzor_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Astro Marine Corps (CPC)", 'system':'cpc','url':'cpc.html?file=cpc/astro_marine_corps.sna&joystick=true', 'img':'cpc/astro_marine_corps.jpg', 'note':''},
    { 'type':'game', 'title':"Bruce Lee (CPC)",      'system':'cpc',    'url':'cpc.html?file=cpc/bruce_lee.sna&joystick=true', 'img':'cpc/bruce_lee.jpg', 'note':''},
    { 'type':'game', 'title':"Bruce Lee (ZX48K)",    'system':'zx',     'url':'zx.html?type=zx48k&file=zx/bruce_lee.z80&joystick=kempston', 'img':'zx/bruce_lee.jpg', 'note':''},
    { 'type':'game', 'title':"Inter.Karate A (CPC)", 'system':'cpc',    'url':'cpc.html?type=cpc464&tape=cpc/interna4.tap&joystick=true', 'img':'cpc/interna4.jpg', 'note':'long loading time'},
    { 'type':'game', 'title':"Inter.Karate B (CPC)", 'system':'cpc',    'url':'cpc.html?type=cpc464&tape=cpc/interna5.tap&joystick=true', 'img':'cpc/interna5.jpg', 'note':'long loading time'},
    { 'type':'game', 'title':"Ikari Warriors (CPC)", 'system':'cpc',    'url':'cpc.html?file=cpc/ikari_warriors.sna&joystick=true', 'img':'cpc/ikariwarriors_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Commando (CPC)",       'system':'cpc',    'url':'cpc.html?type=cpc464&tape=cpc/commando.tap&joystick=true', 'img':'cpc/commando.jpg', 'note':'long loading time'},
    { 'type':'game', 'title':"Head over Heels (CPC)", 'system':'cpc',   'url':'cpc.html?file=cpc/head_over_heels.sna&joystick=true', 'img':'cpc/headoverheels_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Head over Heels (ZX)', 'system':'zx',  'url':'zx.html?file=zx/head_over_heels.z80&joystick=sinclair2', 'img':'zx/head_over_heels.jpg', 'note':''},
    { 'type':'game', 'title':"Rick Dangerous (CPC)",  'system':'cpc',   'url':'cpc.html?file=cpc/rick_dangerous.sna&joystick=true', 'img':'cpc/rickdangerous_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Blagger (CPC)",         'system':'cpc',   'url':'cpc.html?type=cpc464&tape=cpc/blagger.tap&joystick=true', 'img':'cpc/blagger.jpg', 'note':''},
    { 'type':'game', 'title':"Alien (CPC)",           'system':'cpc',   'url':'cpc.html?type=cpc464&tape=cpc/alien.tap&joystick=true', 'img':'cpc/alien.jpg', 'note':''},
    { 'type':'game', 'title':"Live and Let Die (CPC)",'system':'cpc',   'url':'cpc.html?type=cpc464&file=cpc/live_and_let_die.sna&joystick=true', 'img':'cpc/live_and_let_die.jpg', 'note':''},
    { 'type':'game', 'title':"Cyclone (ZX48k)",    'system':'zx', 'url':'zx.html?file=zx/cyclone.z80&joystick=kempston&type=zx48k', 'img':'zx/cyclone.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Exolon (ZX48k)",     'system':'zx', 'url':'zx.html?file=zx/exolon.z80&joystick=kempston&type=zx48k', 'img':'zx/exolon.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Quazatron (ZX128)",  'system':'zx', 'url':'zx.html?file=zx/quazatron.z80&joystick=kempston', 'img':'zx/quazatron.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Rainbow Islands (ZX)",  'system':'zx', 'url':'zx.html?file=zx/rainbow_islands.z80&joystick=kempston', 'img':'zx/rainbow_islands.jpg', 'note':'' },
    { 'type':'game', 'title':"Sir Fred (ZX48K)",  'system':'zx', 'url':'zx.html?type=zx48k&file=zx/sir_fred.z80&joystick=kempston', 'img':'zx/sir_fred.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Chucky Egg (Atom)",  'system':'atom', 'url':'atom.html?tape=atom/cchuck.tap&joystick=mmc', 'img':'atom/cchuck.jpg', 'note':'press SPACE to start!' },
    { 'type':'game', 'title':"Jet Set Willy (Atom)", 'system':'atom', 'url':'atom.html?tape=atom/jsw.tap&joystick=mmc', 'img':'atom/jsw.jpg', 'note':'select JOYMMC!'},
    { 'type':'game', 'title':"Mazogs (Z1013)",     'system':'z1013', 'url':'z1013.html?file=z1013/mazog_deutsch.z80', 'img':'z1013/mazogs_z1013.jpg', 'note':'' },
    { 'type':'game', 'title':"Galactica (Z1013)",  'system':'z1013', 'url':'z1013.html?file=z1013/galactica.z80', 'img':'z1013/galactica_z1013.jpg', 'note':'' },
    { 'type':'game', 'title':"Demolation (Z1013)", 'system':'z1013', 'url':'z1013.html?file=z1013/demolation.z80', 'img':'z1013/demolation_z1013.jpg', 'note':'' },
]
GitHubSamplesURL = 'https://github.com/floooh/chips-test/master/examples'
BuildConfig = 'wasm-ninja-release'

#-------------------------------------------------------------------------------
def deploy_webpage(fips_dir, proj_dir, webpage_dir) :
    """builds the final webpage under under fips-deploy/chips-webpage"""
    ws_dir = util.get_workspace_dir(fips_dir)
    emsc_deploy_dir = '{}/fips-deploy/chips-test/{}'.format(ws_dir, BuildConfig)

    # build the thumbnail gallery
    content = ''
    for item in items :
        title = item['title']
        system = item['system']
        url = item['url']
        ui_url = url.replace(".html", "-ui.html")
        image = item['img']
        note = item['note']
        log.info('> adding thumbnail for {}'.format(url))
        content += '<div class="thumb">\n'
        content += '  <div class="thumb-title">{}</div>\n'.format(title)
        if os.path.exists(emsc_deploy_dir + '/' + system + '-ui.js'):
            content += '<div class="img-btn"><a class="img-btn-link" href="{}">UI</a></div>'.format(ui_url)
        content += '  <div class="img-frame"><a href="{}"><img class="image" src="{}"></img></a></div>\n'.format(url,image)
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
    for name in ['emsc.js', 'favicon.png', 'help.html'] :
        log.info('> copy file: {}'.format(name))
        shutil.copy(proj_dir + '/webpage/' + name, webpage_dir + '/' + name)

    # generate emu HTML pages
    for system in systems :
        log.info('> generate emscripten HTML page: {}'.format(system))
        for ext in ['wasm', 'js'] :
            src_path = '{}/{}.{}'.format(emsc_deploy_dir, system, ext)
            if os.path.isfile(src_path) :
                shutil.copy(src_path, '{}/'.format(webpage_dir))
        with open(proj_dir + '/webpage/emsc.html', 'r') as f :
            templ = Template(f.read())
        src_url = GitHubSamplesURL + name
        html = templ.safe_substitute(name=system, prog=system, source=src_url)
        with open('{}/{}.html'.format(webpage_dir, system), 'w') as f :
            f.write(html)

    # copy data files and images
    for system in systems:
        src_dir = proj_dir + '/webpage/' + system
        if os.path.exists(src_dir):
            dst_dir = webpage_dir + '/' + system
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

