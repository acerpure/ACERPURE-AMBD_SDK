#include "flash_util.h"

#include "flash_api.h"
#include "device_lock.h"


void user_data_partition_ops(USER_PARTITION id, USER_PARTITION_OP ops, u8 *data, u16 len)
{
	flash_t flash;
	int size, offset, i;
	u32 read_data;

	u32 addr;

	switch (id)
	{
		case USER_PARTITION_WIFI:
			addr = WIFI_SETTING_SECTOR;
			size = WIFI_CONF_DATA_SIZE;
			offset = WIFI_CONF_DATA_OFFSET;
			break;
		case USER_PARTITION_AWS_MQTT:
			addr = AWS_MQTT_SETTING_SECTOR;
			size = AWS_MQTT_CONF_DATA_SIZE;
			offset = AWS_MQTT_CONF_DATA_OFFSET;
			break;
		case USER_PARTITION_AWS_DEVICE_INFO:
			addr = AWS_DEVICE_INFO_SECTOR;
			size = AWS_DEVICE_INFO_DATA_SIZE;
			offset = AWS_DEVICE_INFO_DATA_OFFSET;
			break;
		case USER_PARTITION_AWS_ALERT:
			addr = AWS_ALERT_SETTING_SECTOR;
			size = AWS_ALERT_CONF_DATA_SIZE;
			offset = AWS_ALERT_CONF_DATA_OFFSET;
			break;
		// case USER_PARTITION_AWS_ROOT_CA:
        //     addr = AWS_ROOT_CA_SETTING_SECTOR;
        //     size = AWS_ROOT_CA_CONF_DATA_SIZE;
        //     offset = AWS_ROOT_CA_CONF_DATA_OFFSET;
        //     break;
		case USER_PARTITION_AWS_KEY:
            addr = AWS_KEY_SETTING_SECTOR;
            size = AWS_KEY_CONF_DATA_SIZE;
            offset = AWS_KEY_CONF_DATA_OFFSET;
            break;
        case USER_PARTITION_AWS_CLIENT_CA:
            addr = AWS_CLIENT_CA_SETTING_SECTOR;
            size = AWS_CLIENT_CA_CONF_DATA_SIZE;
            offset = AWS_CLIENT_CA_CONF_DATA_OFFSET;
            break;
		default:
			printf("partition id is invalid!\r\n");
			return;
	}

	device_mutex_lock(RT_DEV_LOCK_FLASH);

	if (ops == USER_PARTITION_ERASE)
	{
		flash_erase_sector(&flash, addr);
		goto exit;
	}
	
	if (ops == USER_PARTITION_READ)
	{
		flash_stream_read(&flash, addr + offset, len, data);
		goto exit;
	}

	flash_erase_sector(&flash, addr);

	flash_stream_write(&flash, addr + offset, len, data);

#if 0
	// write to backup sector first, erase FLASH_BACKUP_SECTOR
	flash_erase_sector(&flash, FLASH_BACKUP_SECTOR);

	if (ops == USER_PARTITION_WRITE)
	{
		// backup new data
		flash_stream_write(&flash, addr + offset, len, data);
	}
	
	// backup front data to backup sector
	for (i = 0; i < offset; i += sizeof(read_data))
	{
		flash_read_word(&flash, addr + i, &read_data);
		flash_write_word(&flash, FLASH_BACKUP_SECTOR + i, read_data);
	}
	
	// backup rear data to backup sector
	for (i = (offset + size); i < FLASH_SECTOR_SIZE; i += sizeof(read_data))
	{
		flash_read_word(&flash, addr + i, &read_data);
		flash_write_word(&flash, FLASH_BACKUP_SECTOR + i, read_data);
	}
	
	// erase user data sector for writing
	flash_erase_sector(&flash, addr);
	
	// restore data to user data sector from backup sector
	for (i = 0; i < FLASH_SECTOR_SIZE; i+= sizeof(read_data))
	{
		flash_read_word(&flash, FLASH_BACKUP_SECTOR + i, &read_data);
		flash_write_word(&flash, addr + i, read_data);
	}
	
	// erase backup sector
	flash_erase_sector(&flash, FLASH_BACKUP_SECTOR);
#endif
exit:
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return;
}