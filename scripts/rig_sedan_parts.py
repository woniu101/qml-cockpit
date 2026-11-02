"""Split the existing mirror geometry into real hinge nodes; preserve materials.

The original CC BY 4.0 GLB is never changed. Only index buffers and two transform
nodes are added to a derived GLB used by Balsam. No generated replacement car.
"""
from pathlib import Path
import copy
import json
import struct

ROOT=Path(__file__).resolve().parents[1]

def rig():
    source=ROOT/'assets/sedan/source/Sedan.glb'
    raw=source.read_bytes();length=struct.unpack_from('<I',raw,12)[0]
    document=json.loads(raw[20:20+length]);binary=bytearray(raw[28+length:])
    def read_accessor(index):
        accessor=document['accessors'][index];view=document['bufferViews'][accessor['bufferView']]
        fmt={5126:'f',5125:'I',5123:'H',5121:'B'}[accessor['componentType']]
        components={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[accessor['type']]
        size=struct.calcsize('<'+fmt*components);offset=view.get('byteOffset',0)+accessor.get('byteOffset',0)
        return [struct.unpack_from('<'+fmt*components,binary,offset+i*view.get('byteStride',size)) for i in range(accessor['count'])]
    def index_accessor(values):
        while len(binary)%4:binary.append(0)
        offset=len(binary);binary.extend(struct.pack('<'+'I'*len(values),*values))
        view=len(document['bufferViews']);document['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(values)*4,'target':34963})
        index=len(document['accessors']);document['accessors'].append({'bufferView':view,'componentType':5125,'count':len(values),'type':'SCALAR','min':[min(values)],'max':[max(values)]})
        return index
    report=[]
    for side in ['lf','rf']:
        parent=next(node for node in document['nodes'] if node.get('name')==f'door_{side}')
        primitives=[];triangles=0
        for node_index in list(parent.get('children',[])):
            node=document['nodes'][node_index]
            if 'mesh' not in node:continue
            mesh=document['meshes'][node['mesh']];remaining_primitives=[]
            for primitive in mesh['primitives']:
                positions=read_accessor(primitive['attributes']['POSITION'])
                indices=[v[0] for v in read_accessor(primitive['indices'])]
                selected=[];remaining=[]
                for start in range(0,len(indices),3):
                    triangle=indices[start:start+3]
                    centroid=[sum(positions[i][axis] for i in triangle)/3 for axis in range(3)]
                    x,y,z=centroid
                    mirror=(-0.245<x<-0.012 if side=='lf' else 0.012<x<0.245) and -0.55<y<-0.28 and 0.245<z<0.48
                    (selected if mirror else remaining).extend(triangle)
                if selected:
                    part=copy.deepcopy(primitive);part['indices']=index_accessor(selected);primitives.append(part);triangles+=len(selected)//3
                if remaining:
                    kept=copy.deepcopy(primitive)
                    if selected:kept['indices']=index_accessor(remaining)
                    remaining_primitives.append(kept)
            if not remaining_primitives:
                del node['mesh']
            elif len(remaining_primitives)!=len(mesh['primitives']) or any(a.get('indices')!=b.get('indices') for a,b in zip(remaining_primitives,mesh['primitives'])):
                node['mesh']=len(document['meshes']);document['meshes'].append({'name':mesh.get('name','')+'_without_mirror','primitives':remaining_primitives})
        assert triangles>400,(side,triangles)
        mesh_index=len(document['meshes']);document['meshes'].append({'name':f'mirror_{side}_geometry','primitives':primitives})
        pivot=[-0.006 if side=='lf' else 0.006,-0.38,0.32]
        child_index=len(document['nodes']);document['nodes'].append({'name':f'mirror_{side}_surface','mesh':mesh_index,'translation':[-v for v in pivot]})
        pivot_index=len(document['nodes']);document['nodes'].append({'name':f'mirror_{side}_pivot','translation':pivot,'children':[child_index]})
        parent.setdefault('children',[]).append(pivot_index)
        report.append({'side':side,'triangles':triangles,'pivot':pivot})
    document['buffers'][0]['byteLength']=len(binary)
    encoded=json.dumps(document,separators=(',',':')).encode()
    encoded+=b' '*((-len(encoded))%4);binary.extend(b'\0'*((-len(binary))%4))
    result=struct.pack('<III',0x46546c67,2,12+8+len(encoded)+8+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary
    target=source.parent/'derived/Sedan.glb';target.parent.mkdir(exist_ok=True);target.write_bytes(result)
    source.with_name('parts-rig.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    return target

if __name__=='__main__':print(rig())
