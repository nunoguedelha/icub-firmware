/*
 * Copyright (C) 2017 iCub Facility - Istituto Italiano di Tecnologia
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


// - include guard ----------------------------------------------------------------------------------------------------
#ifndef _STM32HAL_BOARD_H_
#define _STM32HAL_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif


// - external dependencies --------------------------------------------------------------------------------------------  
    
#include "stm32hal_define.h"
#include "stm32hal_driver.h"
    
// - public #define  --------------------------------------------------------------------------------------------------
// empty-section
  

// - declaration of public user-defined types -------------------------------------------------------------------------    
// empty-section
    
    
// - declaration of extern public functions ---------------------------------------------------------------------------
 
// called by stm32hal_init() if stm32hal_config_t::initbsp is true
    
extern void stm32hal_board_init(void);     
    
// - public interface: begin  -----------------------------------------------------------------------------------------  
// it contains whatever cube-mx generates.


#if     defined(STM32HAL_BOARD_NUCLEO64)

    // one version only: v172
    #if (STM32HAL_DRIVER_VERSION == 0x172)

        #include "../src/config/stm32hal_driver_cfg_of_nucleo64.h"

        #include "../src/board/nucleo64/v172/inc/gpio.h"
        #include "../src/board/nucleo64//v172/inc/main.h"
        #include "../src/board/nucleo64/v172/inc/usart.h"

    #else
        #error unsupported driver version for nucleo64
    #endif
    
#elif   defined(STM32HAL_BOARD_MTB4)

    // one version only: v172
    #if (STM32HAL_DRIVER_VERSION == 0x172)

        #include "../src/config/stm32hal_driver_cfg_of_mtb4.h"

        #include "../src/board/mtb4/v172/inc/adc.h"
        #include "../src/board/mtb4/v172/inc/can.h"
        #include "../src/board/mtb4/v172/inc/dma.h"
        #include "../src/board/mtb4/v172/inc/gpio.h"
        #include "../src/board/mtb4/v172/inc/i2c.h"
        #include "../src/board/mtb4/v172/inc/rng.h"
        #include "../src/board/mtb4/v172/inc/main.h"
        #include "../src/board/mtb4/v172/inc/tim.h"
        #include "../src/board/mtb4/v172/inc/usart.h"


        #include "../src/board/mtb4/v172/inc/stm32l4xx_it.h"
        
    #elif(STM32HAL_DRIVER_VERSION == 0x190)

        #include "../src/config/stm32hal_driver_cfg_of_mtb4_v190.h"

        #include "../src/board/mtb4/v190/inc/adc.h"
        #include "../src/board/mtb4/v190/inc/can.h"
        #include "../src/board/mtb4/v190/inc/dma.h"
        #include "../src/board/mtb4/v190/inc/gpio.h"
        #include "../src/board/mtb4/v190/inc/i2c.h"
        #include "../src/board/mtb4/v190/inc/rng.h"
        #include "../src/board/mtb4/v190/inc/main.h"
        #include "../src/board/mtb4/v190/inc/tim.h"
        #include "../src/board/mtb4/v190/inc/usart.h"

        #include "../src/board/mtb4/v190/inc/stm32l4xx_it.h"


    #else
        #error unsupported driver version for mtb4
    #endif

#elif   defined(STM32HAL_BOARD_STRAIN2)	

    // three cases: v172, v190, v1B0
    #if (STM32HAL_DRIVER_VERSION == 0x172)

        #include "../src/config/stm32hal_driver_cfg_of_strain2.h"

        #include "../src/board/strain2/v172/inc/adc.h"
        #include "../src/board/strain2/v172/inc/can.h"
        #include "../src/board/strain2/v172/inc/dma.h"
        #include "../src/board/strain2/v172/inc/gpio.h"
        #include "../src/board/strain2/v172/inc/i2c.h"
        #include "../src/board/strain2/v172/inc/main.h"
        #include "../src/board/strain2/v172/inc/rng.h"
        #include "../src/board/strain2/v172/inc/tim.h"
        #include "../src/board/strain2/v172/inc/usart.h"

        #include "../src/board/strain2/v172/inc/stm32l4xx_it.h"

    #elif(STM32HAL_DRIVER_VERSION == 0x190)

        #include "../src/config/stm32hal_driver_cfg_of_strain2_v190.h"

        #include "../src/board/strain2/v190/inc/adc.h"
        #include "../src/board/strain2/v190/inc/can.h"
        #include "../src/board/strain2/v190/inc/dma.h"
        #include "../src/board/strain2/v190/inc/gpio.h"
        #include "../src/board/strain2/v190/inc/i2c.h"
        #include "../src/board/strain2/v190/inc/main.h"
        #include "../src/board/strain2/v190/inc/rng.h"
        #include "../src/board/strain2/v190/inc/tim.h"
        #include "../src/board/strain2/v190/inc/usart.h"

        #include "../src/board/strain2/v190/inc/stm32l4xx_it.h"

    #elif(STM32HAL_DRIVER_VERSION == 0x1B0)

				#if !defined(CPU_AT_80MHZ)
				#define CPU_AT_80MHZ
				#endif
				
        #include "../src/config/stm32hal_driver_cfg_of_strain2_v1B0.h"

        #include "../src/board/strain2/v1B0/inc/adc.h"
        #include "../src/board/strain2/v1B0/inc/can.h"
        #include "../src/board/strain2/v1B0/inc/dma.h"
        #include "../src/board/strain2/v1B0/inc/gpio.h"
        #include "../src/board/strain2/v1B0/inc/i2c.h"
        #include "../src/board/strain2/v1B0/inc/main.h"
        #include "../src/board/strain2/v1B0/inc/rng.h"
        #include "../src/board/strain2/v1B0/inc/tim.h"
        #include "../src/board/strain2/v1B0/inc/usart.h"

        #include "../src/board/strain2/v1B0/inc/stm32l4xx_it.h"

    #else
        #error unsupported driver version for strain2
    #endif


#elif   defined(STM32HAL_BOARD_PSC)	

// one case only: v190
    #if (STM32HAL_DRIVER_VERSION == 0x190)

        #include "../src/config/stm32hal_driver_cfg_of_psc_v190.h"
        
        #include "../src/board/psc/v190/inc/main.h"

        #include "../src/board/psc/v190/inc/can.h"
        #include "../src/board/psc/v190/inc/dma.h"
        #include "../src/board/psc/v190/inc/gpio.h"
        #include "../src/board/psc/v190/inc/i2c.h"        
        #include "../src/board/psc/v190/inc/rng.h"
        #include "../src/board/psc/v190/inc/tim.h"

        #include "../src/board/psc/v190/inc/stm32l4xx_it.h"

    #else
        #error unsupported driver version for psc
    #endif
    
#elif   defined(STM32HAL_BOARD_RFE) 

    // one case only: v183
    #if (STM32HAL_DRIVER_VERSION == 0x183)

        #include "../src/config/stm32hal_driver_cfg_of_rfe.h"

        #include "../src/board/rfe/v183/inc/adc.h"
        #include "../src/board/rfe/v183/inc/can.h"
        #include "../src/board/rfe/v183/inc/dma.h"
        #include "../src/board/rfe/v183/inc/gpio.h"
        #include "../src/board/rfe/v183/inc/i2c.h"
        #include "../src/board/rfe/v183/inc/main.h"
        #include "../src/board/rfe/v183/inc/spi.h"
        #include "../src/board/rfe/v183/inc/rng.h"
        #include "../src/board/rfe/v183/inc/tim.h"

        #include "../src/board/rfe/v183/inc/stm32l4xx_it.h"

    #else
        #error unsupported driver version for rfe
    #endif

#elif   defined(STM32HAL_BOARD_SG3) 

    // one case only: v190
    #if (STM32HAL_DRIVER_VERSION == 0x190)

        #include "../src/config/stm32hal_driver_cfg_of_sg3_v190.h"

        #include "../src/board/sg3/v190/inc/adc.h"
        #include "../src/board/sg3/v190/inc/can.h"
        #include "../src/board/sg3/v190/inc/dma.h"
        #include "../src/board/sg3/v190/inc/gpio.h"
        #include "../src/board/sg3/v190/inc/i2c.h"
        #include "../src/board/sg3/v190/inc/main.h"
        #include "../src/board/sg3/v190/inc/tim.h"

        #include "../src/board/sg3/v190/inc/stm32l4xx_it.h"

    #else
        #error unsupported driver version for sg3
    #endif

#elif   defined(STM32HAL_BOARD_NUCLEOH7) 

    // one case only: v180
    #if (STM32HAL_DRIVER_VERSION == 0x180)

        #include "../src/config/stm32hal_driver_cfg_of_nucleoh7_v180.h"

        #include "../src/board/nucleoh7/v180/inc/dma.h"
        #include "../src/board/nucleoh7/v180/inc/gpio.h"
        #include "../src/board/nucleoh7/v180/inc/i2c.h"
        #include "../src/board/nucleoh7/v180/inc/main.h"
        #include "../src/board/nucleoh7/v180/inc/usb_otg.h"
        #include "../src/board/nucleoh7/v180/inc/usart.h"

        #include "../src/board/nucleoh7/v180/inc/stm32h7xx_it.h"

    #else
        #error unsupported driver version for nucleoh7
    #endif
    
#else
    #error STM32HAL: you must define a STM32HAL_BOARD_${BRD}
#endif


// - public interface: end --------------------------------------------------------------------------------------------


#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 


#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------
