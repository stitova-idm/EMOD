
#pragma once

#include "IVectorCohort.h"
#include "ISerializable.h"

namespace Kernel
{
    struct INodeContext;
    struct IMigrationInfo;

    struct IVectorPopulation : ISerializable
    {
        virtual const std::string& get_SpeciesID() const = 0;
        virtual void SetContextTo( INodeContext *context ) = 0;
        virtual void SetupLarvalHabitat( INodeContext *context ) = 0;
        virtual void SetVectorMortality( bool mortality ) = 0;

        // The function that NodeVector calls into once per species per timestep
        virtual void UpdateVectorPopulation( float dt ) = 0;

        // For NodeVector to calculate # of migrating vectors (processEmigratingVectors) and put them in new node (processImmigratingVector)
        virtual void Vector_Migration( float dt, IMigrationInfo* pMigInfo, VectorCohortVector_t* pMigratingQueue ) = 0;
        virtual void AddImmigratingVector( IVectorCohort* pvc ) = 0;
        virtual void SetSortingVectors() = 0;
        virtual void SortImmigratingVectors() = 0;

        // Supports MosquitoRelease intervention
        virtual void AddVectors( const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious ) = 0;
    };
}
