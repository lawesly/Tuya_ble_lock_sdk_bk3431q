/**
 * \file tuya_ble_storage.c
 *
 * \brief
 */
/*
 *  Copyright (C) 2014-2019, Tuya Inc., All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of tuya ble sdk
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_type.h"
#include "tuya_ble_main.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_internal_config.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_utils.h"

//static uint8_t init=0;
static tuya_ble_auth_settings_t flash_settings_auth;
static tuya_ble_auth_settings_t flash_settings_auth_backup;
static tuya_ble_sys_settings_t flash_settings_sys;
static tuya_ble_sys_settings_t flash_settings_sys_backup;


#if (!TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

tuya_ble_status_t tuya_ble_nv_init(void)
{
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_nv_erase(uint32_t addr,uint32_t size)
{
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_nv_write(uint32_t addr,const uint8_t *p_data, uint32_t size)
{
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_nv_read(uint32_t addr,uint8_t *p_data, uint32_t size)
{
    return TUYA_BLE_SUCCESS;
}

#endif

static bool buffer_value_is_all_x(uint8_t *buffer,uint16_t len,uint8_t value)
{
    bool ret = true;
    for(uint16_t i = 0; i<len; i++)
    {
        if(buffer[i]!= value)
        {
            ret = false;
            break;
        }
    }
    return ret;
}



static uint32_t auth_settings_crc_get(tuya_ble_auth_settings_t const * p_settings)
{
    // The crc is calculated from the s_dfu_settings struct, except the crc itself and the init command
    return tuya_ble_crc32_compute((uint8_t*)(p_settings) + 4, sizeof(tuya_ble_auth_settings_t) - 4, NULL);
}

static bool auth_settings_crc_ok(tuya_ble_auth_settings_t const * p_settings)
{
    if (p_settings->crc != 0xFFFFFFFF)
    {
        // CRC is set. Content must be valid
        uint32_t crc = auth_settings_crc_get(p_settings);
        if (crc == p_settings->crc)
        {
            return true;
        }
    }
    return false;
}


static uint32_t sys_settings_crc_get(tuya_ble_sys_settings_t const * p_settings)
{
    // The crc is calculated from the s_dfu_settings struct, except the crc itself and the init command
    return tuya_ble_crc32_compute((uint8_t*)(p_settings) + 4, sizeof(tuya_ble_sys_settings_t) - 4, NULL);
}



static bool sys_settings_crc_ok(tuya_ble_sys_settings_t const * p_settings)
{
    if (p_settings->crc != 0xFFFFFFFF)
    {
        // CRC is set. Content must be valid
        uint32_t crc = sys_settings_crc_get(p_settings);
        if (crc == p_settings->crc)
        {
            return true;
        }
    }
    return false;
}


uint32_t tuya_ble_storage_load_settings(void)
{
    //uint8_t id_null[AUTH_KEY_LEN] = {0};
    uint32_t err_code = 0;
    bool settings_valid ;
    bool settings_backup_valid;

    tuya_ble_nv_read(TUYA_BLE_AUTH_FLASH_ADDR,(uint8_t *)&flash_settings_auth,sizeof(tuya_ble_auth_settings_t));
    tuya_ble_nv_read(TUYA_BLE_AUTH_FLASH_BACKUP_ADDR,(uint8_t *)&flash_settings_auth_backup,sizeof(tuya_ble_auth_settings_t));

    settings_valid = auth_settings_crc_ok(&flash_settings_auth);
    settings_backup_valid = auth_settings_crc_ok(&flash_settings_auth_backup);

    if(settings_valid)
    {
        memcpy(&tuya_ble_current_para.auth_settings,&flash_settings_auth,sizeof(tuya_ble_auth_settings_t));
    }
    else if(settings_backup_valid)
    {
        memcpy(&tuya_ble_current_para.auth_settings,&flash_settings_auth_backup,sizeof(tuya_ble_auth_settings_t));
    }
    else
    {
        memset(&tuya_ble_current_para.auth_settings,0,sizeof(tuya_ble_auth_settings_t));
    }

    tuya_ble_nv_read(TUYA_BLE_SYS_FLASH_ADDR,(uint8_t *)&flash_settings_sys,sizeof(tuya_ble_sys_settings_t));
    tuya_ble_nv_read(TUYA_BLE_SYS_FLASH_BACKUP_ADDR,(uint8_t *)&flash_settings_sys_backup,sizeof(tuya_ble_sys_settings_t));

    settings_valid = sys_settings_crc_ok(&flash_settings_sys);
    settings_backup_valid = sys_settings_crc_ok(&flash_settings_sys_backup);

    if(settings_valid)
    {
        memcpy(&tuya_ble_current_para.sys_settings,&flash_settings_sys,sizeof(tuya_ble_sys_settings_t));
    }
    else if(settings_backup_valid)
    {
        memcpy(&tuya_ble_current_para.sys_settings,&flash_settings_sys_backup,sizeof(tuya_ble_sys_settings_t));
    }
    else
    {
        memset(&tuya_ble_current_para.sys_settings,0,sizeof(tuya_ble_sys_settings_t));
    }

    tuya_ble_current_para.pid_type = tuya_ble_current_para.sys_settings.pid_type;
    tuya_ble_current_para.pid_len = tuya_ble_current_para.sys_settings.pid_len;
    memcpy(tuya_ble_current_para.pid,tuya_ble_current_para.sys_settings.common_pid,tuya_ble_current_para.pid_len);


    return err_code;
}


uint32_t tuya_ble_storage_save_auth_settings(void)
{
    uint32_t err_code=0;
    // static flash_auth_settings_t flash_settings_auth;
    memcpy(&flash_settings_auth,&tuya_ble_current_para.auth_settings,sizeof(tuya_ble_auth_settings_t));

    flash_settings_auth.crc = tuya_ble_crc32_compute((uint8_t *)&flash_settings_auth+4,sizeof(flash_settings_auth)-4,NULL);


    if(tuya_ble_nv_erase(TUYA_BLE_AUTH_FLASH_ADDR,TUYA_NV_ERASE_MIN_SIZE)==TUYA_BLE_SUCCESS)
    {
        err_code = tuya_ble_nv_write(TUYA_BLE_AUTH_FLASH_ADDR,(uint8_t *)&flash_settings_auth,sizeof(tuya_ble_auth_settings_t));

        if(err_code == TUYA_BLE_SUCCESS)
        {
            TUYA_BLE_LOG("write flash_settings_auth data succeed!");

            if(tuya_ble_nv_erase(TUYA_BLE_AUTH_FLASH_BACKUP_ADDR,TUYA_NV_ERASE_MIN_SIZE)==TUYA_BLE_SUCCESS)
            {
                if(tuya_ble_nv_write(TUYA_BLE_AUTH_FLASH_BACKUP_ADDR,(uint8_t *)&flash_settings_auth,sizeof(tuya_ble_auth_settings_t))!=TUYA_BLE_SUCCESS)
                {
                    TUYA_BLE_LOG("write flash_settings_auth data backup failed!");
                    err_code = 1;
                }
            }
            else
            {
                TUYA_BLE_LOG("erase flash_settings_auth data backup failed!");
                err_code = 1;
            }
        }
        else
        {
            TUYA_BLE_LOG("write flash_settings_auth data failed!");
            err_code = 1;
        }
    }
    else
    {
        TUYA_BLE_LOG("erase flash_settings_auth data failed!");
        err_code = 1;
    }
    return err_code;
}

uint32_t tuya_ble_storage_save_sys_settings(void)
{
    uint32_t err_code=0;

#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

    // static flash_sys_settings_t flash_settings_sys;
    memcpy(&flash_settings_sys,&tuya_ble_current_para.sys_settings,sizeof(tuya_ble_sys_settings_t));

    flash_settings_sys.crc = tuya_ble_crc32_compute((uint8_t *)&flash_settings_sys+4,sizeof(tuya_ble_sys_settings_t)-4,NULL);

    if(tuya_ble_nv_erase(TUYA_BLE_SYS_FLASH_ADDR,TUYA_NV_ERASE_MIN_SIZE)==TUYA_BLE_SUCCESS)
    {
        err_code = tuya_ble_nv_write(TUYA_BLE_SYS_FLASH_ADDR,(uint8_t *)&flash_settings_sys,sizeof(tuya_ble_sys_settings_t));

        if(err_code == TUYA_BLE_SUCCESS)
        {
            TUYA_BLE_LOG("write flash_settings_sys data succeed!");

            if(tuya_ble_nv_erase(TUYA_BLE_SYS_FLASH_BACKUP_ADDR,TUYA_NV_ERASE_MIN_SIZE)==TUYA_BLE_SUCCESS)
            {
                if(tuya_ble_nv_write(TUYA_BLE_SYS_FLASH_BACKUP_ADDR,(uint8_t *)&flash_settings_sys,sizeof(tuya_ble_sys_settings_t))!=TUYA_BLE_SUCCESS)
                {
                    TUYA_BLE_LOG("write flash_settings_sys data backup failed!");
                    err_code = 1;
                }
            }
            else
            {
                TUYA_BLE_LOG("erase flash_settings_sys data backup failed!");
                err_code = 1;
            }

        }
        else
        {
            TUYA_BLE_LOG("write flash_settings_sys data failed!");
            err_code = 1;
        }
    }
    else
    {
        TUYA_BLE_LOG("erase flash_settings_sys data failed!");
        err_code = 1;
    }
#endif

    return err_code;
}

#include "app_common.h"
static const char auth_key_test[] = TUYA_DEVICE_AUTH_KEY;
//static const char h_id_test[] = "1462859897706602496";
static const char device_id_test[] = TUYA_DEVICE_DID;

uint32_t tuya_ble_storage_init(void)
{
    uint32_t err=0;

    tuya_ble_nv_init();

#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

    tuya_ble_storage_load_settings();

    if(buffer_value_is_all_x(tuya_ble_current_para.auth_settings.auth_key,AUTH_KEY_LEN,0))
    {
        memcpy(tuya_ble_current_para.auth_settings.device_id,device_id_test,DEVICE_ID_LEN);
        memcpy(tuya_ble_current_para.auth_settings.auth_key,auth_key_test,AUTH_KEY_LEN);
//        tuya_ble_current_para.auth_settings.pid_type = TUYA_BLE_PRODUCT_ID_TYPE_PID;
//        tuya_ble_current_para.auth_settings.pid_len = 8;
//        memcpy(tuya_ble_current_para.auth_settings.factory_pid,"xQq6PQ6n",8);
//        tuya_ble_current_para.pid_type = tuya_ble_current_para.auth_settings.pid_type;
//        tuya_ble_current_para.pid_len = tuya_ble_current_para.auth_settings.pid_len;
//        memcpy(tuya_ble_current_para.pid,"xQq6PQ6n",8);

        tuya_ble_storage_save_auth_settings();

    }



#endif
    return err;
}


/**
 * @brief   Function for write hid to nv
 *
 * @note
 *
 * */
tuya_ble_status_t tuya_ble_storage_write_pid(tuya_ble_product_id_type_t pid_type,uint8_t pid_len,uint8_t *pid)
{
    tuya_ble_status_t ret = TUYA_BLE_SUCCESS;
    uint8_t is_write = 0;

    if(pid_len>TUYA_BLE_PRODUCT_ID_MAX_LEN)
    {
        return TUYA_BLE_ERR_INVALID_PARAM;
    }

    if((pid_type!=tuya_ble_current_para.pid_type)||(pid_len!=tuya_ble_current_para.pid_len))
    {
        tuya_ble_current_para.pid_type = pid_type;
        tuya_ble_current_para.pid_len = pid_len;
        memcpy(tuya_ble_current_para.pid,pid,pid_len);
        tuya_ble_current_para.sys_settings.pid_type = pid_type;
        tuya_ble_current_para.sys_settings.pid_len = pid_len;
        memcpy(tuya_ble_current_para.sys_settings.common_pid,pid,pid_len);
        is_write = 1;
    }
    else if(memcmp(pid,tuya_ble_current_para.pid,pid_len)!=0)
    {
        tuya_ble_current_para.pid_type = pid_type;
        tuya_ble_current_para.pid_len = pid_len;
        memcpy(tuya_ble_current_para.pid,pid,pid_len);
        tuya_ble_current_para.sys_settings.pid_type = pid_type;
        tuya_ble_current_para.sys_settings.pid_len = pid_len;
        memcpy(tuya_ble_current_para.sys_settings.common_pid,pid,pid_len);
        is_write = 1;
    }
    else
    {

    }
    if(is_write==1)
    {
        if(tuya_ble_storage_save_sys_settings())
        {
            ret = TUYA_BLE_ERR_BUSY;
        }
    }

    return ret;
}


/**
 * @brief   Function for write hid to nv
 *
 * @note
 *
 * */
tuya_ble_status_t tuya_ble_storage_write_hid(uint8_t *hid,uint8_t len)
{
    tuya_ble_status_t ret = TUYA_BLE_SUCCESS;

    if(len!=H_ID_LEN)
    {
        ret = TUYA_BLE_ERR_INVALID_PARAM;
    }
    else
    {
        if(memcmp(hid,tuya_ble_current_para.auth_settings.h_id,H_ID_LEN)!=0)
        {
            memcpy(tuya_ble_current_para.auth_settings.h_id,hid,H_ID_LEN);
            if(tuya_ble_storage_save_auth_settings())
            {
                ret = TUYA_BLE_ERR_BUSY;
            }
        }
    }

    return ret;
}

/**
 * @brief   Function for read id parameters
 *
 * @note
 *
 * */
tuya_ble_status_t tuya_ble_storage_read_id_info(tuya_ble_factory_id_data_t *id)
{
    tuya_ble_status_t ret = TUYA_BLE_SUCCESS;

    id->pid_type = tuya_ble_current_para.pid_type;
    id->pid_len = tuya_ble_current_para.pid_len;
    memcpy(id->pid,tuya_ble_current_para.pid,tuya_ble_current_para.pid_len);

    memcpy(id->h_id,tuya_ble_current_para.auth_settings.h_id,H_ID_LEN);
    memcpy(id->device_id,tuya_ble_current_para.auth_settings.device_id,DEVICE_ID_LEN);
    memcpy(id->mac,tuya_ble_current_para.auth_settings.mac,MAC_LEN);
    memcpy(id->auth_key,tuya_ble_current_para.auth_settings.auth_key,AUTH_KEY_LEN);

    return ret;
}


/**
 * @brief   Function for write auth key/uuid/mac
 *
 * @note  If the id length is 0, the corresponding id will not be written.
 *
 * */
tuya_ble_status_t tuya_ble_storage_write_auth_key_device_id_mac(uint8_t *auth_key,uint8_t auth_key_len,uint8_t *device_id,uint8_t device_id_len,uint8_t *mac,uint8_t mac_len)
{
    tuya_ble_status_t ret = TUYA_BLE_SUCCESS;
    uint8_t is_write = 0;

    if(((auth_key_len!=AUTH_KEY_LEN)&&(auth_key_len!=0))||((device_id_len!=DEVICE_ID_LEN)&&(device_id_len!=0))||((mac_len!=MAC_LEN)&&(mac_len!=0)))
    {
        ret = TUYA_BLE_ERR_INVALID_PARAM;
    }
    else
    {
        if(auth_key_len==AUTH_KEY_LEN)
        {
            if(memcmp(tuya_ble_current_para.auth_settings.auth_key,auth_key,AUTH_KEY_LEN)!=0)
            {
                memcpy(tuya_ble_current_para.auth_settings.auth_key,auth_key,AUTH_KEY_LEN);
                is_write = 1;
            }
        }
        if(device_id_len==DEVICE_ID_LEN)
        {
            if(memcmp(tuya_ble_current_para.auth_settings.device_id,device_id,DEVICE_ID_LEN)!=0)
            {
                memcpy(tuya_ble_current_para.auth_settings.device_id,device_id,DEVICE_ID_LEN);
                is_write = 1;
            }
        }
        if(mac_len==MAC_LEN)
        {
            if(memcmp(tuya_ble_current_para.auth_settings.mac,mac,MAC_LEN)!=0)
            {
                memcpy(tuya_ble_current_para.auth_settings.mac,mac,MAC_LEN);
                is_write = 1;
            }
        }

        if(is_write==1)
        {
            if(tuya_ble_storage_save_auth_settings())
            {
                ret = TUYA_BLE_ERR_BUSY;
            }
        }

    }

    return ret;
}

