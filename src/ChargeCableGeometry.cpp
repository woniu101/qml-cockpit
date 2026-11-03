#include "ChargeCableGeometry.h"
#include <QVector3D>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace {
    // A continuous rubber tube: strain relief, hanging section, then a loose floor curve.
    const std::array<QVector3D,9> points{{{132,76,-221},{139,60,-218},{139,28,-190},
        {143,6,-147},{146,2.1f,-110},{180,2.1f,-60},{212,2.1f,10},
        {234,2.1f,90},{410,2.1f,260}}};
    QVector3D curve(float at){
        const int i=std::min(int(at),int(points.size())-2);const float t=at-i;
        const auto a=points[std::max(i-1,0)],b=points[i],c=points[i+1],d=points[std::min(i+2,int(points.size())-1)];
        return 0.5f*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t);
    }
}
QVector3D ChargeCableGeometry::pointAt(qreal progress) const {return curve(float(std::clamp(progress,0.0,1.0))*(points.size()-1));}
ChargeCableGeometry::ChargeCableGeometry(QQuick3DObject *parent):QQuick3DGeometry(parent) {
    constexpr int rings=129,sides=12;constexpr float radius=1.65f,pi=3.14159265358979323846f;
    struct Vertex{float x,y,z,nx,ny,nz;};std::vector<Vertex> vertices;std::vector<quint32> indices;
    vertices.reserve(rings*sides);indices.reserve((rings-1)*sides*6);
    QVector3D low(1e6,1e6,1e6),high(-1e6,-1e6,-1e6);
    for(int i=0;i<rings;++i){
        const float at=float(i)/(rings-1)*(points.size()-1);const auto center=curve(at);
        const auto tangent=(curve(std::min(at+0.01f,float(points.size()-1)))-curve(std::max(at-0.01f,0.0f))).normalized();
        const auto normal=QVector3D::crossProduct(tangent,std::abs(tangent.y())>0.95f?QVector3D(1,0,0):QVector3D(0,1,0)).normalized();
        const auto binormal=QVector3D::crossProduct(tangent,normal).normalized();
        for(int j=0;j<sides;++j){
            const float angle=2*pi*j/sides;const auto n=normal*std::cos(angle)+binormal*std::sin(angle);const auto v=center+n*radius;
            vertices.push_back({v.x(),v.y(),v.z(),n.x(),n.y(),n.z()});
            for(int axis=0;axis<3;++axis){low[axis]=std::min(low[axis],v[axis]);high[axis]=std::max(high[axis],v[axis]);}
            if(i<rings-1){const quint32 a=i*sides+j,b=i*sides+(j+1)%sides,c=a+sides,d=b+sides;indices.insert(indices.end(),{a,b,c,b,d,c});}
        }
    }
    setStride(sizeof(Vertex));setPrimitiveType(PrimitiveType::Triangles);
    addAttribute(Attribute::PositionSemantic,0,Attribute::F32Type);addAttribute(Attribute::NormalSemantic,3*sizeof(float),Attribute::F32Type);addAttribute(Attribute::IndexSemantic,0,Attribute::U32Type);
    setVertexData(QByteArray(reinterpret_cast<const char*>(vertices.data()),qsizetype(vertices.size()*sizeof(Vertex))));
    setIndexData(QByteArray(reinterpret_cast<const char*>(indices.data()),qsizetype(indices.size()*sizeof(quint32))));setBounds(low,high);
}
