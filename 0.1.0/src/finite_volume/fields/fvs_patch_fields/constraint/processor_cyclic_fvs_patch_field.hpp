// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::processorCyclicFvsPatchField
// Description
//   mousse::processorCyclicFvsPatchField
// SourceFiles
//   processor_cyclic_fvs_patch_field.cpp

#ifndef processor_cyclic_fvs_patch_field_hpp_
#define processor_cyclic_fvs_patch_field_hpp_

#include "coupled_fvs_patch_field.hpp"
#include "processor_cyclic_fv_patch.hpp"

namespace mousse
{

template<class Type>
class processorCyclicFvsPatchField
:
  public coupledFvsPatchField<Type>
{

  // Private data

    //- Local reference cast into the processor patch
    const processorCyclicFvPatch& procPatch_;

public:

  //- Runtime type information
  TYPE_NAME(processorCyclicFvPatch::typeName_());

  // Constructors

    //- Construct from patch and internal field
    processorCyclicFvsPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, surfaceMesh>&
    );

    //- Construct from patch and internal field and patch field
    processorCyclicFvsPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, surfaceMesh>&,
      const Field<Type>&
    );

    //- Construct from patch, internal field and dictionary
    processorCyclicFvsPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, surfaceMesh>&,
      const dictionary&
    );

    //- Construct by mapping given processorCyclicFvsPatchField onto a
    //  new patch
    processorCyclicFvsPatchField
    (
      const processorCyclicFvsPatchField<Type>&,
      const fvPatch&,
      const DimensionedField<Type, surfaceMesh>&,
      const fvPatchFieldMapper&
    );

    //- Construct as copy
    processorCyclicFvsPatchField(const processorCyclicFvsPatchField<Type>&);

    //- Construct and return a clone
    virtual tmp<fvsPatchField<Type> > clone() const
    {
      return tmp<fvsPatchField<Type> >
      {
        new processorCyclicFvsPatchField<Type>{*this}
      };
    }

    //- Construct as copy setting internal field reference
    processorCyclicFvsPatchField
    (
      const processorCyclicFvsPatchField<Type>&,
      const DimensionedField<Type, surfaceMesh>&
    );

    //- Construct and return a clone setting internal field reference
    virtual tmp<fvsPatchField<Type> > clone
    (
      const DimensionedField<Type, surfaceMesh>& iF
    ) const
    {
      return tmp<fvsPatchField<Type> >
      {
        new processorCyclicFvsPatchField<Type>{*this, iF}
      };
    }

  //- Destructor
  virtual ~processorCyclicFvsPatchField();

  // Member functions

    // Access

      //- Return true if running parallel
      virtual bool coupled() const
      {
        /*
        if (Pstream::parRun())
        {
          return true;
        }
        else
        {
          return false;
        }
        */

        return Pstream::parRun();
      }
};

}  // namespace mousse

#ifdef NoRepository
#   include "processor_cyclic_fvs_patch_field.cpp"
#endif

#endif
