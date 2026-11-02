from pathlib import Path
import json
import re
from html import escape
from urllib.parse import quote

from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    Image, PageBreak, KeepTogether, Flowable
)
from PIL import Image as PILImage
from pypdf import PdfReader

ROOT = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / 'research/report_data.json').read_text(encoding='utf-8'))
out = ROOT / 'output/pdf/国内新能源汽车车机界面调研报告.pdf'
pdfmetrics.registerFont(TTFont('YaHei', r'C:\Windows\Fonts\msyh.ttc', subfontIndex=0))
pdfmetrics.registerFont(TTFont('YaHeiBold', r'C:\Windows\Fonts\msyhbd.ttc', subfontIndex=0))
pdfmetrics.registerFontFamily('YaHei', normal='YaHei', bold='YaHeiBold')
INK = colors.HexColor('#17232b')
MUTED = colors.HexColor('#536572')
TEAL = colors.HexColor('#236c62')
LIGHT = colors.HexColor('#edf2f4')
WIDTH = A4[0] - 92

styles = {
    'title': ParagraphStyle('Title', fontName='YaHeiBold', fontSize=22, leading=33, textColor=INK, spaceAfter=12),
    'h': ParagraphStyle('Heading', fontName='YaHeiBold', fontSize=12, leading=18, spaceBefore=10, spaceAfter=6, textColor=INK, keepWithNext=True),
    'p': ParagraphStyle('Body', fontName='YaHei', fontSize=10, leading=16.5, textColor=INK, spaceAfter=6, wordWrap='CJK'),
    'lead': ParagraphStyle('Lead', fontName='YaHei', fontSize=10.8, leading=18, textColor=MUTED, spaceAfter=13, wordWrap='CJK'),
    'caption': ParagraphStyle('Caption', fontName='YaHei', fontSize=8, leading=12, textColor=MUTED, spaceAfter=10, wordWrap='CJK'),
    'cell': ParagraphStyle('Cell', fontName='YaHei', fontSize=9, leading=14, textColor=INK, wordWrap='CJK'),
    'th': ParagraphStyle('TableHeader', fontName='YaHeiBold', fontSize=9, leading=14, textColor=colors.white, wordWrap='CJK'),
    'source': ParagraphStyle('Source', fontName='YaHei', fontSize=8.6, leading=13, textColor=INK, spaceAfter=7, wordWrap='CJK'),
}
source_map = {s['id']: s for s in data['sources']}
def url(u):
    return quote(u, safe=':/?=&%#-_.~')

def rich(s):
    s = escape(s)
    return re.sub(r'\[(S\d+)\]', lambda m: '<link href="' + escape(url(source_map[m[1]]['url'])) + '" color="#236c62">[' + m[1] + ']</link>', s)

def p(s, kind='p'):
    return Paragraph(rich(s), styles[kind])

def image_fit(file, maxw, maxh):
    path = ROOT / file
    with PILImage.open(path) as im:
        w, h = im.size
    scale = min(maxw / w, maxh / h)
    return Image(str(path), width=w*scale, height=h*scale, hAlign='CENTER')

class Wireframes(Flowable):
    def __init__(self):
        super().__init__()
        self.width = WIDTH
        self.height = 296

    def draw(self):
        c = self.canv
        w = self.width
        def label(x, y, text, size=9, color=INK):
            c.setFillColor(color); c.setFont('YaHei', size); c.drawString(x, y, text)
        def pane(x, y, pw, ph, fill, text):
            c.setFillColor(colors.HexColor(fill)); c.setStrokeColor(colors.HexColor('#d5dfe3'))
            c.roundRect(x, y, pw, ph, 7, fill=1, stroke=1)
            label(x+10, y+ph/2-3, text)
        for base, title in [(156, '驻车桌面   车辆状态与出发准备'), (4, '行车桌面   导航任务优先')]:
            label(0, base+129, title, 10, TEAL)
            c.setFillColor(colors.HexColor('#f4f5f3')); c.setStrokeColor(colors.HexColor('#c4ced3'))
            c.roundRect(0, base, w, 119, 9, fill=1, stroke=1)
            label(12, base+103, 'P  /  D     电量与连接状态', 8, MUTED)
            label(w-85, base+103, '时间   天气', 8, MUTED)
            if base == 156:
                pane(10,base+32, w*.62, 61, '#e5ecea', '3D 车辆  ·  部件热点与状态')
                pane(w*.62+20,base+65,w*.38-30,28,'#ffffff','导航目的地')
                pane(w*.62+20,base+32,w*.38-30,28,'#ffffff','正在播放')
            else:
                pane(10,base+32,w*.71,61,'#e5ecea','导航地图  ·  转向与路线信息')
                pane(w*.71+20,base+32,w*.29-30,61,'#ffffff','媒体与任务卡片')
            pane(10,base+7,w-20,19,'#e5e9e8','主页     导航     车辆       主驾温度     风量     副驾温度     媒体')

def footer(c, doc):
    c.saveState()
    c.setFont('YaHei', 8)
    c.setFillColor(MUTED)
    c.drawString(46, A4[1]-32, '车机界面调研  /  QML 演示设计参考')
    c.drawRightString(A4[0]-46, A4[1]-32, '2026.11.01')
    c.setStrokeColor(colors.HexColor('#d6dfe2'))
    c.line(46, 39, A4[0]-46, 39)
    c.setFont('YaHei', 8)
    c.drawString(46, 25, '公开资料研究  ·  设计判断与项目建议已分别标注')
    c.drawRightString(A4[0]-46, 25, f'{doc.page:02d}')
    c.restoreState()

story=[]
for idx, page in enumerate(data['pages']):
    if idx:
        story.append(PageBreak())
    if idx == 0:
        story.append(p(data['title'], 'title'))
        story.append(p(data['subtitle'] + '  |  ' + data['date'], 'caption'))
        story.append(p(page['title'], 'h'))
    else:
        story.append(p(f'{idx+1:02d}  ' + page['title'], 'title'))
    story.append(p(page['lead'], 'lead'))
    if 'figures' in page:
        for f in page['figures']:
            story.append(KeepTogether([image_fit(f['file'], WIDTH, 174), Spacer(1,6), p(f['caption'],'caption')]))
    if 'pairfigures' in page:
        figs=page['pairfigures']
        columns=[]
        for f in figs:
            columns.append([image_fit(f['file'], WIDTH/2-14, 190), Spacer(1,7), p(f['caption'],'caption')])
        t=Table([columns],colWidths=[WIDTH/2]*2)
        t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),3),('RIGHTPADDING',(0,0),(-1,-1),8)]))
        story.extend([t,Spacer(1,8)])
    if 'table' in page:
        tab=page['table']
        cells=[[p(c,'th') for c in tab['headers']]]+[[p(c,'cell') for c in row] for row in tab['rows']]
        widths=[WIDTH*.19,WIDTH*.40,WIDTH*.41]
        if idx in (6,8): widths=[WIDTH*.33,WIDTH*.39,WIDTH*.28]
        t=Table(cells,colWidths=widths,hAlign='LEFT',repeatRows=1)
        ts=[('BACKGROUND',(0,0),(-1,0),colors.HexColor('#243e4b')),
            ('GRID',(0,0),(-1,-1),.5,colors.HexColor('#d9d9d9')),
            ('VALIGN',(0,0),(-1,-1),'MIDDLE'),
            ('LEFTPADDING',(0,0),(-1,-1),8),('RIGHTPADDING',(0,0),(-1,-1),8),
            ('TOPPADDING',(0,0),(-1,-1),8),('BOTTOMPADDING',(0,0),(-1,-1),8)]
        for row in range(1,len(cells)):
            ts.append(('BACKGROUND',(0,row),(-1,row),colors.HexColor('#f1f5f6') if row%2==0 else colors.white))
        t.setStyle(TableStyle(ts)); story.extend([t,Spacer(1,8)])
    if page.get('wireframes'):
        story.extend([Wireframes(),Spacer(1,10)])
    if page.get('sources'):
        for s in data['sources']:
            name=escape(s['id']+'  '+s['name'])
            html=f'<link href="{escape(url(s["url"]))}" color="#236c62"><b>{name}</b></link><br/>{escape(s["note"])}'
            story.append(Paragraph(html,styles['source']))
    for block in page.get('blocks',[]):
        story.append(p(block['heading'],'h'))
        if 'text' in block: story.append(p(block['text']))
        for i,item in enumerate(block.get('items',[]),1):
            story.append(p(f'{i}. '+item))

doc=SimpleDocTemplate(str(out),pagesize=A4,rightMargin=46,leftMargin=46,topMargin=58,bottomMargin=53,
    title=data['title'],author='Codex',subject=data['subtitle'])
doc.build(story,onFirstPage=footer,onLaterPages=footer)
reader=PdfReader(str(out))
print('PDF pages:',len(reader.pages),'bytes:',out.stat().st_size)
for i,page in enumerate(reader.pages):
    text=page.extract_text()
    print(i+1,text.splitlines()[2:4])

# Editable companion, with sources next to claims and original reference images.
md=[f'# {data["title"]}',f'\n{data["subtitle"]}  \n检索日期：{data["date"]}\n']
def md_links(t):
    return re.sub(r'\[(S\d+)\]',lambda m:f'[{m[1]}]({url(source_map[m[1]]["url"])})',t)
for i,page in enumerate(data['pages'],1):
    md.extend([f'## {i:02d} {page["title"]}',md_links(page['lead'])])
    for f in page.get('figures',[])+page.get('pairfigures',[]):
        md.extend([f'![{f["caption"].split("。")[0]}]({(ROOT/f["file"]).as_posix()})',md_links(f['caption'])])
    if 'table' in page:
        t=page['table'];md.extend(['| '+' | '.join(t['headers'])+' |','| '+' | '.join(['---']*len(t['headers']))+' |'])
        md.extend('| '+' | '.join(md_links(c) for c in row)+' |' for row in t['rows'])
    if page.get('wireframes'):
        md.extend(['\n| 层级 | 驻车桌面 | 行车桌面 |','| --- | --- | --- |','| 顶部 | 挡位、电量、连接、时间 | 必要车辆状态与连接信息 |','| 主内容 | 3D 车辆、部件热点 | 地图、转向、路线 |','| 次内容 | 目的地、音乐卡片 | 媒体与当前任务卡片 |','| 底部 | 稳定的导航、空调与媒体入口 | 保持相同位置的高频控制 |'])
    if page.get('sources'):
        md.extend(f'- [{s["id"]} {s["name"]}]({url(s["url"])}): {s["note"]}' for s in data['sources'])
    for b in page.get('blocks',[]):
        md.extend([f'### {b["heading"]}'])
        if 'text' in b: md.append(md_links(b['text']))
        md.extend('- '+md_links(t) for t in b.get('items',[]))
    md.append('')
(ROOT/'research/国内新能源汽车车机界面调研报告.md').write_text('\n\n'.join(md),encoding='utf-8')

if len(reader.pages)!=10:
    raise SystemExit('Expected 10 pages; inspect overflow before delivery.')
