/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"

#include "EoCommon.h"
#include "EOtheErrorManager.h"
#include "EoError.h"
#include "EOtheEntities.h"

#include "EOtheCANservice.h"
#include "EOtheCANmapping.h"
#include "EOtheCANprotocol.h"

#include "EoProtocolAS.h"

#include "EOMtheEMSappl.h"

#include "EOVtheCallbackManager.h"

// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheMAIS.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheMAIS_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables. deprecated: better using _get(), _set() on static variables 
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static eOresult_t s_eo_mais_TXstart(EOtheMAIS *p, uint8_t datarate, eOas_maismode_t mode, eOas_maisresolution_t resolution);

static eOresult_t s_eo_mais_TXstop(EOtheMAIS *p);

static void s_eo_mais_process_mais_resolution(eOas_maisresolution_t resolution, eOas_mais_status_t* status);

static eOresult_t s_eo_mais_onstop_search4mais(void *par, EOtheCANdiscovery2* cd2, eObool_t searchisok);

static void s_eo_mais_send_periodic_error_report(void *par);

static eObool_t s_eo_mais_isID32relevant(uint32_t id32);



// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static EOtheMAIS s_eo_themais = 
{
    .service = 
    {
        .servconfig             = { .type = eomn_serv_NONE },
        .initted                = eobool_false,
        .active                 = eobool_false,
        .activateafterverify    = eobool_false,
        .started                = eobool_false,
        .onverify               = NULL,
        .state                  = eomn_serv_state_notsupported
    },
    .diagnostics = 
    {
        .reportTimer            = NULL,
        .reportPeriod           = 0, // 10*EOK_reltime1sec, // with 0 we dont periodically report
        .errorDescriptor        = {0},
        .errorType              = eo_errortype_info,
        .errorCallbackCount     = 0, 
        .repetitionOKcase       = 0 // 10 // with 0 we transmit report only once at succesful activation
    },     
    .sharedcan =
    {
        .boardproperties        = NULL,
        .entitydescriptor       = NULL,
        .discoverytarget        = {0},
        .ondiscoverystop        = {0},
        .command                = {0}, 
    },    

    .id32                       = eo_prot_ID32dummy,  
    .mais                       = NULL,
    .id32ofregulars             = NULL,
    .numberofowners             = 0    
};

static const char s_eobj_ownname[] = "EOtheMAIS";


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOtheMAIS* eo_mais_Initialise(void)
{
    EOtheMAIS* p = &s_eo_themais;

    if(eobool_true == p->service.initted)
    {
        return(p);
    }
    
    p->id32 = eoprot_ID_get(eoprot_endpoint_analogsensors, eoprot_entity_as_mais, 0, eoprot_tag_none);

    p->service.servconfig.type = eomn_serv_NONE;
    
    
    p->sharedcan.boardproperties = eo_vector_New(sizeof(eObrd_canproperties_t), 1, NULL, NULL, NULL, NULL);
    
    p->sharedcan.entitydescriptor = eo_vector_New(sizeof(eOcanmap_entitydescriptor_t), 1, NULL, NULL, NULL, NULL);
    
    p->mais = NULL;
    p->id32ofregulars = eo_array_New(mais_maxRegulars, sizeof(uint32_t), NULL);
    p->numberofowners = 0;
    
    p->canmsgwatchdog = eo_canmsg_watchdog_new(NULL);
    p->diagnostics.reportTimer = eo_timer_New();
    p->diagnostics.errorType = eo_errortype_error;
    p->diagnostics.errorDescriptor.sourceaddress = 0;
    p->diagnostics.errorDescriptor.sourcedevice = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mais_not_verified_yet);  
        
    p->service.initted = eobool_true;    
    p->service.active = eobool_false;
    p->service.started = eobool_false;
    p->service.state = eomn_serv_state_idle;    
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);
       
    return(p);   
}


extern EOtheMAIS* eo_mais_GetHandle(void)
{
    if(eobool_true == s_eo_themais.service.initted)
    {
        return(&s_eo_themais);
    }
    
    return(NULL);
}


extern eOmn_serv_state_t eo_mais_GetServiceState(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(eomn_serv_state_notsupported);
    } 

    return(p->service.state);
}


extern eOresult_t eo_mais_SendReport(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    // the report is sent in any state ....

    eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
    
    eOerror_value_t errorvalue = eoerror_code2value(p->diagnostics.errorDescriptor.code);
    
    switch(errorvalue)
    {
        case eoerror_value_CFG_mais_failed_candiscovery:
        {
            eo_candiscovery2_SendLatestSearchResults(eo_candiscovery2_GetHandle());            
        } break;
        
        default:
        {
            // dont send any additional info
        } break;
    }       
    
    return(eores_OK);      
}


extern eOresult_t eo_mais_Verify(EOtheMAIS *p, const eOmn_serv_configuration_t * servcfg, eOservice_onendofoperation_fun_t onverify, eObool_t activateafterverify)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_mais_Verify()", s_eobj_ownname);
    
    if((NULL == p) || (NULL == servcfg))
    {
        s_eo_themais.service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, s_eo_themais.service.state);
        
        if(NULL != onverify)
        {
            onverify(p, eobool_false); 
        }            
        return(eores_NOK_nullpointer);
    }  
    
    if(eomn_serv_AS_mais != servcfg->type)
    {
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);
        if(NULL != onverify)
        {
            onverify(p, eobool_false); 
        }          
        return(eores_NOK_generic);
    } 



    if(eobool_true == p->service.active)
    {
        // it means that some other object has activated the mais. it can be either EOtheMotionController or EOtheServices upon request from embObjMais
        // ... we dont want to interfere, thus we cannot call eo_mais_Deactivate().  
        // we just verify that the eOmn_serv_configuration_t* in argument is the same of (or compatible with) the one already used for activation.
        
        eObool_t verificationOK = eobool_false;
        
        if(0 == memcmp(&p->service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t)))
        {
            verificationOK = eobool_true;
        }
        else
        {
            verificationOK = eobool_false;
            p->diagnostics.errorType = eo_errortype_error;
            p->diagnostics.errorDescriptor.par16 = 0;
            p->diagnostics.errorDescriptor.par64 = 0;
            p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mais_failed_verify_because_active);
            p->diagnostics.errorDescriptor.sourceaddress = 0;
            p->diagnostics.errorDescriptor.sourcedevice = eo_errman_sourcedevice_localboard; 
            eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor); 
        }
        
        // in here we dont activate because it is already active ...

        if(NULL != onverify)
        {
            onverify(p, verificationOK); 
        }          
        return((eobool_true == verificationOK) ? (eores_OK) : (eores_NOK_generic));        
    }     
 
    
    p->service.state = eomn_serv_state_verifying;   
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);

    // make sure the timer is not running
    eo_timer_Stop(p->diagnostics.reportTimer);      

    p->service.onverify = onverify;
    p->service.activateafterverify = activateafterverify;


    p->sharedcan.discoverytarget.info.type = eobrd_cantype_mais;
    p->sharedcan.discoverytarget.info.protocol.major = servcfg->data.as.mais.version.protocol.major; 
    p->sharedcan.discoverytarget.info.protocol.minor = servcfg->data.as.mais.version.protocol.minor;
    p->sharedcan.discoverytarget.info.firmware.major = servcfg->data.as.mais.version.firmware.major; 
    p->sharedcan.discoverytarget.info.firmware.minor = servcfg->data.as.mais.version.firmware.minor;
    p->sharedcan.discoverytarget.info.firmware.build = servcfg->data.as.mais.version.firmware.build;    
    p->sharedcan.discoverytarget.canmap[servcfg->data.as.mais.canloc.port] = 0x0001 << servcfg->data.as.mais.canloc.addr; 
    
    p->sharedcan.ondiscoverystop.function = s_eo_mais_onstop_search4mais;
    p->sharedcan.ondiscoverystop.parameter = (void*)servcfg;
        
    // start discovery    
    eo_candiscovery2_Start(eo_candiscovery2_GetHandle(), &p->sharedcan.discoverytarget, &p->sharedcan.ondiscoverystop);   
    
    return(eores_OK);   
}

extern uint8_t eo_mais_GetNumberOfOwners(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(0);
    } 

    return(p->numberofowners);   
}

extern eOresult_t eo_mais_Deactivate(EOtheMAIS *p)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_mais_Deactivate()", s_eobj_ownname);
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == p->service.active)
    {
        // i force to eomn_serv_state_idle because it may be that state was eomn_serv_state_verified or eomn_serv_state_failureofverify
        p->service.state = eomn_serv_state_idle; 
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);
        return(eores_OK);        
    } 
    
    // send stop messages to mais, unload the entity-can-mapping and the board-can-mapping, reset all things inside this object
       
    if(eobool_true == p->service.started)
    {
        eo_mais_Stop(p);
    }
    
    if(0 != p->numberofowners)
    {   // _Stop() decrements numberofowners and we can _Deactivate() only when there is no owner anymore
        return(eores_OK); 
    }
    
    eo_mais_SetRegulars(p, NULL, NULL);
    
    eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_analogsensors, eoprot_entity_as_mais, p->sharedcan.entitydescriptor); 
    
    eo_canmap_UnloadBoards(eo_canmap_GetHandle(), p->sharedcan.boardproperties);
     
    
    eo_entities_SetNumOfMaises(eo_entities_GetHandle(), 0);
    
    p->mais = NULL;
    
    memset(&p->service.servconfig, 0, sizeof(eOmn_serv_configuration_t));
    p->service.servconfig.type = eomn_serv_NONE;
    
    eo_vector_Clear(p->sharedcan.boardproperties);
    eo_vector_Clear(p->sharedcan.entitydescriptor);
    
    // make sure the timer is not running
    eo_timer_Stop(p->diagnostics.reportTimer); 
    
    p->service.active = eobool_false;    
    p->service.state = eomn_serv_state_idle; 
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);
    
    return(eores_OK);
}


extern eOresult_t eo_mais_Activate(EOtheMAIS *p, const eOmn_serv_configuration_t * servcfg)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_mais_Activate()", s_eobj_ownname);
    
    if((NULL == p) || (NULL == servcfg))
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eomn_serv_AS_mais != servcfg->type)
    {
        return(eores_NOK_generic);
    }    
    
    if(eobool_true == p->service.active)
    {
        // it means that some other object has activated the mais. it can be either EOtheMotionController or EOtheServices upon request from embObjMais
        // ... we dont want to interfere, thus we cannot call eo_mais_Deactivate().  
        // we just verify that the eOmn_serv_configuration_t* in argument is the same of (or compatible with) the one already used for activation.
        
        eObool_t verificationOK = eobool_false;
        
        if(0 == memcmp(&p->service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t)))
        {
            verificationOK = eobool_true;
        }
        else
        {
            verificationOK = eobool_false;
        }
        
        // in here we dont activate because it is already active ...
        
        return((eobool_true == verificationOK) ? (eores_OK) : (eores_NOK_generic));        
    }   
    
    eo_entities_SetNumOfMaises(eo_entities_GetHandle(), 1);
    

    if(0 == eo_entities_NumOfMaises(eo_entities_GetHandle()))
    {
        p->service.active = eobool_false;
        return(eores_NOK_generic);
    }
    else
    {
        
        p->mais = eo_entities_GetMais(eo_entities_GetHandle(), 0);
        
        memcpy(&p->service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));
            
        
        // now... use the servcfg
        eObrd_canproperties_t prop = 
        {
            .type               = eobrd_cantype_mais, 
            .location           = { .port = servcfg->data.as.mais.canloc.port, .addr = servcfg->data.as.mais.canloc.addr, .insideindex = eobrd_caninsideindex_none },
            .requiredprotocol   = { .major = servcfg->data.as.mais.version.protocol.major, .minor = servcfg->data.as.mais.version.protocol.minor }
        };       
        eo_vector_PushBack(p->sharedcan.boardproperties, &prop);
        
        // load the can mapping ... make an UnloadBoards()
        eo_canmap_LoadBoards(eo_canmap_GetHandle(), p->sharedcan.boardproperties); 
        
        // load the entity mapping.
        eOcanmap_entitydescriptor_t des = 
        {
            .location   = { .port = servcfg->data.as.mais.canloc.port, .addr = servcfg->data.as.mais.canloc.addr, .insideindex = eobrd_caninsideindex_none },
            .index      = entindex00 // we have only one mais
        };
        eo_vector_PushBack(p->sharedcan.entitydescriptor, &des);
        
        eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_analogsensors, eoprot_entity_as_mais, p->sharedcan.entitydescriptor);   


        p->service.active = eobool_true;    
        p->service.state = eomn_serv_state_activated;   
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);        
    }
    
    return(eores_OK);   
}


extern eOresult_t eo_mais_Start(EOtheMAIS *p)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_mais_Start()", s_eobj_ownname);
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
    
    if(eobool_true == p->service.started)
    {   // it is already running. however we increment number of owners
        p->numberofowners ++;
        return(eores_OK);
    }   
    
    p->numberofowners = 1; // dont increment. just set to 1.
    p->service.started = eobool_true;    
    p->service.state = eomn_serv_state_started; 
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);   
    
    // now we activate tx according to config of mais
    //s_eo_mais_TXstart(p, p->mais->config.datarate, eoas_maismode_txdatacontinuously, (eOas_maisresolution_t)p->mais->config.resolution);   
    
    return(eores_OK);
}


extern eOresult_t eo_mais_SetRegulars(EOtheMAIS *p, eOmn_serv_arrayof_id32_t* arrayofid32, uint8_t* numberofthem)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
    
    return(eo_service_hid_SetRegulars(p->id32ofregulars, arrayofid32, s_eo_mais_isID32relevant, numberofthem));
}


extern eOresult_t eo_mais_Stop(EOtheMAIS *p)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_mais_Stop()", s_eobj_ownname);
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
    
    if(eobool_false == p->service.started)
    {   // it is already stopped
        return(eores_OK);
    }

    if(0 == p->numberofowners)
    {   // actually we should not get in here because of previous check of running equal false
        return(eores_OK);
    }
    
    p->numberofowners --;
    
    if(0 != p->numberofowners)
    {   // there is someone else owning the object ... i dont stop it.
        return(eores_OK);
    }    
     
    s_eo_mais_TXstop(p);
       
    p->service.started = eobool_false;
    p->service.state = eomn_serv_state_activated;
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);       
    
    // remove all regulars related to mais entity ... no, dont do that
    //eo_mais_SetRegulars(p, NULL, NULL);    

    return(eores_OK);
}


extern eOresult_t eo_mais_Tick(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    } 

    if(eobool_false == p->service.started)
    {   // not running, thus we do nothing
        return(eores_OK);
    }     
    
//    if(!eo_canmsg_watchdog_check(p->canmsgwatchdog))
//    {
//        #warning VALE: segnala errore
//    }
    
    return(eores_OK);       
}


extern eOresult_t eo_mais_Transmission(EOtheMAIS *p, eObool_t on)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    } 

    if(eobool_false == p->service.started)
    {   // not running, thus we do nothing
        return(eores_OK);
    }     
    
    
    if(eobool_true == on)
    {   // we activate tx according to config of mais
        s_eo_mais_TXstart(p, p->mais->config.datarate, eoas_maismode_txdatacontinuously, (eOas_maisresolution_t)p->mais->config.resolution); 
    }
    else
    {
        s_eo_mais_TXstop(p);
    }
    
    return(eores_OK);      
}


extern eOresult_t eo_mais_Set(EOtheMAIS *p, eOas_mais_config_t* maiscfg)
{
    if((NULL == p) || (NULL == maiscfg))
    {
        return(eores_NOK_nullpointer);
    }
        
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
  
// we allow doing things also if we are not in running mode yet    
//    if(eobool_false == p->service.started)
//    {   // not running, thus we do nothing
//        return(eores_OK);
//    }     

    // ok, now we do something. 
    
    eOresult_t res = eores_OK;
    
    if(eores_OK != (res = eo_mais_SetDataRate(p, maiscfg->datarate)))
    {
        return(res);
    }   
    
    if(eores_OK != (res = eo_mais_SetResolution(p, (eOas_maisresolution_t)maiscfg->resolution)))
    {
        return(res);
    }
    
    if(eores_OK != (res = eo_mais_SetMode(p, (eOas_maismode_t)maiscfg->mode)))
    {
        return(res);
    }
       
    return(eores_OK);     
}


extern eOresult_t eo_mais_SetMode(EOtheMAIS *p, eOas_maismode_t mode)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  

// we allow doing things also if we are not in running mode yet     
//    if(eobool_false == p->service.started)
//    {   // not running, thus we do nothing
//        return(eores_OK);
//    }     

    // ok, now we do something.
    p->sharedcan.command.class = eocanprot_msgclass_pollingAnalogSensor;
    p->sharedcan.command.type  = ICUBCANPROTO_POL_AS_CMD__SET_TXMODE;
    p->sharedcan.command.value = &mode;
    
    eo_canserv_SendCommandToEntity(eo_canserv_GetHandle(), &p->sharedcan.command, p->id32);
    
    if(eoas_maismode_txdatacontinuously == mode)
    {
        eo_canmsg_watchdog_start(p->canmsgwatchdog);
    }
    else
    {
        eo_canmsg_watchdog_stop(p->canmsgwatchdog);
    }
        
    return(eores_OK);
}


extern eOresult_t eo_mais_SetDataRate(EOtheMAIS *p, uint8_t datarate)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    } 

// we allow doing things also if we are not in running mode yet 
//    if(eobool_false == p->service.started)
//    {   // not running, thus we do nothing
//        return(eores_OK);
//    }     

    // ok, now we do something.     
 
    p->sharedcan.command.class = eocanprot_msgclass_pollingAnalogSensor;
    p->sharedcan.command.type  = ICUBCANPROTO_POL_AS_CMD__SET_CANDATARATE;
    p->sharedcan.command.value = &datarate;
    
    eo_canserv_SendCommandToEntity(eo_canserv_GetHandle(), &p->sharedcan.command, p->id32);    
    
    
    eOcanmsg_watchdog_cfg_t cfg = {.period = datarate};
    eo_canmsg_watchdog_updateconfig(p->canmsgwatchdog, &cfg);
    
    return(eores_OK);  
}



extern eOresult_t eo_mais_SetResolution(EOtheMAIS *p, eOas_maisresolution_t resolution)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  

// we allow doing things also if we are not in running mode yet     
//    if(eobool_false == p->service.started)
//    {   // not running, thus we do nothing
//        return(eores_OK);
//    }     

    // ok, now we do something.     
 
    p->sharedcan.command.class = eocanprot_msgclass_pollingAnalogSensor;
    p->sharedcan.command.type  = ICUBCANPROTO_POL_AS_CMD__SET_RESOLUTION;
    p->sharedcan.command.value = &resolution;
    
    eo_canserv_SendCommandToEntity(eo_canserv_GetHandle(), &p->sharedcan.command, p->id32);    
    
    eOas_mais_status_t *status = eo_entities_GetMaisStatus(eo_entities_GetHandle(), 0);
    if(NULL == status)
    {
        return(eores_NOK_generic); //error
    }     

    s_eo_mais_process_mais_resolution(resolution, status);  
    
    return(eores_OK);          
}


extern eOresult_t eo_mais_notifymeOnNewReceivedData(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
    
    eo_canmsg_watchdog_rearm(p->canmsgwatchdog);

    return(eores_OK);
}

extern eObool_t eo_mais_isAlive(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(eobool_false);
    }

    return(eo_canmsg_watchdog_check(p->canmsgwatchdog));
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// -- in here .... so that all things related to MAIS are in a unique place


extern void eoprot_fun_INIT_as_mais_config(const EOnv* nv)
{
    eOas_mais_config_t* maiscfg = eo_nv_RAM(nv);
    
    maiscfg->datarate = 10;
    maiscfg->mode = eoas_maismode_txdatacontinuously;
    maiscfg->resolution = eoas_maisresolution_08;
}


extern void eoprot_fun_INIT_as_mais_status(const EOnv* nv)
{
    eOas_mais_status_t *status = eo_nv_RAM(nv);  
    
    // marco.accame: i init as for eoas_maisresolution_08 
    //               the array the15values can be initted for size 0 or 15 as i now use teh proper eo_array_Assign() method
    uint8_t capacity    = 15;
    uint8_t itemsize    = 1;
    uint8_t size        = 15;
    EOarray* array = eo_array_New(capacity, itemsize, &status->the15values);
    eo_array_Resize(array, size);
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

static eOresult_t s_eo_mais_TXstart(EOtheMAIS *p, uint8_t datarate, eOas_maismode_t mode, eOas_maisresolution_t resolution)
{   
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  

    if(eobool_false == p->service.started)
    {   // not running, thus we do nothing
        return(eores_OK);
    }     
    
    eOas_mais_config_t config = {0};
    config.datarate = datarate;
    config.mode = mode;
    config.resolution = resolution;
    
    return(eo_mais_Set(p, &config));      
}


static eOresult_t s_eo_mais_TXstop(EOtheMAIS *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }   
    
    if(eobool_false == p->service.started)
    {   // not running, thus we do nothing
        return(eores_OK);
    }     
    
    // marco.accame on 28apr16: i prefer to NEVER stop the tx of the mais can board to avoid the mc4 can board may go in hw fault
    // eo_mais_SetMode(p, eoas_maismode_acquirebutdonttx);
 
    return(eores_OK);
}


static void s_eo_mais_process_mais_resolution(eOas_maisresolution_t resolution, eOas_mais_status_t* status)
{
    uint8_t size = 0;
    uint8_t itemsize = 1;
    uint8_t capacity = 0;
    
    if(eoas_maisresolution_08 == resolution)
    {
        capacity    = 15;
        itemsize    = 1;
        size        = 15;
    }
    else if(eoas_maisresolution_16 == resolution)
    {
        capacity    = 15;
        itemsize    = 2;
        size        = 15;
    } 

    EOarray* array = eo_array_New(capacity, itemsize, &status->the15values);
    eo_array_Resize(array, size);      
}


static eOresult_t s_eo_mais_onstop_search4mais(void *par, EOtheCANdiscovery2* cd2, eObool_t searchisok)
{  
    EOtheMAIS* p = &s_eo_themais;
    
    const eOmn_serv_configuration_t * servcfg = (const eOmn_serv_configuration_t *)par;
    
    if(eobool_true == searchisok)
    {
        p->service.state = eomn_serv_state_verified;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);   
    }
    else
    {   
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mais, p->service.state);   
    }    
    
    if((eobool_true == searchisok) && (eobool_true == p->service.activateafterverify))
    {
        eo_mais_Activate(&s_eo_themais, servcfg);        
    }
    
    p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.sourceaddress    = 0;
    p->diagnostics.errorDescriptor.par16            = (servcfg->data.as.mais.canloc.addr) | (servcfg->data.as.mais.canloc.port << 8);
    p->diagnostics.errorDescriptor.par64            = (servcfg->data.as.mais.version.firmware.minor << 0) | (servcfg->data.as.mais.version.firmware.major << 8) |
                                                      (servcfg->data.as.mais.version.protocol.minor << 16) | (servcfg->data.as.mais.version.protocol.major << 24);    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_mais_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == searchisok)
    {           
        p->diagnostics.errorType = eo_errortype_debug;
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mais_ok);
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if((0 != p->diagnostics.repetitionOKcase) && (0 != p->diagnostics.reportPeriod))
        {
            p->diagnostics.errorCallbackCount = p->diagnostics.repetitionOKcase;        
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }    
    
    if(eobool_false == searchisok)
    {
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mais_failed_candiscovery);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);  
        }
    }      
    
    if(NULL != p->service.onverify)
    {
        p->service.onverify(&s_eo_themais, searchisok); 
    }    
    
    return(eores_OK);   
}


static void s_eo_mais_send_periodic_error_report(void *par)
{
    EOtheMAIS* p = (EOtheMAIS*) par;
    
    eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
    
    if(eoerror_value_CFG_mais_failed_candiscovery == eoerror_code2value(p->diagnostics.errorDescriptor.code))
    {   // if i dont find the mais, i keep on sending the discovery results up. it is a temporary diagnostics tricks until we use the verification of services at bootstrap
        eo_candiscovery2_SendLatestSearchResults(eo_candiscovery2_GetHandle());
    }
    
    if(EOK_int08dummy != p->diagnostics.errorCallbackCount)
    {
        p->diagnostics.errorCallbackCount--;
    }
    if(0 == p->diagnostics.errorCallbackCount)
    {
        eo_timer_Stop(p->diagnostics.reportTimer);
    }
}


static eObool_t s_eo_mais_isID32relevant(uint32_t id32)
{
    static const uint32_t mask0 = (((uint32_t)eoprot_endpoint_analogsensors) << 24) | (((uint32_t)eoprot_entity_as_mais) << 16);
    
    if((id32 & mask0) == mask0)
    {
        return(eobool_true);
    }
    
    return(eobool_false); 
}

// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



