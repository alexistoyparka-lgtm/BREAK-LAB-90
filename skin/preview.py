#!/usr/bin/env python3
"""Offline skin preview with explicit parameter values (not a hardware capture).

Unlike OpenMPC's generic preview, this draws configured values and the matching
bank waveform. It is a layout check, not evidence of on-device rendering.
Requires Pillow, already used by the skin build; no device/runtime dependency.
"""
import argparse
import json
from pathlib import Path
import re
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]


def box(component):
    return tuple(int(float(v)) for v in component['bounds']['bounds'].split())


def build_previews(skin, output, bank=0, pad_layout=0):
    params=json.loads((ROOT/'vst/params.json').read_text())['params']
    bykey={p['key']:i for i,p in enumerate(params)}
    values={i:p.get('default',0) for i,p in enumerate(params)}
    values[bykey['break_select']]=bank
    values[bykey['pad_layout']]=pad_layout
    names=params[bykey['break_select']]['options']
    texts={bykey['status']:(names[bank] if bank else 'break01.wav')+' READY',bykey['midi_note']:'--'}
    data=json.loads((skin/'TUI.json').read_text())['pageData']
    defs={d['key']:d['value'] for d in data['componentDefinitions']['localComponentDefinitions']}
    output.mkdir(parents=True,exist_ok=True)
    fontdir=ROOT.parent/'mpc-vst-plugins/tools/html_art/fonts'
    def index(component,handle='Data'):
        target=next((m['value'] for m in component.get('handle remapping',{}).get('map',[]) if m['key']==handle),'')
        match=re.fullmatch(r'Parameter (\d+)',target)
        return int(match[1]) if match else None
    def normalized(i):
        if i is None:return 0
        p=params[i];v=values[i]
        if p.get('options'):return v/max(1,len(p['options'])-1)
        return (v-p.get('min',0))/max(.000001,p.get('max',1)-p.get('min',0))
    def shown(c):
        for handle in c['bounds'].get('additionalInvalidatingHandles',[]):
            m=re.fullmatch(r'IndexedEnabling/(\d+)/(\d+)/Parameter (\d+)',handle)
            if m and values.get(int(m[3]),0)!=int(m[1]):return False
        return True
    def value_text(i):
        if i in texts:return texts[i]
        p=params[i];v=values[i]
        if p.get('options'):return p['options'][int(v)]
        return f'{v:g}'
    for tab in data['tabs']:
        image=Image.new('RGB',(1280,628))
        draw=ImageDraw.Draw(image)
        for component in defs[tab['componentName']]['componentsData']:
            if not shown(component):continue
            cd=component['componentData']; x,y,w,h=box(component)
            if cd['type']=='Image':
                image.paste(Image.open(skin/cd['data']['image']).convert('RGB'),(x,y));continue
            for sub in defs[cd['type']]['componentsData']:
                sd=sub['componentData']; sx,sy,sw,sh=box(sub)
                i=index(component,sd['data'].get('handleName','Data'))
                if sd['type']=='Knob':
                    strip=Image.open(skin/sd['data']['filmStrip']).convert('RGBA')
                    frames=sd['data']['numFrames']
                    count=frames+1 if strip.height==(frames+1)*strip.width else frames
                    fh=strip.height//count; k=round(normalized(i)*(count-1))
                    frame=strip.crop((0,k*fh,strip.width,(k+1)*fh))
                    image.paste(frame,(x+sx,y+sy),frame)
                elif sd['type']=='Button':
                    b=sd['data'];selected=values.get(i,0)==b.get('buttonId',1)
                    path=b['onImage'] if selected else b['offImage']
                    if path:
                        picture=Image.open(skin/path).convert('RGBA');image.paste(picture,(x+sx,y+sy),picture)
                elif sd['type']=='Label':
                    p=params[i] if i is not None else {}
                    text=p.get('name',cd.get('name','')) if sd['data'].get('type')=='Name' else value_text(i)
                    style=sd['data']['textStyle'];height=style['font']['height']
                    # MPC native font heights are ~1.52 times the CSS text size.
                    font=ImageFont.truetype(str(fontdir/'TitilliumWeb-SemiBold.ttf'),round(height/1.52))
                    color=style['colour'];rgb=tuple(int(color[j:j+2],16) for j in (2,4,6))
                    bounds=draw.textbbox((0,0),text,font=font);tw=bounds[2]-bounds[0];th=bounds[3]-bounds[1]
                    justification=style.get('justification','')
                    tx=x+sx+(sw-tw)//2
                    if 'left' in justification.lower():tx=x+sx
                    elif 'right' in justification.lower():tx=x+sx+sw-tw
                    draw.text((tx,y+sy+(sh-th)//2-bounds[1]),text,fill=rgb,font=font)
        path=output/(tab['tabName'].lower().replace(' ','_')+'.png')
        image.save(path)
        print(path)


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('skin',type=Path)
    ap.add_argument('output',type=Path)
    ap.add_argument('--bank',type=int,choices=range(11),default=0)
    ap.add_argument('--pad-layout',type=int,choices=range(5),default=0)
    a=ap.parse_args()
    build_previews(a.skin,a.output,a.bank,a.pad_layout)
