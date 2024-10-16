/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ScaleLarvalHabitat.h"
#include "NodeVectorEventContext.h" // for INodeVectorInterventionEffectsApply methods

SETUP_LOGGING( "ScaleLarvalHabitat" )

namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(ScaleLarvalHabitat)

    ScaleLarvalHabitat::ScaleLarvalHabitat()
    : SimpleVectorControlNode()
    , m_LHM(true)
    {
    }

    ScaleLarvalHabitat::ScaleLarvalHabitat( const ScaleLarvalHabitat& master )
    : SimpleVectorControlNode( master )
    , m_LHM( master.m_LHM )
    {
    }

    void ScaleLarvalHabitat::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        // Do not decay the scaled habitat,
        // although it can be overriden by another Distribute.

        // Just push its effects to the NodeEventContext
        ApplyEffects();
    }

    bool ScaleLarvalHabitat::Configure( const Configuration * inputJson )
    {
        m_LHM.Initialize();

        initConfigTypeMap("Larval_Habitat_Multiplier", &m_LHM, SLH_Larval_Habitat_Multiplier_DESC_TEXT);
    
        // Don't call subclass SimpleVectorControlNode::Configure() because it will add cost_per_unit
        return BaseNodeIntervention::Configure( inputJson );
    }

    void ScaleLarvalHabitat::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateLarvalHabitatReduction( m_LHM );
        }
        else
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, 
                                        "invic", "INodeVectorInterventionEffectsApply" );
        }
    }
}