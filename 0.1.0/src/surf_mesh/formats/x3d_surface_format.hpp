#ifndef SURF_MESH_SURFACE_FORMATS_X3D_SURFACE_FORMAT_HPP_
#define SURF_MESH_SURFACE_FORMATS_X3D_SURFACE_FORMAT_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::fileFormats::X3DsurfaceFormat
// Description
//   Provide a means of writing x3d format.

#include "meshed_surface.hpp"
#include "meshed_surface_proxy.hpp"
#include "unsorted_meshed_surface.hpp"
#include "x3d_surface_format_core.hpp"


namespace mousse {
namespace fileFormats {

template<class Face>
class X3DsurfaceFormat
:
  public MeshedSurface<Face>,
  public X3DsurfaceFormatCore
{
public:
  // Constructors
    //- Construct null
    X3DsurfaceFormat();

    //- Disallow default bitwise copy construct
    X3DsurfaceFormat(const X3DsurfaceFormat<Face>&) = delete;

    //- Disallow default bitwise assignment
    X3DsurfaceFormat<Face>& operator=(const X3DsurfaceFormat<Face>&) = delete;

  //- Destructor
  virtual ~X3DsurfaceFormat()
  {}

  // Member Functions

    //- Write surface mesh components by proxy
    static void write(const fileName&, const MeshedSurfaceProxy<Face>&);

    //- Write object file
    virtual void write(const fileName& name) const
    {
      write(name, MeshedSurfaceProxy<Face>(*this));
    }

};

}  // namespace fileFormats

}  // namespace mousse

#include "x3d_surface_format.ipp"

#endif
