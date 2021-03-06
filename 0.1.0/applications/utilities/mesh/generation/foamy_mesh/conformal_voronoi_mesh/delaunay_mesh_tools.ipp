// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "delaunay_mesh_tools.hpp"
#include "mesh_tools.hpp"
#include "ofstream.hpp"
#include "point_conversion.hpp"
#include "point_io_field.hpp"
#include "indexed_vertex_ops.hpp"


// Member Functions 
template<typename Triangulation>
void mousse::DelaunayMeshTools::writeOBJ
(
  const fileName& fName,
  const Triangulation& t,
  const indexedVertexEnum::vertexType startPointType,
  const indexedVertexEnum::vertexType endPointType
)
{
  OFstream str{fName};
  Pout << nl << "Writing points of types:" << nl;
  FOR_ALL_CONST_ITER
  (
    HashTable<int>,
    indexedVertexEnum::vertexTypeNames_,
    iter
  ) {
    if (iter() >= startPointType && iter() <= endPointType) {
      Pout << "    " << iter.key() << nl;
    }
  }
  Pout << "to " << str.name() << endl;
  for (auto vit = t.finite_vertices_begin();
       vit != t.finite_vertices_end();
       ++vit) {
    if (vit->type() >= startPointType && vit->type() <= endPointType) {
      meshTools::writeOBJ(str, topoint(vit->point()));
    }
  }
}


template<typename Triangulation>
void mousse::DelaunayMeshTools::writeOBJ
(
  const fileName& fName,
  const Triangulation& t,
  const indexedVertexEnum::vertexType pointType
)
{
  writeOBJ(fName, t, pointType, pointType);
}


template<typename Triangulation>
void mousse::DelaunayMeshTools::writeFixedPoints
(
  const fileName& fName,
  const Triangulation& t
)
{
  OFstream str{fName};
  Pout << nl << "Writing fixed points to " << str.name() << endl;
  for (auto vit = t.finite_vertices_begin();
       vit != t.finite_vertices_end();
       ++vit) {
    if (vit->fixed()) {
      meshTools::writeOBJ(str, topoint(vit->point()));
    }
  }
}


template<typename Triangulation>
void mousse::DelaunayMeshTools::writeBoundaryPoints
(
  const fileName& fName,
  const Triangulation& t
)
{
  OFstream str{fName};
  Pout << nl << "Writing boundary points to " << str.name() << endl;
  for (auto vit = t.finite_vertices_begin();
       vit != t.finite_vertices_end();
       ++vit) {
    if (!vit->internalPoint()) {
      meshTools::writeOBJ(str, topoint(vit->point()));
    }
  }
}


template<typename Triangulation>
void mousse::DelaunayMeshTools::writeProcessorInterface
(
  const fileName& fName,
  const Triangulation& t,
  const faceList& faces
)
{
  OFstream str{fName};
  pointField points{static_cast<label>(t.number_of_finite_cells()), point::max};
  for (auto cit = t.finite_cells_begin();
       cit != t.finite_cells_end();
       ++cit) {
    if (!cit->hasFarPoint() && !t.is_infinite(cit)) {
      points[cit->cellIndex()] = cit->dual();
    }
  }
  meshTools::writeOBJ(str, faces, points);
}


template<typename Triangulation>
void mousse::DelaunayMeshTools::writeInternalDelaunayVertices
(
  const fileName& instance,
  const Triangulation& t
)
{
  pointField internalDelaunayVertices(t.number_of_vertices());
  label vertI = 0;
  for (auto vit = t.finite_vertices_begin();
       vit != t.finite_vertices_end();
       ++vit) {
    if (vit->internalPoint()) {
      internalDelaunayVertices[vertI++] = topoint(vit->point());
    }
  }
  internalDelaunayVertices.setSize(vertI);
  pointIOField internalDVs
  {
    {
      "internalDelaunayVertices",
      instance,
      t.time(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    },
    internalDelaunayVertices
  };
  Info << nl << "Writing " << internalDVs.name() << " to "
    << internalDVs.instance() << endl;
  internalDVs.write();
}


template<typename CellHandle>
void mousse::DelaunayMeshTools::drawDelaunayCell
(
  Ostream& os,
  const CellHandle& c,
  label offset
)
{
  // Supply offset as tet number
  offset *= 4;
  os << "# cell index: " << label(c->cellIndex())
    << " INT_MIN = " << INT_MIN
    << endl;
  os << "# circumradius "
    << mag(c->dual() - topoint(c->vertex(0)->point()))
    << endl;
  for (int i = 0; i < 4; i++) {
    os << "# index / type / procIndex: "
      << label(c->vertex(i)->index()) << " "
      << label(c->vertex(i)->type()) << " "
      << label(c->vertex(i)->procIndex())
      <<
      (
        CGAL::indexedVertexOps::uninitialised(c->vertex(i))
        ? " # This vertex is uninitialised!"
        : ""
      )
      << endl;
    meshTools::writeOBJ(os, topoint(c->vertex(i)->point()));
  }
  os << "f " << 1 + offset << " " << 3 + offset << " " << 2 + offset << nl
    << "f " << 2 + offset << " " << 3 + offset << " " << 4 + offset << nl
    << "f " << 1 + offset << " " << 4 + offset << " " << 3 + offset << nl
    << "f " << 1 + offset << " " << 2 + offset << " " << 4 + offset << endl;
}


template<typename Triangulation>
mousse::tmp<mousse::pointField> mousse::DelaunayMeshTools::allPoints
(
  const Triangulation& t
)
{
  tmp<pointField> tpts{new pointField{t.vertexCount(), point::max}};
  pointField& pts = tpts();
  for (auto vit = t.finite_vertices_begin();
       vit != t.finite_vertices_end();
       ++vit) {
    if (vit->internalOrBoundaryPoint() && !vit->referred()) {
      pts[vit->index()] = topoint(vit->point());
    }
  }
  return tpts;
}

