// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::fv::interRegionOption
// Description
//   Base class for inter-region exchange.
#ifndef inter_region_option_hpp_
#define inter_region_option_hpp_
#include "fv_option.hpp"
#include "vol_fields.hpp"
#include "auto_ptr.hpp"
#include "mesh_to_mesh.hpp"
namespace mousse
{
namespace fv
{
class interRegionOption
:
  public option
{
protected:
  // Protected data
    //- Master or slave region
    bool master_;
    //- Name of the neighbour region to map
    word nbrRegionName_;
    //- Mesh to mesh interpolation object
    autoPtr<meshToMesh> meshInterpPtr_;
  // Protected member functions
    //- Set the mesh to mesh interpolation object
    void setMapper();
public:
  //- Runtime type information
  TYPE_NAME("interRegionOption");
  // Constructors
    //- Construct from dictionary
    interRegionOption
    (
      const word& name,
      const word& modelType,
      const dictionary& dict,
      const fvMesh& mesh
    );
  //- Destructor
  virtual ~interRegionOption();
  // Member Functions
    // Access
      //- Return const access to the neighbour region name
      inline const word& nbrRegionName() const;
      //- Return const access to the mapToMap pointer
      inline const meshToMesh& meshInterp() const;
    // IO
      //- Read dictionary
      virtual bool read(const dictionary& dict);
};
}  // namespace fv
}  // namespace mousse
#include "inter_region_option_i.hpp"
#endif
