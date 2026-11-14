#!/usr/bin/env python3
"""Regenerate the retro chassis and an honest waveform from the bundled PCM WAV.

The waveform is a static full-file overview, never a live playhead or editable
slice display. Native widgets are layered above these backgrounds by layout.conf.
"""
from pathlib import Path
import array
import math
import json
import sys
import wave

ROOT = Path(__file__).resolve().parent
FONT = {
 'A':['01110','10001','10001','11111','10001','10001','10001'],
 'B':['11110','10001','10001','11110','10001','10001','11110'],
 'E':['11111','10000','10000','11110','10000','10000','11111'],
 'K':['10001','10010','10100','11000','10100','10010','10001'],
 'L':['10000','10000','10000','10000','10000','10000','11111'],
 'R':['11110','10001','10001','11110','10100','10010','10001'],
 '0':['01110','10001','10011','10101','11001','10001','01110'],
 '9':['01110','10001','10001','01111','00001','00001','01110'],
 ' ':['00000']*7,
}

def pixel_title(text, x, y, scale=4):
    rects=[]
    for c in text:
        for row,bits in enumerate(FONT[c]):
            for col,bit in enumerate(bits):
                if bit=='1':
                    rects.append(f'<rect x="{x+col*scale}" y="{y+row*scale}" width="{scale}" height="{scale}"/>')
        x+=6*scale
    return '<g fill="#e8e8e4" shape-rendering="crispEdges">'+''.join(rects)+'</g>'

def text(x,y,s,size=12,fill='#9a9b99',extra=''):
    return f'<text x="{x}" y="{y}" fill="{fill}" font-family="DejaVu Sans Mono, monospace" font-size="{size}" {extra}>{s}</text>'

def panel(x,y,w,h,fill='#212223',line='#606260'):
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" stroke="{line}"/>'
            f'<path d="M{x+1} {y+h-1}V{y+1}H{x+w-1}" fill="none" stroke="#8b8d89" stroke-opacity=".65"/>'
            f'<path d="M{x+1} {y+h-1}H{x+w-1}V{y+1}" fill="none" stroke="#080909" stroke-width="2"/>')

def waveform(path):
    with wave.open(str(path),'rb') as f:
        rate=f.getframerate(); channels=f.getnchannels(); width=f.getsampwidth()
        if width != 2: raise SystemExit('waveform source must be PCM16')
        samples=array.array('h',f.readframes(f.getnframes()))
        if sys.byteorder!='little': samples.byteswap()
    frames=len(samples)//channels
    # A min/max envelope preserves brief percussive peaks in this reduced overview.
    cols=1180; upper=[]; lower=[]
    peak=max(abs(s) for s in samples) or 1
    for i in range(cols):
        a=i*frames//cols; b=max(a+1,(i+1)*frames//cols)
        chunk=samples[a*channels:b*channels]
        upper.append((40+i,221-min(chunk or [0])/peak*57))
        lower.append((40+i,221-max(chunk or [0])/peak*57))
    points=' '.join(f'{x},{y:.2f}' for x,y in upper+list(reversed(lower)))
    return f'<polygon points="{points}" fill="#ff343e"/>',frames/rate

def header(page,title,detail):
    s=['<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="628" viewBox="0 0 1280 628">',
       '<defs><pattern id="scan" width="4" height="4" patternUnits="userSpaceOnUse"><path d="M0 3.5H4" stroke="#fff" stroke-opacity=".015"/></pattern></defs>',
       '<rect width="1280" height="628" fill="#151617"/>',
       '<rect width="1280" height="628" fill="url(#scan)"/>',
       panel(10,10,1260,608,'#242526','#525451'),
       panel(24,24,1232,60,'#1b1c1d','#7c7f7a'),
       '<rect x="24" y="24" width="6" height="60" fill="#ff343e"/>',
       pixel_title('BREAK LAB 90',44,34,4),
       text(44,75,'AMIGA INSPIRED  /  BREAKBEAT SAMPLER',10,'#a0a29d', 'letter-spacing="1"'),
       text(470,49,'CUT THE BREAK.',16,'#e8e8e4','font-weight="bold" letter-spacing="2"'),
       text(470,70,'KEEP THE CHARACTER.',12,'#969995','letter-spacing="2"'),
       '<path d="M866 34V74" stroke="#555853"/>',
       text(890,49,'16 SLICES   /   08 VOICES',12,'#d2d4cd'),
       text(890,70,'STANDALONE RHYTHM MACHINE',10,'#92958f','letter-spacing="1"'),
       panel(24,96,1232,48,'#111314','#4c504c'),
       text(40,115,'SAMPLE MEMORY',10,'#8a8e87','letter-spacing="1"'),
       text(1226,116,f'{page:02d} / {title}',12,'#dedfd8','text-anchor="end" font-weight="bold"'),
       '<rect x="24" y="604" width="1232" height="2" fill="#74766f"/>',
       '<rect x="24" y="606" width="1232" height="2" fill="#090a0a"/>',
       text(38,624,'OPENBENCH',10,'#a2a59b','letter-spacing="2"'),
       text(1240,624,detail,10,'#8c9087','text-anchor="end" letter-spacing="1"')]
    # Recessed fasteners are quiet enough to avoid competing with controls.
    for x,y in [(16,16),(1264,16),(16,612),(1264,612)]:
        s += [f'<circle cx="{x}" cy="{y}" r="3" fill="#111211" stroke="#71756e"/>', f'<path d="M{x-1.5} {y-1.5}L{x+1.5} {y+1.5}" stroke="#777b72"/>']
    return s

def wave_panel(s,shape,seconds):
    s += [panel(24,156,1232,154,'#030505','#81847e'),
          '<path d="M36 221H1244" stroke="#40443e" stroke-width=".6"/>',
          shape,
          text(40,174,'FULL SAMPLE / STATIC OVERVIEW',9,'#b2b6ad','letter-spacing="1"'),
          text(1236,174,f'{seconds:.2f} SEC' if seconds else '',9,'#b2b6ad','text-anchor="end"'),
          '<rect x="25" y="293" width="1230" height="16" fill="#3a0a10"/>',
          '<rect x="25" y="293" width="1230" height="3" fill="#ff343e"/>',
          text(40,305,'00',8,'#e39d9f'),text(1237,305,'END',8,'#e39d9f','text-anchor="end"')]

def pads(s):
    s += [panel(1042,330,214,260,'#1b1d1b','#50564d'),text(1060,353,'PAD ORDER',11,'#d0d5ca','letter-spacing="1"')]
    for r in range(4):
        for c in range(4):
            n=(3-r)*4+c+1; x=1058+c*47; y=366+r*46
            s += [panel(x,y,40,38,'#30332e','#7f8779'),text(x+20,y+24,f'{n:02d}',13,'#e8ece3','text-anchor="middle" font-weight="bold"')]
    s += [text(1060,573,'01 = LOWER LEFT',9,'#a4ad9b'),'<path d="M1058 543H1098" stroke="#ff343e" stroke-width="3"/>']

def end(s,path):
    s += ['</svg>']; (ROOT/path).write_text('\n'.join(s)+'\n')

shape,seconds=waveform(ROOT.parent/'engine/breaks/break01.wav')
s=header(1,'PLAY','ONE SHOT / VELOCITY / EIGHT VOICES')
wave_panel(s,'',0)
s += [panel(24,330,478,260,'#202220','#555b51'),text(40,353,'PERFORMANCE',11,'#d1d6cb','letter-spacing="1"'),
      panel(514,330,516,260,'#202220','#555b51'),text(532,353,'PLAYBACK',11,'#d1d6cb','letter-spacing="1"'),
      text(48,581,'SELECT A SLICE. STRIKE A PAD. MAKE IT YOURS.',9,'#929b88'),
      text(534,379,'DIRECTION',9,'#9da792'),text(534,449,'CHARACTER',9,'#9da792'),text(534,519,'INTERPOLATION',9,'#9da792')]
pads(s);end(s,'bg.svg')

s=header(2,'SLICE','START / END / ONE STEP UNDO')
wave_panel(s,'',0)
s += [panel(24,330,1006,260,'#202220','#555b51'),text(40,353,'SLICE EDITOR',11,'#d1d6cb','letter-spacing="1"'),
      text(40,578,'EDIT BOUNDARIES INSIDE THE SELECTED REGION.',9,'#929b88')]
pads(s);end(s,'slice.svg')

s=header(3,'CHARACTER','CLEAN / VINTAGE / PAULA INSPIRED')
wave_panel(s,'',0)
s += [panel(24,330,1006,260,'#202220','#555b51'),text(40,353,'SIGNAL CHARACTER',11,'#d1d6cb','letter-spacing="1"'),
      text(1048,357,'PAULA',19,'#e7ece1','font-weight="bold" letter-spacing="2"'),
      text(1048,377,'INSPIRED',11,'#ff5960','letter-spacing="2"'),
      text(1048,412,'8 BIT.',12,'#c5ccbc'),text(1048,434,'NEW IDEAS.',12,'#c5ccbc'),
      text(1048,482,'HOLD / LINEAR',10,'#909a85'),text(1048,500,'/ CUBIC',10,'#909a85'),
      text(1048,552,'BREAK IT.',14,'#e7ece1','font-weight="bold"')]
end(s,'character.svg')

# Bank-indexed overviews. There is no animation and no fabricated slice marker.
bank=json.loads((ROOT.parent/'engine/breaks/bank.json').read_text())
entries=[{'index':0,'file':'break01.wav','name':'Original Break'}]+bank['selection']
for item in entries:
    shape,seconds=waveform(ROOT.parent/'engine/breaks'/item['file'])
    s=['<svg xmlns="http://www.w3.org/2000/svg" width="1232" height="154" viewBox="24 156 1232 154">']
    wave_panel(s,shape,seconds)
    s += ['</svg>']
    (ROOT/'assets'/f"wave_{item['index']:02d}.svg").write_text('\n'.join(s)+'\n')

s=header(4,'PADS','PAD 01 / LOWER LEFT / CHRONOLOGICAL SLICES')
s += [panel(24,156,484,434,'#202220','#555b51'),text(40,181,'PHYSICAL PAD ORDER',12,'#d1d6cb','letter-spacing="1"')]
for r in range(4):
    for c in range(4):
        n=(3-r)*4+c+1;x=48+c*110;y=200+r*82
        s += [panel(x,y,96,68,'#30332e','#7f8779'),text(x+48,y+42,f'{n:02d}',23,'#e8ece3','text-anchor="middle" font-weight="bold"')]
s += ['<path d="M48 514H144" stroke="#ff343e" stroke-width="4"/>',text(48,560,'START HERE. THEN LEFT TO RIGHT.',10,'#b1bba4'),
      panel(520,156,736,434,'#202220','#555b51'),text(540,181,'PAD SETUP',12,'#d1d6cb','letter-spacing="1"'),
      text(540,209,'LAYOUT',10,'#aab49b','letter-spacing="1"'),
      text(540,481,'CHROMATIC: LEARN PAD 01, THEN PAD 05.',10,'#aab49b')]
end(s,'pads.svg')
