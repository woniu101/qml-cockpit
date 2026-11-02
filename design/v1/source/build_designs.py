"""Editable SVG artboards for static design review; independent of runtime QML."""
from pathlib import Path
from html import escape
import base64

ROOT = Path(__file__).resolve().parents[1]
CAR = 'data:image/png;base64,' + base64.b64encode((ROOT/'assets/vehicle-pearl.png').read_bytes()).decode()
C = {'ink':'#23332F','muted':'#6F7D73','soft':'#A7AEA7','green':'#557A65','active':'#E3EBE2','line':'#E2E6E0','bg':'#F1F3EE'}

def text(x,y,s,size=24,color=None,weight=400,anchor='start',extra=''):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color or C["ink"]}" font-weight="{weight}" text-anchor="{anchor}" {extra}>{escape(s)}</text>'

def rect(x,y,w,h,r=24,fill='#FFFFFF',stroke='none',extra=''):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{stroke}" {extra}/>'

def line(x1,y1,x2,y2,color=None,width=1.5,extra=''):
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color or C["line"]}" stroke-width="{width}" {extra}/>'

PATHS={
 'home':'<path d="m3 10 9-7 9 7v10H15v-7H9v7H3Z"/>',
 'nav':'<path d="m12 3 8 18-8-4-8 4Z"/>',
 'car':'<path d="m5 7 2-4h10l2 4 2 3v8H3v-8Z"/><path d="M5 7h14M3 12h18M6 17v3m12-3v3"/><path d="M6 10h2m8 0h2"/>',
 'music':'<path d="M9 18V5l11-2v13M9 8l11-2"/><ellipse cx="6" cy="18" rx="3" ry="2.5"/><ellipse cx="17" cy="16" rx="3" ry="2.5"/>',
 'search':'<circle cx="10.5" cy="10.5" r="6.5"/><path d="m16 16 5 5"/>',
 'lock':'<rect x="5" y="10" width="14" height="11" rx="3"/><path d="M8 10V7a4 4 0 0 1 8 0v3"/><path d="M12 14v3"/>',
 'trunk':'<path d="M4 12h16v7H4Z"/><path d="m6 12 2-4h8l2 4M4 7l4-4h9m-9 0v4"/><path d="M7 19v2m10-2v2"/>',
 'charge':'<path d="M7 3h10v18H7Z"/><path d="m13 6-3 6h4l-3 6M17 6h2l2 3v8a2 2 0 0 1-2 2h-2"/>',
 'fan':'<circle cx="12" cy="12" r="2"/><path d="M10 10C2 9 3 2 8 3c3 0 4 4 4 7m2 0c5-6 10-1 7 3-2 3-6 1-8 0m-2 1c2 8-5 9-7 5-2-3 2-5 6-6"/>',
 'defrost':'<path d="M3 18V8a27 27 0 0 1 18 0v10Z"/><path d="M7 15V8m5 7V7m5 8V8m-12 2 2-2 2 2m1-1 2-2 2 2m1 1 2-2 2 2"/>',
 'seat':'<path d="M8 4v9a3 3 0 0 0 3 3h8v4H9a5 5 0 0 1-5-5V8m6 12v2"/><path d="M15 3c-3 3 3 3 0 6m4-6c-3 3 3 3 0 6"/>',
 'volume':'<path d="M3 9h4l5-4v14l-5-4H3Z"/><path d="M16 8a6 6 0 0 1 0 8m3-11a10 10 0 0 1 0 14"/>',
 'apps':'<rect x="3" y="3" width="7" height="7" rx="2"/><rect x="14" y="3" width="7" height="7" rx="2"/><rect x="3" y="14" width="7" height="7" rx="2"/><rect x="14" y="14" width="7" height="7" rx="2"/>',
 'chevron-right':'<path d="m9 5 7 7-7 7"/>',
 'chevron-left':'<path d="m15 5-7 7 7 7"/>',
 'wifi':'<path d="M3 8a15 15 0 0 1 18 0M6 12a10 10 0 0 1 12 0m-9 4a5 5 0 0 1 6 0"/><circle cx="12" cy="20" r=".8" fill="currentColor"/>',
 'bluetooth':'<path d="m7 7 10 10-5 4V3l5 4L7 17"/>',
 'signal':'<path d="M4 20v-3m5 3v-7m5 7V9m5 11V4"/>',
 'bell':'<path d="M5 17h14l-2-4V8a5 5 0 0 0-10 0v5Zm5 3h4"/>',
 'sun':'<circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M2 12h2m16 0h2M5 5l1.5 1.5M17.5 17.5 19 19M5 19l1.5-1.5M17.5 6.5 19 5"/>',
 'moon':'<path d="M20 15A8 8 0 0 1 9 4a8.5 8.5 0 1 0 11 11Z"/>',
 'coffee':'<path d="M4 8h12v7a5 5 0 0 1-5 5H9a5 5 0 0 1-5-5Zm12 1h2a3 3 0 0 1 0 6h-2M7 2v3m4-3v3M2 22h18"/>',
 'tent':'<path d="m2 20 10-17 10 17ZM7 20l5-9 5 9M12 3V1"/>',
 'pin':'<path d="M19 10c0 5-7 11-7 11S5 15 5 10a7 7 0 1 1 14 0Z"/><circle cx="12" cy="10" r="2"/>',
 'briefcase':'<rect x="3" y="7" width="18" height="14" rx="3"/><path d="M8 7V3h8v4M3 12h18m-9-2v4"/>',
 'window':'<path d="M3 5h18v14H3ZM3 14h18M9 5v9"/>',
 'light':'<path d="M14 5a9 9 0 0 0 0 14ZM18 6l4-1m-4 6h4m-4 6 4 1M2 9h3m-3 6h3"/>',
 'mirror':'<path d="M5 4h9a6 6 0 0 1 0 12H5ZM5 16v4H2M9 8h5"/>',
 'refresh':'<path d="M20 8A8 8 0 1 0 21 15M20 3v5h-5"/>',
 'arrow-up':'<path d="M12 20V4m-6 6 6-6 6 6"/>',
 'arrow-down':'<path d="M12 4v16m-6-6 6 6 6-6"/>',
 'layers':'<path d="m12 3 10 6-10 6L2 9Zm-10 11 10 6 10-6m-20 5 10 6 10-6"/>',
 'locate':'<circle cx="12" cy="12" r="7"/><circle cx="12" cy="12" r="2"/><path d="M12 1v4m0 14v4M1 12h4m14 0h4"/>',
 'plus':'<path d="M12 4v16M4 12h16"/>',
 'minus':'<path d="M4 12h16"/>',
 'close':'<path d="m5 5 14 14M5 19 19 5"/>',
 'check':'<path d="m4 12 5 5L20 6"/>',
 'arrow-left-turn':'<path d="M18 21V11a4 4 0 0 0-4-4H4m5-5L4 7l5 5"/>',
 'user':'<circle cx="12" cy="8" r="4"/><path d="M4 22v-3a8 8 0 0 1 16 0v3"/>',
 'settings':'<path d="M4 6h16M4 12h16M4 18h16"/><circle cx="9" cy="6" r="2.5" fill="#fff"/><circle cx="15" cy="12" r="2.5" fill="#fff"/><circle cx="9" cy="18" r="2.5" fill="#fff"/>',
 'pause':'<path d="M8 5v14M16 5v14"/>',
 'skip':'<path d="m4 5 10 7-10 7ZM19 5v14"/>',
 'previous':'<path d="m20 5-10 7 10 7ZM5 5v14"/>',
 'play':'<path d="m7 4 13 8-13 8Z"/>',
 'door':'<path d="M5 21V4l12-2v19ZM17 8h3v13M12 12h1"/>',
 'snow':'<path d="M12 2v20M3.3 7l17.4 10M3.3 17 20.7 7M9 4l3 3 3-3m-6 16 3-3 3 3"/>'
}

def icon(name,x,y,size=28,color=None,stroke=1.7,extra=''):
    return f'<g transform="translate({x} {y}) scale({size/24})" fill="none" stroke="{color or C["ink"]}" stroke-width="{stroke}" stroke-linecap="round" stroke-linejoin="round" {extra}>{PATHS[name]}</g>'

def pill(x,y,w,label,active=False,name=None,h=44):
    s=rect(x,y,w,h,h/2,C['active'] if active else '#FFFFFF',extra='opacity=".96"')
    if name: s+=icon(name,x+15,y+(h-22)/2,22,C['green'] if active else C['ink'])
    s+=text(x+(43 if name else w/2),y+h/2+7,label,19,C['green'] if active else C['ink'],500,'start' if name else 'middle')
    return s

def base(title):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="1600" height="900" viewBox="0 0 1600 900">
<title>{escape(title)}</title>
<defs>
<linearGradient id="bg" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#F7F7F2"/><stop offset=".55" stop-color="#EEF1EB"/><stop offset="1" stop-color="#E2E8E0"/></linearGradient>
<radialGradient id="glow"><stop stop-color="#FFFFFF" stop-opacity=".8"/><stop offset="1" stop-color="#FFFFFF" stop-opacity="0"/></radialGradient>
<linearGradient id="album" x1="0" y1="1" x2="1" y2="0"><stop stop-color="#B79A70"/><stop offset=".6" stop-color="#D7C9AE"/><stop offset="1" stop-color="#F3EADB"/></linearGradient>
<linearGradient id="water" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#D4E4E2"/><stop offset="1" stop-color="#C2D8D7"/></linearGradient>
<filter id="shadow" x="-20%" y="-20%" width="140%" height="150%"><feDropShadow dx="0" dy="10" stdDeviation="18" flood-color="#596E55" flood-opacity=".07"/></filter>
<clipPath id="canvas"><rect width="1600" height="900"/></clipPath>
</defs>
<g font-family="Microsoft YaHei UI, Microsoft YaHei, sans-serif" letter-spacing="0" clip-path="url(#canvas)">
<rect width="1600" height="900" fill="url(#bg)"/>
'''

def status(gear='P'):
    s=text(40,40,'16:28',22,weight=500)+line(125,22,125,44,'#D5DDD3')
    s+=icon('sun',147,20,25,C['green'])+text(185,40,'24°  上海',20,C['muted'])
    s+=pill(316,15,74,'驻车' if gear=='P' else '行驶',True,h=34)
    s+=text(1170,39,'82%',19,C['muted'])
    s+=rect(1220,24,37,17,4,'none','#69786D')+rect(1223,27,26,11,2,'#6C8A70')+rect(1260,29,3,7,1,'#69786D')
    s+=icon('bluetooth',1290,20,24,C['muted'])+icon('wifi',1343,20,24,C['muted'])+icon('signal',1396,20,24,C['muted'])
    s+=icon('bell',1450,20,24,C['muted'])+rect(1512,13,42,42,21,'#D7E1D3')+text(1533,42,'林',20,C['green'],500,'middle')
    return s

def dock(selected):
    s=rect(0,790,1600,110,0,'#FAFBF8',extra='opacity=".98"')+line(0,790,1600,790,'#DDE3D9')
    for i,(name,label) in enumerate([('home','首页'),('nav','导航'),('car','车辆'),('music','音乐')]):
        x=30+i*83
        if label==selected:s+=rect(x,803,69,79,20,'#E2EADF')
        s+=icon(name,x+21,816,27,C['green'] if label==selected else '#69766C')+text(x+35,869,label,17,C['green'] if label==selected else '#69766C',500 if label==selected else 400,'middle')
    s+=line(383,818,383,872,'#DEE4D9')
    s+=text(489,822,'主驾',16,C['muted'],anchor='middle')+text(489,866,'22.0°',38,weight=400,anchor='middle')
    s+=icon('chevron-left',409,839,23,C['muted'])+icon('chevron-right',549,839,23,C['muted'])
    s+=icon('fan',624,821,30,C['green'])+text(640,871,'AUTO',15,C['green'],500,'middle')
    s+=icon('snow',718,821,30,C['green'])+text(733,871,'A/C',15,C['green'],500,'middle')
    s+=icon('defrost',812,821,30,'#69766C')+text(827,871,'除雾',15,C['muted'],anchor='middle')
    s+=icon('seat',905,821,30,'#69766C')+text(920,871,'座椅',15,C['muted'],anchor='middle')
    s+=line(993,818,993,872,'#DEE4D9')
    s+=text(1104,822,'副驾',16,C['muted'],anchor='middle')+text(1104,866,'22.0°',38,anchor='middle')
    s+=icon('chevron-left',1023,839,23,C['muted'])+icon('chevron-right',1166,839,23,C['muted'])
    s+=line(1225,818,1225,872,'#DEE4D9')
    s+=icon('volume',1275,832,29,'#69766C')+line(1322,847,1431,847,'#D4DDD1',5,extra='stroke-linecap="round"')+line(1322,847,1374,847,C['green'],5,extra='stroke-linecap="round"')
    s+='<circle cx="1374" cy="847" r="7" fill="#557A65"/>'
    s+=icon('apps',1495,831,32,'#69766C')
    return s

def album(x,y,size):
    s=rect(x,y,size,size,16,'url(#album)')
    s+=f'<circle cx="{x+size*.69}" cy="{y+size*.29}" r="{size*.13}" fill="#F9F1DE"/>'
    s+=f'<path d="M{x} {y+size*.74} Q{x+size*.24} {y+size*.42} {x+size*.55} {y+size*.73} Q{x+size*.78} {y+size*.57} {x+size} {y+size*.75} V{y+size-10} Q{x+size} {y+size} {x+size-10} {y+size} H{x+10} Q{x} {y+size} {x} {y+size-10} Z" fill="#8D9A81"/>'
    s+=f'<path d="M{x} {y+size*.9} Q{x+size*.4} {y+size*.67} {x+size} {y+size*.89} V{y+size-10} Q{x+size} {y+size} {x+size-10} {y+size} H{x+10} Q{x} {y+size} {x} {y+size-10} Z" fill="#66745D"/>'
    return s

def card(x,y,w,h):
    return rect(x,y,w,h,28,'#FFFFFF',extra='fill-opacity=".9" filter="url(#shadow)"')

def home():
    s=base('驻车首页 静态设计稿')+status()
    s+='<ellipse cx="571" cy="426" rx="590" ry="290" fill="url(#glow)"/>'
    s+=text(56,126,'下午好，林先生',34,weight=500)+text(57,164,'11月1日 周日 · 车辆已就绪',19,C['muted'])
    s+=text(59,247,'82',50,weight=400)+text(128,247,'%',24,C['muted'])+text(59,278,'电池电量',18,C['muted'])
    s+=line(196,207,196,278,'#D4DDD1')+text(232,247,'426',50)+text(340,247,'km',23,C['muted'])+text(234,278,'预计续航',18,C['muted'])
    for x,name,label in [(716,'lock','车锁'),(834,'trunk','后备厢'),(952,'charge','充电口')]:
        s+=rect(x,204,72,62,20,'#FFFFFF',extra='fill-opacity=".74"')+icon(name,x+23,221,27,'#667A6A')+text(x+36,294,label,17,C['muted'],anchor='middle')
    s+=f'<image x="38" y="270" width="1000" height="528" href="{CAR}"/>'
    s+=icon('lock',458,255,21,C['green'])+text(493,275,'车辆已锁定',18,C['green'])
    # Destination card.
    s+=card(1110,92,438,285)
    s+=icon('search',1138,121,27,C['green'])+text(1181,144,'想去哪里？',27,weight=500)
    s+=text(1138,190,'常用目的地',17,C['muted'])
    for x,name,label,time in [(1138,'home','回家','28 分钟'),(1336,'briefcase','公司','18 分钟')]:
        s+=rect(x,209,184,80,18,'#F0F3ED')+icon(name,x+16,226,25,C['green'])+text(x+54,239,label,21,weight=500)+text(x+54,269,time,17,C['muted'])
    s+=line(1139,312,1519,312,'#E6EBE2')+icon('charge',1139,332,23,C['green'])+text(1175,352,'附近充电站',20)+text(1483,352,'查看',18,C['muted'],anchor='end')+icon('chevron-right',1494,334,22,C['muted'])
    # Music card.
    s+=card(1110,397,438,235)+album(1138,425,99)
    s+=text(1259,457,'沿途',27,weight=500)+text(1260,490,'轻音乐精选',18,C['muted'])
    s+=icon('music',1486,429,23,C['muted'])
    s+=line(1139,553,1519,553,'#E5EADF',4,extra='stroke-linecap="round"')+line(1139,553,1271,553,C['green'],4,extra='stroke-linecap="round"')
    s+=text(1139,583,'01:28',15,C['muted'])+text(1519,583,'04:16',15,C['muted'],anchor='end')
    s+=icon('previous',1279,584,22,C['ink'])
    s+=rect(1339,574,42,42,21,C['active'])+icon('pause',1349,584,22,C['green'])+icon('skip',1418,584,22,C['ink'])
    # Scene shortcuts.
    s+=card(1110,652,438,110)
    s+=text(1138,692,'场景模式',19,C['muted'])
    s+=pill(1243,676,125,'小憩',False,'moon',h=52)+pill(1383,676,135,'露营',False,'tent',h=52)
    s+=dock('首页')+'</g></svg>'
    return s

def map_background():
    s='<rect x="0" y="65" width="1600" height="725" fill="#EAEEE7"/>'
    s+='<path d="M1406 64C1308 179 1491 230 1370 337S1384 538 1339 639 1336 735 1269 791H1600V64Z" fill="url(#water)"/>'
    # Parks and blocks are deliberately schematic, not a geographic map.
    for x,y,w,h,r in [(552,112,173,104,18),(1000,458,232,132,28),(638,496,136,115,18),(1090,94,185,87,18),(475,637,270,83,20),(1003,651,235,105,24)]:
        s+=rect(x,y,w,h,r,'#DDE7D6')
    for x,y,w,h in [(792,108,197,90),(514,276,107,83),(745,275,170,75),(1022,305,208,97),(502,478,104,118),(958,472,26,117),(789,660,71,98)]:
        s+=rect(x,y,w,h,10,'#E2E7DE')
    paths=['M458 243H1303','M457 425H1352','M447 631H1318','M678 64V790','M899 64V790','M1262 66V790',
           'M457 154H1020','M484 335H1317','M468 542H1320','M481 723H1290','M792 70V780','M1098 65V790',
           'M449 93C680 245 1013 453 1320 673']
    for i,path in enumerate(paths):
        width=20 if i<6 else 11
        s+=f'<path d="{path}" fill="none" stroke="#D9E0D5" stroke-width="{width+3}" stroke-linejoin="round"/>'
        s+=f'<path d="{path}" fill="none" stroke="#FCFCF7" stroke-width="{width}" stroke-linejoin="round"/>'
    for x,y,label in [(991,232,'云杉路'),(1120,413,'望江路'),(1061,620,'栖霞路'),(560,536,'云杉公园'),(1110,529,'滨江绿地'),(1429,555,'浦 江')]:
        s+=text(x,y,label,17,'#95A18F',anchor='middle')
    s+=text(878,577,'滨江大道',17,'#95A18F',extra='transform="rotate(-90 878 577)"')
    return s

def navigation():
    s=base('行车导航 静态设计稿')+map_background()+status('D')
    route='M899 753V443Q899 425 881 425H699Q678 425 678 404V267Q678 243 702 243H1183'
    s+=f'<path d="{route}" fill="none" stroke="#FFFFFF" stroke-width="22" stroke-linecap="round" stroke-linejoin="round"/>'
    s+=f'<path d="{route}" fill="none" stroke="#577E68" stroke-width="13" stroke-linecap="round" stroke-linejoin="round"/>'
    for x,y in [(899,505),(899,585)]:
        s+=f'<path d="m{x-5} {y+5} 5-6 5 6" fill="none" stroke="#FFF" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/>'
    s+='<circle cx="899" cy="661" r="42" fill="#709C7D" fill-opacity=".14"/><circle cx="899" cy="661" r="29" fill="#FFF"/>'
    s+='<path d="m899 638 15 37-15-7-15 7Z" fill="#426C55"/>'
    s+='<circle cx="1183" cy="243" r="19" fill="#FFF"/><circle cx="1183" cy="243" r="12" fill="#577E68"/>'
    s+=rect(1041,189,245,42,16,'#FFFFFF')+text(1060,217,'滨江艺术中心',20)+text(1269,217,'终点',15,C['muted'],anchor='end')
    # Primary manoeuvre.
    s+=rect(40,92,398,203,28,'#344F42',extra='filter="url(#shadow)"')+icon('arrow-left-turn',70,134,69,'#FFFFFF',2.1)
    s+=text(169,171,'300',55,'#FFFFFF',500)+text(287,171,'米',24,'#D6E2D5')
    s+=text(171,214,'左转进入',20,'#C4D6C7')+text(171,252,'望江路',29,'#FFFFFF',500)
    # Remaining route.
    s+=card(40,311,398,124)+text(68,357,'18',39,weight=500)+text(124,357,'分钟',19,C['muted'])
    s+=text(209,355,'12',29)+text(254,355,'公里',18,C['muted'])+text(68,400,'预计 16:46 到达',20,C['muted'])+icon('pin',376,368,27,C['green'])
    # Lane instruction.
    s+=card(665,90,469,92)
    for i in range(5):
        x=707+i*71
        s+=icon('arrow-up',x,108,29,C['green'] if i in [0,1] else '#BEC7BA',2.2)
    s+=text(900,162,'保持左侧车道',17,C['muted'],anchor='middle')
    # Context media.
    s+=card(40,541,398,152)+album(65,565,77)+text(163,594,'沿途',24,weight=500)+text(164,626,'轻音乐精选',17,C['muted'])
    s+=icon('pause',333,573,26,C['green'])+icon('skip',383,573,23,C['ink'])
    s+=line(164,657,405,657,'#E2E8DC',3,extra='stroke-linecap="round"')+line(164,657,246,657,C['green'],3,extra='stroke-linecap="round"')
    s+=pill(40,717,145,'结束导航',False,'close',h=48)+pill(199,717,145,'路线总览',False,'layers',h=48)
    # Map controls and speed.
    for y,name in [(98,'layers'),(174,'plus'),(240,'minus'),(644,'locate')]:
        s+=rect(1487,y,65,60,18,'#FFFFFF',extra='filter="url(#shadow)"')+icon(name,1506,y+18,25,C['green'])
    s+=rect(1379,698,173,65,21,'#FFFFFF',extra='fill-opacity=".96"')
    s+=text(1416,744,'58',38,weight=500,anchor='middle')+text(1451,744,'km/h',15,C['muted'])
    s+='<circle cx="1520" cy="730" r="20" fill="#FFFFFF" stroke="#B47B6B" stroke-width="3"/>'+text(1520,737,'60',19,anchor='middle')
    s+=dock('导航')+'</g></svg>'
    return s

def hotspot(x,y,label,lx,ly,w=168):
    s=f'<circle cx="{x}" cy="{y}" r="17" fill="#FFFFFF" fill-opacity=".65"/><circle cx="{x}" cy="{y}" r="7" fill="#567C65" stroke="#FFFFFF" stroke-width="3"/>'
    s+=f'<path d="M{x} {y} L{lx+20} {ly+24}" stroke="#9AAD9B" stroke-width="1.5" fill="none"/>'
    s+=rect(lx,ly,w,48,17,'#FFFFFF',extra='fill-opacity=".96" filter="url(#shadow)"')+text(lx+19,ly+31,label,18,C['green'],500)
    return s

def controls():
    s=base('车辆控制 静态设计稿')+status()
    s+=text(43,121,'车辆',34,weight=500)
    entries=[('settings','快捷控制'),('window','车门车窗'),('seat','座椅舒适'),('light','灯光'),('mirror','后视镜'),('charge','充电')]
    for i,(name,label) in enumerate(entries):
        y=163+i*77;active=i==1
        if active:s+=rect(24,y,202,62,18,'#DDE8DB')
        s+=icon(name,43,y+18,26,C['green'] if active else '#8A9589')+text(88,y+41,label,23,C['green'] if active else '#738071',500 if active else 400)
    s+=line(249,103,249,745,'#DCE4D7')
    s+=text(290,126,'车门与车窗',32,weight=500)+pill(1023,95,114,'已锁车',True,'lock',h=40)
    s+='<ellipse cx="705" cy="428" rx="495" ry="280" fill="url(#glow)"/>'
    s+=f'<image x="265" y="254" width="900" height="475" href="{CAR}"/>'
    s+=hotspot(845,393,'左前车窗 · 已关闭',645,203,221)
    s+=hotspot(1117,417,'后备厢 · 已关闭',942,245,210)
    s+=pill(501,711,105,'车外',True,h=43)+pill(616,711,105,'车内',False,h=43)+icon('refresh',758,720,25,C['muted'])
    # Context-specific control panel.
    s+=card(1175,93,379,670)
    s+=text(1204,140,'车窗控制',27,weight=500)+text(1204,180,'全车车窗已关闭',18,C['muted'])+text(1524,180,'全部关闭',17,C['green'],anchor='end')
    for x,y,label,active in [(1203,209,'左前',True),(1373,209,'右前',False),(1203,293,'左后',False),(1373,293,'右后',False)]:
        s+=rect(x,y,151,70,18,C['active'] if active else '#F4F6F0')
        s+=icon('window',x+15,y+21,24,C['green'] if active else '#8F9B8E')+text(x+51,y+31,label,20,C['green'] if active else C['ink'],500)
        s+=text(x+51,y+55,'已关闭',15,C['muted'])
    s+=rect(1203,385,151,59,18,'#557A65')+icon('arrow-down',1221,403,22,'#FFFFFF')+text(1256,423,'开启',21,'#FFFFFF',500)
    s+=rect(1373,385,151,59,18,'#EDF1E9')+icon('arrow-up',1400,403,22,'#A0AA9C')+text(1437,423,'关闭',20,'#A0AA9C',500)
    s+=line(1204,472,1524,472,'#E6EBE1')
    s+=icon('lock',1203,499,26,C['green'])+text(1245,519,'车门锁',21)+text(1245,548,'已锁车',16,C['muted'])
    s+=rect(1460,505,62,34,17,'#6E8C70')+'<circle cx="1505" cy="522" r="13" fill="#FFFFFF"/>'
    s+=line(1204,573,1524,573,'#E6EBE1')
    s+=icon('trunk',1203,597,27,C['muted'])+text(1245,620,'后备厢',21)+text(1245,647,'已关闭',16,C['muted'])
    s+=pill(1420,602,104,'开启',False,h=43)
    s+=line(1204,668,1524,668,'#E6EBE1')
    s+=icon('charge',1203,693,26,C['muted'])+text(1245,717,'充电口',21)+text(1245,743,'已关闭',16,C['muted'])
    s+=pill(1420,697,104,'开启',False,h=43)
    s+=dock('车辆')+'</g></svg>'
    return s

if __name__=='__main__':
    for name,fn in [('01-parking',home),('02-navigation',navigation),('03-vehicle-controls',controls)]:
        (ROOT/'screens'/f'{name}.svg').write_text(fn(),encoding='utf-8')
        print(name)
