#include <stdio.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/drv/emac/emac_drv.h>
#include <ti/drv/emac/src/v4/emac_drv_v4.h>
#include <ti/starterware/include/types.h>
#include <ti/starterware/include/hw/hw_types.h>
#include <ti/starterware/include/hw/hw_control_am335x.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/starterware/include/ethernet.h>
#include <ti/starterware/include/soc_control.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/i2c/I2C.h>
#include <ti/drv/i2c/soc/I2C_v1.h>
#include <ti/csl/soc.h>
#include <ti/drv/i2c/soc/I2C_soc.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_v1.h>
#include <ti/csl/csl_utils.h>
#include <ti/csl/csl_types.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>
#include "network/network.h"

//----------------------------------------------------------------------------
// GPIO configuration
//----------------------------------------------------------------------------

/* Number of GPIO ports */
#define CSL_GPIO_PER_CNT    4U

/* GPIO Driver hardware attributes */
GPIO_v1_hwAttrs_list GPIO_v1_hwAttrs = {
    {
       SOC_GPIO_0_REGS,
       96,
       97,
       0,
       0
    },
    {
       SOC_GPIO_1_REGS,
       98,
       99,
       0,
       0
    },
    {
       SOC_GPIO_2_REGS,
       32,
       33,
       0,
       0
    },
    {
       SOC_GPIO_3_REGS,
       62,
       63,
       0,
       0
    },
    /* "pad to full predefined length of array" */
    {   0,0,0,0,0   },
    {   0,0,0,0,0   },
    {   0,0,0,0,0   },
    {   0,0,0,0,0   },
};

/* GPIO configuration structure */
CSL_PUBLIC_CONST GPIOConfigList GPIO_config =
{
    {
        &GPIO_FxnTable_v1,
        NULL,
        NULL
    },
    /* "pad to full predefined length of array" */
    {
        NULL,
        NULL,
        NULL
    },
    {
        NULL,
        NULL,
        NULL
    }
};

/* GPIO pin value definitions */
#define GPIO_PIN_VAL_LOW     (0U)
#define GPIO_PIN_VAL_HIGH    (1U)

/* Port and pin number mask for GPIO Load pin.
   Bits 7-0: Pin number  and Bits 15-8: (Port number + 1) */
#define GPIO_PR1_MII_CTRL_PIN_NUM       (0x04)
#define GPIO_MUX_MII_CTRL_PIN_NUM       (0x0A)
#define GPIO_FET_SWITCH_CTRL_PIN_NUM    (0x07)
#define GPIO_DDR_VTT_EN_PIN_NUM         (0x12)
#define GPIO_PHY_0_1_RST_PIN_NUM        (0x05)
#define GPIO_PR1_MII_CTRL_PORT_NUM      (0x03)
#define GPIO_MUX_MII_CTRL_PORT_NUM      (0x03)
#define GPIO_FET_SWITCH_CTRL_PORT_NUM   (0x00)
#define GPIO_DDR_VTT_EN_PORT_NUM        (0x00)
#define GPIO_PHY_0_1_RST_PORT_NUM       (0x02)

/* ON Board LED pins which are connected to GPIO pins. */
typedef enum GPIO_PIN {
    GPIO_PIN_PR1_MII_CTRL      = 0U,
    GPIO_PIN_MUX_MII_CTRL      = 1U,
    GPIO_PIN_FET_SWITCH_CTRL   = 2U,
    GPIO_PIN_DDR_VTT_EN        = 3U,
    GPIO_PIN_PHY_0_1_RST       = 4U,
    GPIO_PIN_COUNT
}GPIO_PIN;

/* GPIO Driver board specific pin configuration structure */
GPIO_PinConfig gpioPinConfigs[] = {
    /* Output pin : AM335X_ICE V2_LD_PIN */
    GPIO_DEVICE_CONFIG((GPIO_PR1_MII_CTRL_PORT_NUM + 1), GPIO_PR1_MII_CTRL_PIN_NUM)      | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_MUX_MII_CTRL_PORT_NUM + 1), GPIO_MUX_MII_CTRL_PIN_NUM)      | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_FET_SWITCH_CTRL_PORT_NUM + 1), GPIO_FET_SWITCH_CTRL_PIN_NUM)| GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_DDR_VTT_EN_PORT_NUM + 1), GPIO_DDR_VTT_EN_PIN_NUM)          | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG((GPIO_PHY_0_1_RST_PORT_NUM + 1), GPIO_PHY_0_1_RST_PIN_NUM)        | GPIO_CFG_OUTPUT,
};

/* GPIO Driver call back functions */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/* GPIO Driver configuration structure */
GPIO_v1_Config GPIO_v1_config = {
    gpioPinConfigs,
    gpioCallbackFunctions,
    sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    sizeof(gpioCallbackFunctions) / sizeof(GPIO_CallbackFxn),
    0,
};

//----------------------------------------------------------------------------
// I2C configuration
//----------------------------------------------------------------------------

/* I2C configuration structure */
I2C_HwAttrs i2cInitCfg[I2C_HWIP_MAX_CNT] =
{
    {
       SOC_I2C_0_REGS,
        70,
        0,
        48000000U,
        true,
        {
            /* default own slave addresses */
            0x70, 0x0, 0x0, 0x0
        },
    },
    {
        SOC_I2C_1_REGS,
        71,
        0,
        48000000U,
        true,
        {
            0x71, 0x0, 0x0, 0x0
        },
    },
    {
        SOC_I2C_2_REGS,
        30,
        0,
        48000000U,
        true,
        {
            0x72, 0x0, 0x0, 0x0
        },
    }
};

/* I2C objects */
I2C_v1_Object I2cObjects[I2C_HWIP_MAX_CNT];

/* I2C configuration structure */
I2C_config_list I2C_config = {
    {
        &I2C_v1_FxnTable,
        &I2cObjects[0],
        &i2cInitCfg[0]
    },

    {
         &I2C_v1_FxnTable,
         &I2cObjects[1],
         &i2cInitCfg[1]
    },

    {
         &I2C_v1_FxnTable,
         &I2cObjects[2],
         &i2cInitCfg[2]
    },
    /*"pad to full predefined length of array"*/
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL}
};

/* Macro indicating the i2c time out value. */
#define I2C_TIMEOUT_VAL           (100U)

/* I2C Instance Controlling Clock Synthesizer */
#define CLOCK_SYNTHESIZER_I2C_INST_NUM  0

/* Clock Synthesizer Device Address */
#define CLOCK_SYNTHESIZER_I2C_ADDR      0x65

#define CLOCK_SYNTHESIZER_ID_REG        0

/* Crystal load capacitor selection */
#define CLOCK_SYNTHESIZER_XCSEL         0x05

/* PLL1 Configuration Register */
#define CLOCK_SYNTHESIZER_MUX_REG       0x14

/* PDIV2 */
#define CLOCK_SYNTHESIZER_PDIV2_REG     0x16

/* PDIV3 */
#define CLOCK_SYNTHESIZER_PDIV3_REG     0x17

#define TX_LENGTH              (2U)
#define RX_LENGTH              (10U)

/* Enable the below macro to have prints on the IO Console */
//#define IO_CONSOLE
#ifndef IO_CONSOLE
#define NIMU_log                UART_printf
#else
#define NIMU_log                printf
#endif

//----------------------------------------------------------------------------

#define NIMU_MAX_DEVICETABLE_ENTRIES   3
/**Phy address of the CPSW port 1*/
#define EMAC_CPSW_PORT0_PHY_ADDR_ICE2    1
/**Phy address of the CPSW port 1*/
#define EMAC_CPSW_PORT1_PHY_ADDR_ICE2    3

extern int CpswEmacInit (STKEVENT_Handle hEvent);

static void delay(unsigned int delayValue);

static void PhySetupAndReset(void);

static uint32_t ClockSynthesizerSetup(void);

NIMU_DEVICE_TABLE_ENTRY NIMUDeviceTable[NIMU_MAX_DEVICETABLE_ENTRIES];

static int nimu_device_index = 0U;

//----------------------------------------------------------------------------
int32_t networkInit()
{
    ClockSynthesizerSetup();

    GPIO_init();

    PhySetupAndReset();

    EMAC_HwAttrs_V4 cfg;

    SOCCtrlCpswPortMacModeSelect(1, ETHERNET_MAC_TYPE_RMII);
    SOCCtrlCpswPortMacModeSelect(2, ETHERNET_MAC_TYPE_RMII);

    EMAC_socGetInitCfg(0, &cfg);
    cfg.port[0].phy_addr = EMAC_CPSW_PORT0_PHY_ADDR_ICE2;
    cfg.port[1].phy_addr = EMAC_CPSW_PORT1_PHY_ADDR_ICE2;
    cfg.macModeFlags = EMAC_CPSW_CONFIG_MODEFLG_FULLDUPLEX | EMAC_CPSW_CONFIG_MODEFLG_IFCTLA;
    EMAC_socSetInitCfg(0, &cfg);

    NIMUDeviceTable[nimu_device_index++].init =  &CpswEmacInit ;
    NIMUDeviceTable[nimu_device_index].init =  NULL ;

    return 0;
}

//----------------------------------------------------------------------------
uint32_t ClockSynthesizerSetup(void)
{
    uint32_t status = TRUE;
    uint32_t regValue = 0U;
    I2C_Params i2cParams;
    I2C_Handle handle = NULL;
    I2C_Transaction i2cTransaction;
    char txBuf[TX_LENGTH] = {0x00, 0x01};
    char rxBuf[RX_LENGTH] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00};

    I2C_init();

    I2C_Params_init(&i2cParams);

    ((I2C_HwAttrs *) I2C_config[CLOCK_SYNTHESIZER_I2C_INST_NUM].hwAttrs)->enableIntr = false;
    handle = I2C_open(CLOCK_SYNTHESIZER_I2C_INST_NUM, &i2cParams);

    /* Initializing I2C transaction params to default value */
    I2C_transactionInit(&i2cTransaction);

/* Read - CLOCK_SYNTHESIZER_ID_REG */

    if (FALSE != status)
    {
        txBuf[0] = CLOCK_SYNTHESIZER_ID_REG | 0x80U;
        i2cTransaction.slaveAddress = CLOCK_SYNTHESIZER_I2C_ADDR;
        i2cTransaction.writeBuf = (uint8_t *)&txBuf[0];
        i2cTransaction.writeCount = 1U;
        i2cTransaction.readBuf = (uint8_t *)&rxBuf[0];
        i2cTransaction.readCount = 0U;
        status = I2C_transfer(handle, &i2cTransaction);

        if(FALSE == status)
        {
            NIMU_log("\n CLOCK_SYNTHESIZER_ID_REG: Data Write failed. \n");
        }
    }

    delay(I2C_TIMEOUT_VAL);

    if (FALSE != status)
    {
        i2cTransaction.slaveAddress = CLOCK_SYNTHESIZER_I2C_ADDR;
        i2cTransaction.writeBuf = (uint8_t *)&txBuf[0];
        i2cTransaction.writeCount = 0U;
        i2cTransaction.readBuf = (uint8_t *)&rxBuf[0];
        i2cTransaction.readCount = 1U;
        status = I2C_transfer(handle, &i2cTransaction);

        if(FALSE == status)
        {
            NIMU_log("\n CLOCK_SYNTHESIZER_ID_REG: Data Read failed. \n");
        }
        else
        {
            regValue = rxBuf[0];
        }
    }

    delay(I2C_TIMEOUT_VAL);

    if((regValue & 0x81U)!= 0x81U)
    {
        NIMU_log("\n Clock synthesizer: Read: Failed");
        status = FALSE;
    }

    /** CDCE913 Clock Synthesizer configuration for RMII Clock = 50 MHz
     * fout = fin/Pdiv x N/M
     * fout = 50 MHz
     * fin  = 25 MHz
     * Pdiv = 2
     * N    = 4
     * M    = 1
     */

    /* Crystal load Capacitor Selection - 18pF: 0x12h(bits 7:3) */

/* Write - CLOCK_SYNTHESIZER_XCSEL */

    if (FALSE != status)
    {
        txBuf[0] = CLOCK_SYNTHESIZER_XCSEL | 0x80U;
        txBuf[1] = 0x90U;
        i2cTransaction.slaveAddress = CLOCK_SYNTHESIZER_I2C_ADDR;
        i2cTransaction.writeBuf = (uint8_t *)&txBuf[0];
        i2cTransaction.writeCount = 2U;
        i2cTransaction.readBuf = (uint8_t *)&rxBuf[0];
        i2cTransaction.readCount = 0U;
        status = I2C_transfer(handle, &i2cTransaction);

        if(FALSE == status)
        {
            NIMU_log("\n CLOCK_SYNTHESIZER_XCSEL: Data Write failed. \n");
        }
    }

    delay(I2C_TIMEOUT_VAL);

    if (FALSE == status)
    {
        NIMU_log("\n Clock synthesizer: Write: Failed");
    }

    /* PLL1 Multiplexer b7:0 (PLL1) */

/* Write - CLOCK_SYNTHESIZER_MUX_REG */

    if (FALSE != status)
    {
        txBuf[0] = CLOCK_SYNTHESIZER_MUX_REG | 0x80U;
        txBuf[1] = 0x6DU;
        i2cTransaction.slaveAddress = CLOCK_SYNTHESIZER_I2C_ADDR;
        i2cTransaction.writeBuf = (uint8_t *)&txBuf[0];
        i2cTransaction.writeCount = 2U;
        i2cTransaction.readBuf = (uint8_t *)&rxBuf[0];
        i2cTransaction.readCount = 0U;
        status = I2C_transfer(handle, &i2cTransaction);

        if(FALSE == status)
        {
            NIMU_log("\n CLOCK_SYNTHESIZER_MUX_REG: Data Write failed. \n");
        }
    }

    delay(I2C_TIMEOUT_VAL);

    if (FALSE == status)
    {
        NIMU_log("\n Clock synthesizer: Write: Failed");
    }

    /** b7-0(PLL1 SSC down selection by default),
     *  b6:0-0x02h(7-bit Y2-Output-Divider Pdiv2)
     */

/* Write - CLOCK_SYNTHESIZER_PDIV2_REG */

    if (FALSE != status)
    {
        txBuf[0] = CLOCK_SYNTHESIZER_PDIV2_REG | 0x80U;
        txBuf[1] = 0x02U;
        i2cTransaction.slaveAddress = CLOCK_SYNTHESIZER_I2C_ADDR;
        i2cTransaction.writeBuf = (uint8_t *)&txBuf[0];
        i2cTransaction.writeCount = 2U;
        i2cTransaction.readBuf = (uint8_t *)&rxBuf[0];
        i2cTransaction.readCount = 0U;
        status = I2C_transfer(handle, &i2cTransaction);

        if(FALSE == status)
        {
            NIMU_log("\n CLOCK_SYNTHESIZER_PDIV2_REG: Data Write failed. \n");
        }
    }

    delay(I2C_TIMEOUT_VAL);

    if (FALSE == status)
    {
        NIMU_log("\n Clock synthesizer: Write: Failed");
    }

    /* b6:0-0x02h(7-bit Y3-Output-Divider Pdiv3) */

/* Write - CLOCK_SYNTHESIZER_PDIV3_REG */

    if (FALSE != status)
    {
        txBuf[0] = CLOCK_SYNTHESIZER_PDIV3_REG | 0x80U;
        txBuf[1] = 0x02U;
        i2cTransaction.slaveAddress = CLOCK_SYNTHESIZER_I2C_ADDR;
        i2cTransaction.writeBuf = (uint8_t *)&txBuf[0];
        i2cTransaction.writeCount = 2U;
        i2cTransaction.readBuf = (uint8_t *)&rxBuf[0];
        i2cTransaction.readCount = 0U;
        status = I2C_transfer(handle, &i2cTransaction);

        if(FALSE == status)
        {
            NIMU_log("\n CLOCK_SYNTHESIZER_PDIV3_REG: Data Write failed. \n");
        }
    }

    delay(I2C_TIMEOUT_VAL);

    if (FALSE == status)
    {
        NIMU_log("\n Clock synthesizer: Write: Failed");
    }

    I2C_close(handle);

    return status;
}

//----------------------------------------------------------------------------
void PhySetupAndReset(void)
{
    /* PR1_MII_CTL */
    GPIO_write(GPIO_PIN_PR1_MII_CTRL, GPIO_PIN_VAL_HIGH);

    /* MUX MII CONTROL */
    GPIO_write(GPIO_PIN_MUX_MII_CTRL, GPIO_PIN_VAL_HIGH);

    /* FET SWITCH CONTROL */
    GPIO_write(GPIO_PIN_FET_SWITCH_CTRL, GPIO_PIN_VAL_HIGH);

    /* DDR VTT ENABLE */
    GPIO_write(GPIO_PIN_DDR_VTT_EN, GPIO_PIN_VAL_HIGH);

    /* Phy 0 & 1 reset */
    GPIO_write(GPIO_PIN_PHY_0_1_RST, GPIO_PIN_VAL_LOW);
    delay(100);
    GPIO_write(GPIO_PIN_PHY_0_1_RST, GPIO_PIN_VAL_HIGH);
    delay(100);
}

//----------------------------------------------------------------------------
void delay(unsigned int delayValue)
{
    volatile uint32_t delay1 = delayValue*10000;
    while (delay1--) ;
}
