#include <stdio.h>
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/mman.h>  
#include <linux/fb.h>  
#include <string.h>  
#include <stdlib.h>  
#include <sys/ioctl.h>
#include <unistd.h>
#include "st7735s_pic.h"


int main() {  
    int fd;  
    struct fb_var_screeninfo varinfo;  
    struct fb_fix_screeninfo fixinfo;  
    unsigned long screensize;  
    char *fbmem;  
  
    // 打开帧缓冲设备  
    fd = open("/dev/fb0", O_RDWR);  
    if (fd == -1) 
    {  
        perror("Error: cannot open framebuffer device");  
        exit(1);  
    }  
  
    // 获取帧缓冲设备的可变和固定信息  
    if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) == -1) 
    {  
        perror("Error reading variable screen information");  
        exit(1);  
    }  
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) == -1) 
    {  
        perror("Error reading fixed screen information");  
        exit(1);  
    }  

    // 计算屏幕尺寸  
    screensize = varinfo.xres * varinfo.yres * varinfo.bits_per_pixel / 8;  
    printf("屏幕尺寸：%d X %d\n", varinfo.xres, varinfo.yres);

    // 映射帧缓冲内存  
    fbmem = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);  
    if ((intptr_t)fbmem == -1) 
    {  
        perror("Error: failed to mmap framebuffer device");  
        exit(1);  
    }  
    //填充显示数据

    memcpy(fbmem, gImage_st7735s_pic, 34848);
    // 解除映射  
    if (munmap(fbmem, screensize) == -1) 
    {  
        perror("Error un-mmapping the device");  
        // perror(strerror(ret));
    }
  
    // 关闭帧缓冲设备文件  
    close(fd);  
  
    return 0;  
}