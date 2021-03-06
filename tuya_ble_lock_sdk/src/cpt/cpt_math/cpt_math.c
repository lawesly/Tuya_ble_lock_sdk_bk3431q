#include "app_common.h"




/*********************************************************************
 * LOCAL CONSTANTS
 */

/*********************************************************************
 * LOCAL STRUCT
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTION
 */

/*********************************************************************
 * VARIABLES
 */




/*********************************************************
FN: sum ����
*/
uint8_t cpt_check_sum(uint8_t *buf, uint32_t size)
{
    uint32_t i;
    uint8_t  sum = 0;
    for(i=0; i<size; i++)
    {
        sum += buf[i];
    }
    return sum;
}

/*********************************************************
FN: sum16 ����
*/
uint16_t cpt_check_sum_16(uint8_t *buf, uint32_t size)
{
    uint32_t i;
    uint16_t  sum = 0;
    for(i=0; i<size; i++)
    {
        sum += buf[i];
    }
    return sum;
}

/*********************************************************
FN: crc16 ����
*/
uint16_t cpt_crc16_compute(uint8_t* buf, uint16_t size, uint16_t* p_crc)
{
    uint16_t poly[2] = {0, 0xa001}; //0x8005 <==> 0xa001
    uint16_t crc = 0xffff;
    int i, j;
    
    for(j=size; j>0; j--)
    {
        uint8_t ds = *buf++;
        for(i=0; i<8; i++)
        {
            crc = (crc >> 1) ^ poly[(crc ^ ds) & 1];
            ds = ds >> 1;
        }
    }
    return crc;
}

/*********************************************************
FN: crc32 ����
*/
uint32_t cpt_crc32_compute(uint8_t* buf, uint32_t size, uint32_t* p_crc)
{
    uint32_t crc;

    crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);
    for (uint32_t i = 0; i < size; i++)
    {
        crc = crc ^ buf[i];
        for (uint32_t j = 8; j > 0; j--)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return ~crc;
}

/*********************************************************
FN: 
*/
void cpt_reverse_byte(void* buf, uint32_t size)
{
    uint8_t* p_tmp = buf;
    uint8_t  tmp;
    for(uint32_t idx=0; idx<size/2; idx++)
    {
        tmp = *(p_tmp+idx);
        *(p_tmp+idx) = *(p_tmp+size-1-idx);
        *(p_tmp+size-1-idx) = tmp;
    }
}









































