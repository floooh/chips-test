"""fips verb to build the samples webpage"""

import os
import yaml
import shutil
import subprocess
import glob
from string import Template

from mod import log, util, project

BuildConfig = 'wasm-ninja-release'

systems = [
    'kc852', 'kc852-ui',
    'kc853', 'kc853-ui',
    'kc854', 'kc854-ui',
    'zx', 'zx-ui',
    'c64', 'c64-ui',
    'vic20', 'vic20-ui',
    'cpc', 'cpc-ui',
    'atom', 'atom-ui',
    'z1013', 'z1013-ui',
    'z9001', 'z9001-ui',
    'bombjack', 'bombjack-ui',
    'pacman', 'pacman-ui',
    'pengo', 'pengo-ui',
    'lc80',
    'ext'
]

asset_dirs = [ 'kc85', 'zx', 'c64', 'vic20', 'cpc', 'atom', 'z1013', 'z9001', 'bombjack', 'pengo', 'lc80', 'ext' ]

items = [
    { 'type':'ext',  'title':'Visual 6502 Remix', 'system':'none',  'url':'https://floooh.github.io/visual6502remix', 'img':'ext/visual6502remix.jpg', 'note':''},
    { 'type':'ext',  'title':'Visual Z80 Remix', 'system':'none',   'url':'https://floooh.github.io/visualz80remix', 'img':'ext/visualz80remix.jpg', 'note':''},
    { 'type':'ext',  'title':'Visual 2A03 Remix', 'system':'none',  'url':'https://floooh.github.io/visual2a03remix', 'img':'ext/visual2a03remix.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC85/2', 'system':'kc852',            'url':'kc852.html', 'img':'kc85/kc85_2.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC85/3', 'system':'kc853',            'url':'kc853.html', 'img':'kc85/kc85_3.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC85/4', 'system':'kc854',            'url':'kc854.html', 'img':'kc85/kc85_4.jpg', 'note':'' },
    { 'type':'emu',  'title':'KC Compact',      'system':'cpc',     'url':'cpc.html?type=kccompact', 'img':'cpc/kccompact.jpg', 'note':'' },
    { 'type':'emu',  'title':'Amstrad CPC464',  'system':'cpc',     'url':'cpc.html?type=cpc464', 'img':'cpc/cpc464.jpg', 'note':'' },
    { 'type':'emu',  'title':'Amstrad CPC6128', 'system':'cpc',     'url':'cpc.html', 'img':'cpc/cpc6128.jpg', 'note':'' },
    { 'type':'emu',  'title':'ZX Spectrum 48k', 'system':'zx',      'url':'zx.html?type=zx48k', 'img':'zx/zx48k.jpg', 'note':'' },
    { 'type':'emu',  'title':'ZX Spectrum 128', 'system':'zx',      'url':'zx.html?type=zx128', 'img':'zx/zx128.jpg', 'note':'' },
    { 'type':'emu',  'title':'Commodore VIC-20','system':'vic20',   'url':'vic20.html',   'img':'vic20/vic20.jpg',    'note':'' },
    { 'type':'emu',  'title':'Commodore C64',   'system':'c64',     'url':'c64.html',     'img':'c64/c64.jpg',       'note':'' },
    { 'type':'emu',  'title':'Acorn Atom',      'system':'atom',    'url':'atom.html',    'img':'atom/atom.jpg', 'note':'' },
    { 'type':'emu',  'title':'LC-80',           'system':'lc80',    'url':'lc80.html',    'img':'lc80/lc80.jpg', 'note':'' },
    { 'type':'emu',  'title':'Robotron Z1013',  'system':'z1013',   'url':'z1013.html',   'img':'z1013/z1013.jpg', 'note':'' },
    { 'type':'emu',  'title':'Robotron Z9001',  'system':'z9001',   'url':'z9001.html',   'img':'z9001/z9001.jpg', 'note':'(with BASIC and RAM modules)' },
    { 'type':'emu',  'title':'Robotron KC87',   'system':'z9001',   'url':'z9001.html?type=kc87', 'img':'z9001/kc87.jpg',     'note':'BASIC[Enter]' },
    { 'type':'demo', 'title':'FORTH (KC85/4)',  'system':'kc854',   'url':'kc854.html?mod=m026&mod_image=kc85/forth.853&input=SWITCH%208%20C1%0AFORTH%0A', 'img':'kc85/forth.jpg', 'note':'' },
    { 'type':'demo', 'title':'FORTH (Z1013)',   'system':'z1013',   'url':'z1013.html?type=z1013_64&file=z1013/z1013_forth.z80', 'img':'z1013/z1013_forth.jpg', 'note':''},
    { 'type':'demo', 'title':'BASIC (Z1013)',   'system':'z1013',   'url':'z1013.html?type=z1013_64&file=z1013/kc_basic.z80', 'img':'z1013/z1013_basic.jpg', 'note':''},
    { 'type':'demo', 'title':'ASMDEV (KC85/4)', 'system':'kc854',   'url':'kc854.html?mod=m027&mod_image=kc85/develop.853&input=SWITCH%208%20C1%0AMENU%0A', 'img':'kc85/development.jpg', 'note':''},
    { 'type':'demo', 'title':'CP/M 2.2 (CPC)',  'system':'cpc',     'url':'cpc.html?file=cpc/6128sp_4.dsk&input=%7Ccpm%0A', 'img':'cpc/cpm_cpc.jpg', 'note':''},
    { 'type':'demo', 'title':'DTC (CPC)',        'system':'cpc',     'url':'cpc.html?file=cpc/dtc_cpc.dsk&input=run%22-DTC%0A', 'img':'cpc/dtc.jpg', 'note':'by Arkos/Overlanders' },
    { 'type':'demo', 'title':'Tire Au Flan (CPC)',  'system':'cpc',     'url':'cpc.html?file=cpc/tireauflan_cpc.dsk&input=run%22TAFAPLIB%0A', 'img':'cpc/tireauflan.jpg', 'note':'press SPACE!' },
    { 'type':'demo', 'title':'Wolfenstrad (CPC)',   'system':'cpc',     'url':'cpc.html?file=cpc/wolfenstrad.dsk&input=run%22-WOLF%0A', 'img':'cpc/wolfenstrad.jpg', 'note':'by Dirty Minds' },
    { 'type':'demo', 'title':"Byte'98 (CPC)",       'system':'cpc',     'url':"cpc.html?file=cpc/byte98.dsk&input=run%22-BYTE'98%0A", 'img':'cpc/byte98.jpg', 'note':'by mortel/Overlanders' },
    { 'type':'demo', 'title':'Ecole Buissonniere',  'system':'cpc',     'url':'cpc.html?file=cpc/ecole_buissonniere.dsk&input=run%22ecole%0A', 'img':'cpc/ecole_buissonniere.jpg', 'note':'by MadRam/OVL (CPC)'},
    { 'type':'demo', 'title':'Demoizart (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/demoizart_cpc.dsk&input=run%22DIA%0A', 'img':'cpc/demoizart.jpg', 'note':'strobe warning!' },
    { 'type':'demo', 'title':"Logon's Run (CPC)",   'system':'cpc',     'url':'cpc.html?file=cpc/logons_run_cpc.dsk&input=run%22logon%27s%0A', 'img':'cpc/logons_run.jpg', 'note':'by Overflow/Logon System' },
    { 'type':'demo', 'title':"YAP! (CPC)",          'system':'cpc',     'url':'cpc.html?file=cpc/yet_another_plasma.dsk&input=run%22-yap%21%0A', 'img':'cpc/yet_another_plasma.jpg', 'note':'by Logon System' },
    { 'type':'demo', 'title':"Backtro (CPC)",       'system':'cpc',     'url':'cpc.html?file=cpc/backtro.dsk&input=run%22-backtro%0A', 'img':'cpc/backtro.jpg', 'note':'by Overflow/Logon System' },
    { 'type':'demo', 'title':'Isometrikum (CPC)',   'system':'cpc',     'url':'cpc.html?file=cpc/isometrikum.dsk&input=run%22isometri.kum%0A', 'img':'cpc/isometrikum.jpg', 'note':'by Vanity'},
    { 'type':'demo', 'title':'SotB Demo (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/ovf-prev.dsk&input=run%22-PREVIEW%0A', 'img':'cpc/sotb_cpc.jpg', 'note':'"Preview 1", then SPACE!'},
    { 'type':'demo', 'title':'Points Barres (CPC)', 'system':'cpc',     'url':'cpc.html?file=cpc/points_barres.dsk&input=run%22-PBARRES%0A', 'img':'cpc/points_barres.jpg', 'note':'press SPACE!'},
    { 'type':'demo', 'title':'Still Rising (CPC)',  'system':'cpc',     'url':'cpc.html?file=cpc/stillrising.dsk&input=run%22-RISING%0A', 'img':'cpc/stillrising.jpg', 'note':'by Vanity'},
    { 'type':'demo', 'title':'Octopus Pocus (CPC)', 'system':'cpc',     'url':'cpc.html?file=cpc/octopuspocus.dsk&input=run%22OCTOPUS%0A', 'img':'cpc/octopuspocus.jpg', 'note':'by Pulpo Corrosivo'},
    { 'type':'demo', 'title':'Gloire a Piou! (CPC)','system':'cpc',     'url':'cpc.html?file=cpc/piou.dsk&input=run%22gap%0A', 'img':'cpc/piou.jpg', 'note':'by Overlanders'},
    { 'type':'demo', 'title':'phX / Condense (CPC)','system':'cpc',     'url':'cpc.html?file=cpc/phx.dsk&input=run%22phx%0A', 'img':'cpc/phx.jpg', 'note':'by Condense' },
    { 'type':'demo', 'title':'Batman Forever (CPC)','system':'cpc',     'url':'cpc.html?file=cpc/batman_forever.dsk&input=run%22disc%0A', 'img':'cpc/batman_forever.jpg', 'note': 'by Batman Group' },
    { 'type':'demo', 'title':'CRTC (CPC)',          'system':'cpc',     'url':'cpc.html?file=cpc/crtc.dsk&input=run%22crtc%0A', 'img':'cpc/crtc.jpg', 'note':'by benediction & Arkos'},
    { 'type':'demo', 'title':'Wunderbar (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/wunderbar.dsk&input=run%22wunder.bar%0A', 'img':'cpc/wunderbar.jpg', 'note':'by Arkos/Benediction'},
    { 'type':'demo', 'title':'Phortem (CPC)',       'system':'cpc',     'url':'cpc.html?file=cpc/phortem.dsk&input=run%22phortem%0A', 'img':'cpc/phortem.jpg', 'note':'by Condense'},
    { 'type':'demo', 'title':'Pheelone (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/pheelone_2009condense.dsk&input=run%22pheelone%0A', 'img':'cpc/pheelone.jpg', 'note':'by Condense' },
    { 'type':'demo', 'title':'Phreaks (CPC)',       'system':'cpc',     'url':'cpc.html?file=cpc/phreaks_2010condense.dsk&input=run%22phreaks%0A', 'img':'cpc/phreaks.jpg', 'note':'by Condense' },
    { 'type':'demo', 'title':'Phat 2 (CPC)',        'system':'cpc',     'url':'cpc.html?file=cpc/phat_2.dsk&input=run%22phat2%0A', 'img':'cpc/phat_2.jpg', 'note':'by Condense'},
    { 'type':'demo', 'title':'Wobbler (CPC)',       'system':'cpc',     'url':'cpc.html?file=cpc/wobbler.dsk&input=run%22-wobbler%0A', 'img':'cpc/wobbler.jpg', 'note':'by TAF' },
    { 'type':'demo', 'title':'Battro (CPC)',        'system':'cpc',     'url':'cpc.html?file=cpc/battro.dsk&input=run%22battro%0A', 'img':'cpc/battro.jpg', 'note':'by Batman Group'},
    { 'type':'demo', 'title':'Stand Up! (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/standup_party_version.dsk&input=run%22standup%0A', 'img':'cpc/standup_party_version.jpg', 'note':'by Benediction'},
    { 'type':'demo', 'title':'Onescreen Colonies #3 (CPC)','system':'cpc','url':'cpc.html?file=cpc/osc3.dsk&input=run%22-osc3%0A', 'img':'cpc/osc3.jpg', 'note':'by Vanity'},
    { 'type':'demo', 'title':'Sphere (CPC)',        'system':'cpc',     'url':'cpc.html?file=cpc/sphere.dsk&input=run%22sphere%0A', 'img':'cpc/sphere.jpg', 'note':'by Resistance'},
    { 'type':'demo', 'title':'Foursome (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/foursome.dsk&input=run%22foursome%0A', 'img':'cpc/foursome.jpg', 'note':'by Pulpo Corrosivo'},
    { 'type':'demo', 'title':'Fractoid (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/fractoid.dsk&input=run%22fractoid%0A', 'img':'cpc/fractoid.jpg', 'note':'by Osmium Path'},
    { 'type':'demo', 'title':'Debris (CPC)',        'system':'cpc',     'url':'cpc.html?file=cpc/debris.dsk&input=run%22debris%0A', 'img':'cpc/debris.jpg', 'note':'by Pulpo Corrosivo'},
    { 'type':'demo', 'title':'Ghost NOP (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/ghostnop.dsk&input=run%22ghostnop%0A', 'img':'cpc/ghostnop.jpg', 'note':'by Pulpo Corrosivo'},
    { 'type':'demo', 'title':'Rebels 1989 (C64)',   'system':'c64',     'url':'c64.html?file=c64/rebels1989_c64.prg', 'img':'c64/rebels1989_c64.jpg', 'note':'by Rebels'},
    { 'type':'demo', 'title':'$777 (C64)',          'system':'c64',     'url':'c64.html?file=c64/s777_c64.prg', 'img':'c64/s777_c64.jpg', 'note':'by Razor 1911' },
    { 'type':'demo', 'title':'The Wobbler (C64)',   'system':'c64',     'url':'c64.html?file=c64/wobbler_c64.prg', 'img':'c64/wobbler_c64.jpg', 'note':'by Abyss Connection' },
    { 'type':'demo', 'title':'Dawnfall (C64)',      'system':'c64',     'url':'c64.html?file=c64/dawnfall_c64.prg', 'img':'c64/dawnfall_c64.jpg', 'note':'by Oxyron'},
    { 'type':'demo', 'title':'+H4K (C64)',          'system':'c64',     'url':'c64.html?file=c64/h4k_c64.prg', 'img':'c64/h4k_c64.jpg', 'note':'by Plush' },
    { 'type':'demo', 'title':'Crapman (C64)',       'system':'c64',     'url':'c64.html?file=c64/crapman_c64.prg', 'img':'c64/crapman_c64.jpg', 'note':'by Latex' },
    { 'type':'demo', 'title':'Kongo2 (C64)',        'system':'c64',     'url':'c64.html?file=c64/kongo2_c64.prg', 'img':'c64/kongo2_c64.jpg', 'note': 'by Cruel Soldiers' },
    { 'type':'demo', 'title':'C.Intentional (C64)', 'system':'c64',     'url':'c64.html?file=c64/cintentional_c64.prg', 'img':'c64/cintentional_c64.jpg', 'note':'by Vision' },
    { 'type':'demo', 'title':'Sour & Salty (C64)',  'system':'c64',     'url':'c64.html?file=c64/soursalty_c64.prg', 'img':'c64/soursalty_c64.jpg', 'note':'by Artline Designs' },
    { 'type':'demo', 'title':'Nightcrawler (C64)',  'system':'c64',     'url':'c64.html?file=c64/nightcrawler_c64.prg', 'img':'c64/nightcrawler_c64.jpg', 'note':'by Almagest' },
    { 'type':'demo', 'title':'Arcade Intro (C64)',  'system':'c64',     'url':'c64.html?file=c64/arcade02_c64.prg', 'img':'c64/arcade02_c64.jpg', 'note':'by Arcade' },
    { 'type':'demo', 'title':'In A Loop 1K (C64)',  'system':'c64',     'url':'c64.html?file=c64/inaloop_c64.prg', 'img':'c64/inaloop_c64.jpg', 'note':'by Ate Bit'},
    { 'type':'demo', 'title':'Dubious 4K (C64)',    'system':'c64',     'url':'c64.html?file=c64/dubious_c64.prg', 'img':'c64/dubious_c64.jpg', 'note':'by Fairlight'},
    { 'type':'demo', 'title':'Space Invaders (C64)','system':'c64',     'url':'c64.html?file=c64/space_invaders_demo.prg', 'img':'c64/space_invaders_demo.jpg', 'note':'by Hokuto Force'},
    { 'type':'demo', 'title':'Rewind (C64)',        'system':'c64',     'url':'c64.html?file=c64/rewind_tempest.prg', 'img':'c64/rewind_tempest.jpg', 'note':'by TempesT'},
    { 'type':'demo', 'title':'Tunnel Vision (C64)', 'system':'c64',     'url':'c64.html?file=c64/tunnelvision_omg.prg', 'img':'c64/tunnelvision_omg.jpg', 'note':'by OMG'},
    { 'type':'demo', 'title':'Party Horse (C64)',   'system':'c64',     'url':'c64.html?file=c64/party_horse.prg', 'img':'c64/party_horse.jpg', 'note':'by Booze Design'},
    { 'type':'demo', 'title':'Funny Rasters (C64)', 'system':'c64',     'url':'c64.html?file=c64/funnyrasters.prg', 'img':'c64/funnyrasters.jpg', 'note':'by Excess'},
    { 'type':'demo', 'title':'Field Sort (C64)',    'system':'c64',     'url':'c64.html?file=c64/lft-field-sort.prg', 'img':'c64/lft-field-sort.jpg', 'note':'by Lft'},
    { 'type':'demo', 'title':'One Bit Wonder (C64)','system':'c64',     'url':'c64.html?file=c64/onebitwonder.prg', 'img':'c64/onebitwonder.jpg', 'note':'by Offence et al.'},
    { 'type':'demo', 'title':'Swirl (C64)',         'system':'c64',     'url':'c64.html?file=c64/swirl_by_dnp.prg', 'img':'c64/swirl_by_dnp.jpg', 'note':'by DnP'},
    { 'type':'demo', 'title':'The Earth (C64)',     'system':'c64',     'url':'c64.html?file=c64/the_earth_party_ver.prg', 'img':'c64/the_earth_party_ver.jpg', 'note':'by eins'},
    { 'type':'demo', 'title':'F600 (C64)',          'system':'c64',     'url':'c64.html?file=c64/f600.prg', 'img':'c64/f600.jpg', 'note':'by Hokuto Force & PVM'},
    { 'type':'demo', 'title':'Reflections (C64)',   'system':'c64',     'url':'c64.html?file=c64/reflections.prg', 'img':'c64/reflections.jpg', 'note':'by Plush'},
    { 'type':'demo', 'title':'Summer of 64 (C64)',  'system':'c64',     'url':'c64.html?file=c64/summerof64.prg', 'img':'c64/summerof64.jpg', 'note':'by Offence'},
    { 'type':'demo', 'title':'One-Der (C64)',       'system':'c64',     'url':'c64.html?file=c64/oneder_oxyron.prg', 'img':'c64/oneder_oxyron.jpg', 'note':'by Oxyron'},
    { 'type':'demo', 'title':'Quadrants (C64)',     'system':'c64',     'url':'c64.html?file=c64/gp_quadrants.prg', 'img':'c64/gp_quadrants.jpg', 'note':'by Genesis Project'},
    { 'type':'demo', 'title':'Copper Booze (C64)',  'system':'c64',     'url':'c64.html?file=c64/copperbooze.prg', 'img':'c64/copperbooze.jpg', 'note':'by Booze Design'},
    { 'type':'demo', 'title':'Square Booze (C64)',  'system':'c64',     'url':'c64.html?file=c64/squarebooze.prg', 'img':'c64/squarebooze.jpg', 'note':'by Booze Design'},
    { 'type':'demo', 'title':'For Your Sprites Only (C64)', 'system':'c64', 'url':'c64.html?file=c64/foryourspritesonly.prg', 'img':'c64/foryourspritesonly.jpg', 'note':'by Booze Design'},
    { 'type':'demo', 'title':'Listen to your Eyes (C64)',   'system':'c64', 'url':'c64.html?file=c64/listentoyoureyes.prg', 'img':'c64/listentoyoureyes.jpg', 'note':'by Booze Design'},
    { 'type':'demo', 'title':'Selbuvotter Latitudes (C64)', 'system':'c64', 'url':'c64.html?file=c64/selbuvotter_latitudes.prg', 'img':'c64/selbuvotter_latitudes.jpg', 'note':'by Fossil'},
    { 'type':'demo', 'title':'Raster Fun (C64)',    'system':'c64',     'url':'c64.html?file=c64/rasterfun.prg', 'img':'c64/rasterfun.jpg', 'note':'by Bonzai'},
    { 'type':'demo', 'title':'Onscreen 5K (C64)',   'system':'c64',     'url':'c64.html?file=c64/onscreen_5k.prg', 'img':'c64/onscreen_5k.jpg', 'note':'by Xenon'},
    { 'type':'demo', 'title':'Doctor Dither (C64)', 'system':'c64',     'url':'c64.html?file=c64/doctor_dither.prg', 'img':'c64/doctor_dither.jpg', 'note':'by Miami Fun Project'},
    { 'type':'demo', 'title':'White Lines (C64)',   'system':'c64',     'url':'c64.html?file=c64/whitelines.prg', 'img':'c64/whitelines.jpg', 'note':'by Plush'},
    { 'type':'demo', 'title':'Tribute to Ben (C64)','system':'c64',     'url':'c64.html?file=c64/lastninja.prg', 'img':'c64/lastninja.jpg', 'note':'by Jason Page'},
    { 'type':'demo', 'title':'The Disks Have Eyes (C64)','system':'c64','url':'c64.html?file=c64/thediskshaveeyes.prg', 'img':'c64/thediskshaveeyes.jpg', 'note':'by Onslaught'},
    { 'type':'demo', 'title':'Elite Code Mechanics (C64)','system':'c64','url':'c64.html?file=c64/elitecodemechanics.prg', 'img':'c64/elitecodemechanics.jpg', 'note':'by Abyss Connection'},
    { 'type':'demo', 'title':'vECMors (C64)',       'system':'c64',     'url':'c64.html?file=c64/vecmors.prg', 'img':'c64/vecmors.jpg', 'note':'by Finnish Gold'},
    { 'type':'demo', 'title':'Layers (C64)',        'system':'c64',     'url':'c64.html?file=c64/layers.prg', 'img':'c64/layers.jpg', 'note':'by Finnish Gold'},
    { 'type':'demo', 'title':'Plasma Scroll (C64)', 'system':'c64',     'url':'c64.html?file=c64/plasmascroll.prg', 'img':'c64/plasmascroll.jpg', 'note':'by Finnish Gold'},
    { 'type':'demo', 'title':'O.M.I.Y.P. (C64)',    'system':'c64',     'url':'c64.html?file=c64/oldmeninyellowpools.prg', 'img':'c64/oldmeninyellowpools.jpg', 'note':'by Fossil'},
    { 'type':'demo', 'title':'Cubic Dream (C64)',   'system':'c64',     'url':'c64.html?file=c64/cubicdream.prg', 'img':'c64/cubicdream.jpg', 'note':'by Lazycow'},
    { 'type':'demo', 'title':'2 Shades of Gray (C64)',  'system':'c64',     'url':'c64.html?file=c64/twoshadesofgray.prg', 'img':'c64/twoshadesofgray.jpg', 'note':'by Gordian'},
    { 'type':'demo', 'title':'PPY (VIC20)',         'system':'vic20',   'url':'vic20.html?file=vic20/proxima_ppy.prg&exp=maxram', 'img':'vic20/proxima_ppy.jpg', 'note':'by Proxima'},
    { 'type':'demo', 'title':'Muna.Paluu (VIC20)',  'system':'vic20',   'url':'vic20.html?file=vic20/munakois.prg&exp=ram8k', 'img':'vic20/munakois.jpg', 'note':'by Dekadence'},
    { 'type':'demo', 'title':'SotB Intro (KC85/4)', 'system':'kc854',   'url':'kc854.html?file=kc85/sotb_intro.kcc', 'img':'kc85/sotb_intro.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'SotB Game (KC85/4)',  'system':'kc854',   'url':'kc854.html?file=kc85/sotb_game.kcc', 'img':'kc85/sotb_game.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'Quantum Leap (KC85/4)','system':'kc854',  'url':'kc854.html?file=kc85/quantumleap.kcc', 'img':'kc85/quantumleap.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'Serious (KC85/4)',    'system':'kc854',   'url':'kc854.html?file=kc85/serious.kcc', 'img':'kc85/serious.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'Twenty (KC85/4)',     'system':'kc854',   'url':'kc854.html?file=kc85/twenty.kcc', 'img':'kc85/twenty.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'Ancient Civilizations (KC85/4)','system':'kc854',  'url':'kc854.html?file=kc85/ancient.kcc', 'img':'kc85/ancient.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'Stereo (KC85/4)',     'system':'kc854',   'url':'kc854.html?file=kc85/stereo.kcc', 'img':'kc85/stereo.jpg', 'note':'by Moods Plateau'},
    { 'type':'demo', 'title':'Fractal Dimension (KC85/4)','system':'kc854',  'url':'kc854.html?file=kc85/anaglyph.kcc', 'img':'kc85/anaglyph.jpg', 'note':'by Moods Plateau' },
    { 'type':'demo', 'title':'Sampled Audio (KC85/4)', 'system':'kc854','url':'kc854.html?file=kc85/sreplay.kcc&input=PLAY%0A', 'img':'kc85/sreplay.jpg', 'note':'by Doc.K/Moods Plateau'},
    { 'type':'game', 'title':'Digger (KC85/3)',     'system':'kc853',   'url':'kc853.html?mod=m022&file=kc85/digger3.tap', 'img':'kc85/digger_3.jpg', 'note':'' },
    { 'type':'demo', 'title':'Jungle (KC85/3)',     'system':'kc853',   'url':'kc853.html?mod=m022&file=kc85/jungle.kcc', 'img':'kc85/jungle.jpg', 'note':'' },
    { 'type':'demo', 'title':'Pengo (KC85/3)',      'system':'kc853',   'url':'kc853.html?file=kc85/pengo.kcc', 'img':'kc85/pengo.jpg', 'note':'' },
    { 'type':'demo', 'title':'House (KC85/3)',      'system':'kc853',   'url':'kc853.html?file=kc85/house.kcc', 'img':'kc85/house.jpg', 'note':'' },
    { 'type':'demo', 'title':'Cave (KC85/3)',       'system':'kc853',   'url':'kc853.html?file=kc85/cave.kcc', 'img':'kc85/cave.jpg', 'note':'' },
    { 'type':'demo', 'title':'Cave (KC85/3)',       'system':'kc853',   'url':'kc853.html?mod=m022&file=kc85/cave2.853', 'img':'kc85/cave2.jpg', 'note':'better alt. version' },
    { 'type':'demo', 'title':'Labyrinth (KC85/3)',  'system':'kc853',   'url':'kc853.html?file=kc85/labyrinth.kcc', 'img':'kc85/labyrinth.jpg', 'note':'' },
    { 'type':'demo', 'title':'Pacman (KC85/3)',     'system':'kc853',   'url':'kc853.html?file=kc85/pacman.kcc&input=PACMAN%0A', 'img':'kc85/pacman.jpg', 'note':'' },
    { 'type':'demo', 'title':'Ladder (KC85/3)',     'system':'kc853',   'url':'kc853.html?file=kc85/ladder-3.kcc&input=LADDER%0A', 'img':'kc85/ladder.jpg', 'note':'' },
    { 'type':'demo', 'title':'Chess (KC85/4)',      'system':'kc854',   'url':'kc854.html?file=kc85/chess.kcc&input=CHESS%0A', 'img':'kc85/chess.jpg', 'note':'' },
    { 'type':'demo', 'title':'Enterprise (KC85/4)', 'system':'kc854',   'url':'kc854.html?file=kc85/enterpri.tap', 'img':'kc85/enterprise.jpg', 'note':'' },
    { 'type':'demo', 'title':'Mad Breakin (KC85/3)', 'system':'kc853',  'url':'kc853.html?mod=m022&file=kc85/breakin.853', 'img':'kc85/breakin.jpg', 'note':'' },
    { 'type':'game', 'title':'Tetris (KC85/4)',     'system':'kc854',   'url':'kc854.html?file=kc85/tetris.kcc', 'img':'kc85/tetris.jpg', 'note':'' },
    { 'type':'game', 'title':'Sokoban (KC85/4)',    'system':'kc854',   'url':'kc854.html?file=kc85/sokoban4.kcc', 'img':'kc85/sokoban4.jpg', 'note':'' },
    { 'type':'game', 'title':'Boulderdash (C64)',   'system':'c64',     'url':'c64.html?file=c64/boulderdash_c64.prg&joystick=digital_1', 'img':'c64/boulderdash_c64.jpg', 'note':'press F1 on start screen'},
    { 'type':'game', 'title':'Boulderdash (CPC)',   'system':'cpc',     'url':'cpc.html?file=cpc/boulder_dash.sna&joystick=true', 'img':'cpc/boulderdash_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Boulderdash (ZX48k)',  'system':'zx',     'url':'zx.html?file=zx/boulderdash_zx.z80&joystick=kempston&type=zx48k', 'img':'zx/boulderdash_zx.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Boulderdash (KC85/3)', 'system':'kc853',  'url':'kc853.html?mod=m022&file=kc85/boulder3.tap', 'img':'kc85/boulderdash_3.jpg', 'note':''},
    { 'type':'game', 'title':'Boulderdash (KC85/4)', 'system':'kc854',  'url':'kc854.html?file=kc85/bd2010.kcc', 'img':'kc85/bd2010.jpg', 'note':''},
    { 'type':'game', 'title':'Boulderdash (Z1013)',  'system':'z1013',  'url':'z1013.html?file=z1013/boulderdash_1_0.z80', 'img':'z1013/boulderdash_z1013.jpg', 'note':''},
    { 'type':'game', 'title':'Bomb Jack (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/bomb_jack.sna&joystick=true', 'img':'cpc/bombjack_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Bomb Jack (ZX48k)',   'system':'zx',      'url':'zx.html?file=zx/bombjack_zx.z80&joystick=kempston&type=zx48k', 'img':'zx/bombjack_zx.jpg', 'note':'select Kempston joystick!' },
    { 'type':'emu',  'title':'Bomb Jack (Arcade)',  'system':'bombjack','url':'bombjack.html', 'img':'bombjack/bombjack.jpg', 'note':'1 for coins, Enter to start'},
    { 'type':'game', 'title':"Pengo (Arcade)",      'system':'pengo',   'url':'pengo.html', 'img':'pengo/pengo.jpg', 'note':'1 for coins, Enter to start' },
    { 'type':'game', 'title':'Zaxxon (C64)',        'system':'c64',     'url':'c64.html?file=c64/zaxxon_c64.prg&joystick=digital_2', 'img':'c64/zaxxon_c64.jpg', 'note':''},
    { 'type':'game', 'title':'Dig Dug (C64)',       'system':'c64',     'url':'c64.html?file=c64/digdug_c64.prg&joystick=digital_1', 'img':'c64/digdug_c64.jpg', 'note':''},
    { 'type':'game', 'title':'Cybernoid (CPC)',     'system':'cpc',     'url':'cpc.html?file=cpc/cybernoid.sna&joystick=true', 'img':'cpc/cybernoid_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Cybernoid (C64)',     'system':'c64',     'url':'c64.html?file=c64/cybernoid_c64.prg&joystick=digital_12', 'img':'c64/cybernoid_c64.jpg', 'note':''},
    { 'type':'game', 'title':'Arkanoid (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/arkanoid.sna&joystick=true', 'img':'cpc/arkanoid_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Arkanoid RoD (CPC)',  'system':'cpc',     'url':'cpc.html?file=cpc/arkanoid2_cpc.dsk&joystick=true&input=run%22arkanoid%0A', 'img':'cpc/arkanoid2_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Arkanoid RoD (ZX128)',   'system':'zx',   'url':'zx.html?file=zx/arkanoid_zx128k.z80&joystick=sinclair1', 'img':'zx/arkanoid_zx128k.jpg', 'note':''},
    { 'type':'game', 'title':'Batty (ZX48K)',       'system':'zx',      'url':'zx.html?type=zx48k&file=zx/batty.z80&joystick=kempston', 'img':'zx/batty.jpg', 'note':''},
    { 'type':'demo', 'title':'Breakout (KC85/3)',      'system':'kc853','url':'kc853.html?file=kc85/breakout.kcc&input=BREAKOUT%0A', 'img':'kc85/breakout.jpg', 'note':'' },
    { 'type':'game', 'title':'Silkworm (ZX128)',       'system':'zx',   'url':'zx.html?file=zx/silkworm_zx128k.z80&joystick=kempston', 'img':'zx/silkworm_zx128k.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Silkworm (CPC)',         'system':'cpc',  'url':'cpc.html?file=cpc/silkworm_cpc.dsk&joystick=true&input=run%22disc%0A', 'img':'cpc/silkworm_cpc.jpg', 'note':'' },
    { 'type':'game', 'title':'Great Escape (ZX128)',   'system':'zx',   'url':'zx.html?file=zx/the_great_escape.z80&joystick=kempston', 'img':'zx/the_great_escape.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Flying Shark (ZX48K)',   'system':'zx',   'url':'zx.html?type=zx48k&file=zx/flying_shark.z80&joystick=kempston', 'img':'zx/flying_shark.jpg', 'note':'select Kempston joystick!'},
    { 'type':'game', 'title':'Chase HQ (CPC)',      'system':'cpc',     'url':'cpc.html?file=cpc/chase_hq.sna&joystick=true', 'img':'cpc/chasehq_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Chase HQ (ZX128)',    'system':'zx',      'url':'zx.html?file=zx/chase_hq.z80&joystick=kempston', 'img':'zx/chase_hq.jpg', 'note':''},
# FIXME TAP: { 'type':'game', 'title':'Crazy Cars (CPC)',     'system':'cpc',    'url':'cpc.html?type=cpc464&file=cpc/crazycar.tap&joystick=true&input=run%22%0A%0A', 'img':'cpc/crazycar.jpg', 'note':'long loading time'},
# FIXME TAP: { 'type':'game', 'title':"Overlander (CPC)",      'system':'cpc',   'url':'cpc.html?type=cpc464&file=cpc/overland.tap&joystick=true&input=run%22%0A%0A', 'img':'cpc/overland.jpg', 'note':'long load time'},
    { 'type':'game', 'title':"Ghosts'n'Goblins (CPC)", 'system':'cpc',  'url':'cpc.html?file=cpc/ghosts_n_goblins.sna&joystick=true', 'img':'cpc/ghostsngoblins_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Ghosts'n'Goblins (C64)", 'system':'c64',  'url':'c64.html?file=c64/ghostsngoblins_c64.prg&joystick=digital_12', 'img':'c64/ghostsngoblins_c64.jpg', 'note':''},
    { 'type':'game', 'title':"Fruity Frank (CPC)",  'system':'cpc',     'url':'cpc.html?file=cpc/fruity_frank.sna&joystick=true', 'img':'cpc/fruityfrank_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"1943 (CPC)",          'system':'cpc',     'url':'cpc.html?file=cpc/1943.sna&joystick=true', 'img':'cpc/1943_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Dragon Ninja (CPC)",  'system':'cpc',     'url':'cpc.html?file=cpc/dragon_ninja.sna&joystick=true', 'img':'cpc/dragonninja_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Gryzor (CPC)",        'system':'cpc',     'url':'cpc.html?file=cpc/gryzor.sna&joystick=true', 'img':'cpc/gryzor_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Astro Marine Corps (CPC)", 'system':'cpc','url':'cpc.html?file=cpc/astro_marine_corps.sna&joystick=true', 'img':'cpc/astro_marine_corps.jpg', 'note':''},
    { 'type':'game', 'title':"Bruce Lee (CPC)",      'system':'cpc',    'url':'cpc.html?file=cpc/bruce_lee.sna&joystick=true', 'img':'cpc/bruce_lee.jpg', 'note':''},
    { 'type':'game', 'title':"Bruce Lee (ZX48K)",    'system':'zx',     'url':'zx.html?type=zx48k&file=zx/bruce_lee.z80&joystick=kempston', 'img':'zx/bruce_lee.jpg', 'note':''},
    { 'type':'game', 'title':"Intern.Karate (CPC)", 'system':'cpc',     'url':'cpc.html?file=cpc/intern_karate_cpc.dsk&joystick=true&input=run%22karate%0A', 'img':'cpc/intern_karate_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"IK+(CPC)",            'system':'cpc',     'url':'cpc.html?file=cpc/intern_karate_plus_cpc.dsk&joystick=true&input=run%22IK%2B%0A', 'img':'cpc/intern_karate_plus_cpc.jpg', 'note':''},
# FIXME broken    { 'type':'game', 'title':"IK+ (C64)",           'system':'c64',     'url':'c64.html?file=c64/ikplus_c64.prg&joystick=digital_2', 'img':'c64/ikplus_c64.jpg', 'note':'' },
    { 'type':'game', 'title':"Ikari Warriors (CPC)", 'system':'cpc',    'url':'cpc.html?file=cpc/ikari_warriors.sna&joystick=true', 'img':'cpc/ikariwarriors_cpc.jpg', 'note':''},
# FIXME TAP: { 'type':'game', 'title':"Commando (CPC)",       'system':'cpc',    'url':'cpc.html?type=cpc464&file=cpc/commando.tap&joystick=true&input=run%22%0A%0A', 'img':'cpc/commando.jpg', 'note':'long loading time'},
    { 'type':'game', 'title':"Head over Heels (CPC)", 'system':'cpc',   'url':'cpc.html?file=cpc/head_over_heels.sna&joystick=true', 'img':'cpc/headoverheels_cpc.jpg', 'note':''},
    { 'type':'game', 'title':'Head over Heels (ZX)', 'system':'zx',     'url':'zx.html?file=zx/head_over_heels.z80&joystick=sinclair2', 'img':'zx/head_over_heels.jpg', 'note':''},
    { 'type':'game', 'title':"Rick Dangerous (CPC)",  'system':'cpc',   'url':'cpc.html?file=cpc/rick_dangerous.sna&joystick=true', 'img':'cpc/rickdangerous_cpc.jpg', 'note':''},
    { 'type':'game', 'title':"Prince of Persia (CPC)", 'system':'cpc',  'url':'cpc.html?file=cpc/prince_of_persia_cpc.dsk&joystick=true&input=run%22prince%0A', 'img':'cpc/prince_of_persia_cpc.jpg', 'note':'' },
    { 'type':'game', 'title':"Nebulus (CPC)",         'system':'cpc',   'url':'cpc.html?file=cpc/nebulus_cpc.dsk&joystick=true&input=run%22nebulus%0A', 'img':'cpc/nebulus_cpc.jpg', 'note':'' },
    { 'type':'game', 'title':"Nebulus (C64)",         'system':'c64',   'url':'c64.html?file=c64/nebulus_c64.prg&joystick=digital_1', 'img':'c64/nebulus_c64.jpg', 'note':'' },
# FIXME TAP: { 'type':'game', 'title':"Blagger (CPC)",         'system':'cpc',   'url':'cpc.html?type=cpc464&file=cpc/blagger.tap&joystick=true&input=run%22%0A%0A', 'img':'cpc/blagger.jpg', 'note':''},
# FIXME TAP: { 'type':'game', 'title':"Alien (CPC)",           'system':'cpc',   'url':'cpc.html?type=cpc464&file=cpc/alien.tap&joystick=true&input=run%22%0A%0A', 'img':'cpc/alien.jpg', 'note':''},
    { 'type':'game', 'title':"Live and Let Die (CPC)",'system':'cpc',   'url':'cpc.html?type=cpc464&file=cpc/live_and_let_die.sna&joystick=true', 'img':'cpc/live_and_let_die.jpg', 'note':''},
    { 'type':'game', 'title':"Cyclone (ZX48k)",    'system':'zx', 'url':'zx.html?file=zx/cyclone.z80&joystick=kempston&type=zx48k', 'img':'zx/cyclone.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Exolon (ZX48k)",     'system':'zx', 'url':'zx.html?file=zx/exolon.z80&joystick=kempston&type=zx48k', 'img':'zx/exolon.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Quazatron (ZX128)",  'system':'zx', 'url':'zx.html?file=zx/quazatron.z80&joystick=kempston', 'img':'zx/quazatron.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Rainbow Islands (ZX)",  'system':'zx', 'url':'zx.html?file=zx/rainbow_islands.z80&joystick=kempston', 'img':'zx/rainbow_islands.jpg', 'note':'' },
    { 'type':'game', 'title':"Sir Fred (ZX48K)",  'system':'zx', 'url':'zx.html?type=zx48k&file=zx/sir_fred.z80&joystick=kempston', 'img':'zx/sir_fred.jpg', 'note':'select Kempston joystick!' },
    { 'type':'game', 'title':"Chucky Egg (Atom)",  'system':'atom', 'url':'atom.html?file=atom/cchuck.tap&joystick=mmc&input=%2ALOAD%0A', 'img':'atom/cchuck.jpg', 'note':'press SPACE to start!' },
    { 'type':'game', 'title':"Jet Set Willy (Atom)", 'system':'atom', 'url':'atom.html?file=atom/jsw.tap&joystick=mmc&input=%2ALOAD%0A', 'img':'atom/jsw.jpg', 'note':'select JOYMMC!'},
    { 'type':'game', 'title':"Alien Blitz (VIC-20)", 'system':'vic20', 'url':'vic20.html?file=vic20/alienblitz_vic20.prg&joystick=true', 'img':'vic20/alienblitz_vic20.jpg', 'note':''},
    { 'type':'game', 'title':"Mazogs (Z1013)",     'system':'z1013', 'url':'z1013.html?file=z1013/mazog_deutsch.z80', 'img':'z1013/mazogs_z1013.jpg', 'note':'' },
    { 'type':'game', 'title':"Galactica (Z1013)",  'system':'z1013', 'url':'z1013.html?file=z1013/galactica.z80', 'img':'z1013/galactica_z1013.jpg', 'note':'' },
    { 'type':'game', 'title':"Demolation (Z1013)", 'system':'z1013', 'url':'z1013.html?file=z1013/demolation.z80', 'img':'z1013/demolation_z1013.jpg', 'note':'' },
    { 'type':'test', 'title':"Pacman (Arcade)", 'system':'pacman', 'url':'pacman.html', 'img':'', 'note':'' },
]

#-------------------------------------------------------------------------------
def cwebp(src_path, dst_path):
    cmd_line = 'cwebp -quiet -q 80 {} -o {}'.format(src_path, dst_path)
    print('> {}'.format(cmd_line))
    subprocess.call(cmd_line, shell=True)

#-------------------------------------------------------------------------------
def to_webp_ext(path):
    return os.path.splitext(path)[0] + '.webp'

#-------------------------------------------------------------------------------
def deploy_webpage(fips_dir, proj_dir, webpage_dir) :
    emsc_deploy_dir = util.get_deploy_dir(fips_dir, 'chips-test', BuildConfig)

    # build the thumbnail gallery
    content = ''
    for item in items :
        if item['type'] == 'test':
            continue
        title = item['title']
        system = item['system']
        url = item['url']
        ui_url = url.replace(".html", "-ui.html")
        image = to_webp_ext(item['img'])
        note = item['note']
        log.info('> adding thumbnail for {}'.format(url))
        content += '<div class="thumb-frame">\n'
        content += '  <div class="thumb-title">{}</div>\n'.format(title)
        if os.path.exists(emsc_deploy_dir + '/' + system + '-ui.js'):
            content += '<a class="ui-btn-link" href="{}"><div class="ui-btn">UI</div></a>'.format(ui_url)
        content += '  <a class="image-frame" href="{}"><img class="image" src="{}"></img></a>\n'.format(url,image)
        if note != '':
            content += '  <div class="thumb-footer">{}</div>\n'.format(note)
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
    for name in ['favicon.png', 'help.html'] :
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
        html = templ.safe_substitute(name=system, prog=system)
        with open('{}/{}.html'.format(webpage_dir, system), 'w') as f :
            f.write(html)

    # copy data files and images
    for asset_dir in asset_dirs:
        src_dir = proj_dir + '/webpage/' + asset_dir
        dst_dir = webpage_dir + '/' + asset_dir
        if os.path.isdir(src_dir):
            if os.path.isdir(dst_dir):
                shutil.rmtree(dst_dir)
            os.makedirs(dst_dir)
            for filename in os.listdir(src_dir):
                src_file = src_dir + '/' + filename
                dst_file = dst_dir + '/' + filename
                if filename.endswith('.jpg'):
                    dst_file = to_webp_ext(dst_file)
                    cwebp(src_file, dst_file)
                else:
                    print('> copy {} => {}'.format(src_file, dst_file))
                    shutil.copyfile(src_file, dst_file)

#-------------------------------------------------------------------------------
def build_deploy_webpage(fips_dir, proj_dir, rebuild) :
    # if webpage dir exists, clear it first
    proj_build_dir = util.get_deploy_root_dir(fips_dir, 'chips-test')
    webpage_dir = '{}/chips-webpage'.format(proj_build_dir)
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
    proj_build_dir = util.get_deploy_root_dir(fips_dir, 'chips-test')
    webpage_dir = '{}/chips-webpage'.format(proj_build_dir)
    p = util.get_host_platform()
    if p == 'osx' :
        try :
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt :
            pass
    elif p == 'win':
        try:
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass
    elif p == 'linux':
        try:
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
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
