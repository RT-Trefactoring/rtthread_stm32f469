
#include <rtthread.h>
#include <board.h>

#if defined(RT_USING_DFS)
#include <dfs_file.h>
#include <dfs_posix.h>
#endif

int mnt_init(void)
{   
#if defined(BSP_USING_RAMDISK) && defined(BSP_USING_RAMDISK_MOUNT) 
    dfs_mkfs("elm", "ram0"); 
    
    if(dfs_mount("ram0", BSP_USING_RAMDISK_PATH_MOUNT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_RAMDISK_PATH_MOUNT);  
    }
#endif
    
#if defined(BSP_USING_SDCARD_SDIO_BUS) && defined(BSP_USING_SDCARD_MOUNT)
    int result = mmcsd_wait_cd_changed(RT_TICK_PER_SECOND);
    if (result == MMCSD_HOST_PLUGED)
    {
        /* mount sd card fat partition 1 as root directory */
        if (dfs_mount("sd0", BSP_USING_SDCARD_PATH_MOUNT, "elm", 0, 0) == 0)
        {
            rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_SDCARD_PATH_MOUNT);  
        }
        extern int chdir(const char *path); 
        chdir(BSP_USING_SDCARD_PATH_MOUNT);
    }
    else
    {
        rt_kprintf("sdcard not inserted!\n");
    }
#elif defined(BSP_USING_SDCARD_BLOCK) && defined(BSP_USING_SDCARD_MOUNT)
    if(dfs_mount("sd0", BSP_USING_SDCARD_PATH_MOUNT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sd0 mount '%s' failed.\n", BSP_USING_SDCARD_PATH_MOUNT); 
    }
    else
    {
        extern int chdir(const char *path); 
        chdir(BSP_USING_SDCARD_PATH_MOUNT); 
    }
    
    
#endif

#ifdef BSP_USING_QSPI_FLASH_MOUNT
    if(dfs_mount("qmtd0", BSP_USING_QSPI_FLASH_PATH_MOUNT, "lfs", 0, 0) != 0)
    {
        dfs_mkfs("lfs", "qmtd0"); 
        if(dfs_mount("qmtd0", BSP_USING_QSPI_FLASH_PATH_MOUNT, "lfs", 0, 0) != 0)
        {
            rt_kprintf("qmtd0 mount '%s' failed.\n", BSP_USING_QSPI_FLASH_PATH_MOUNT); 
        }
    }
#endif
    return 0;
}
INIT_ENV_EXPORT(mnt_init);
