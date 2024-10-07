#ifndef __ST7735S_H__
#define __ST7735S_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/ioctl.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include "KernelDebug.h"
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#define DTS_PATH  "/spi@ff500000/st7735s@0"

#define SET_LCD_BL 	 gpio_set_value(priv->bl_pin, 1)
#define SET_LCD_RES  gpio_set_value(priv->res_pin, 1)
#define SET_LCD_A0 	 gpio_set_value(priv->dc_pin, 1)
#define SET_LCD_CS 	 //
  
#define	CLR_LCD_BL    gpio_set_value(priv->bl_pin, 0)
#define	CLR_LCD_RES   gpio_set_value(priv->res_pin, 0)
#define	CLR_LCD_A0    gpio_set_value(priv->dc_pin, 0)
#define	CLR_LCD_CS    //

#define WHITE	0xFFFF
#define BLACK	0x0000	  
#define BLUE	0x001F  
#define BRED	0XF81F
#define GRED	0XFFE0
#define GBLUE	0X07FF
#define RED		0xF800
#define MAGENTA	0xF81F
#define GREEN	0x07E0
#define CYAN	0x7FFF
#define YELLOW	0xFFE0
#define BROWN	0XBC40 //棕色
#define BRRED	0XFC07 //棕红色
#define GRAY	0X8430 //灰色
#define DARKBLUE	0X01CF	//深蓝色
#define LIGHTBLUE	0X7D7C	//浅蓝色  
#define GRAYBLUE	0X5458 //灰蓝色



#define NAME "st7735s" // 设备节点名字
#define ST7735S_WIDTH 132
#define ST7735S_HEIGHT 162
#define ST7735S_PIXEL_SIZE ST7735S_WIDTH * ST7735S_HEIGHT
#define ST7735S_BPP 16
#define ST7735S_SBUF_SIZE ST7735S_PIXEL_SIZE * ST7735S_BPP / 8 //RGB565


typedef struct 
{
    unsigned int interval;      //屏幕刷新任务执行间隔
    char *map_buffer;        // 映射到用户空间的显存
    struct fb_info fbinfo;      // fb结构体
    struct delayed_work work;   // 用于延时刷新的结构体
    struct spi_device *spi;
    struct device_node	*nd;
    dma_addr_t dma_handle;
    int dc_pin;
    int res_pin;
    int bl_pin;
} st7735s_priv_t;



int lcd_gpio_init(st7735s_priv_t *priv);
void lcd_st7735s_init(st7735s_priv_t *priv);
int lcd_write_register(st7735s_priv_t *priv, unsigned char data);
int lcd_write_data(st7735s_priv_t *priv, unsigned char data);
int lcd_write_data_u16(st7735s_priv_t *priv, unsigned short data);

void lcd_set_address(st7735s_priv_t *priv, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2);
void lcd_clear(st7735s_priv_t *priv, unsigned short color);

void lcd_draw_point(st7735s_priv_t *priv, unsigned short x, unsigned short y, unsigned short color);

void lcd_display_on(st7735s_priv_t *priv); //开显示
void lcd_display_off(st7735s_priv_t *priv); //关显示
void lcd_draw_circle(st7735s_priv_t *priv, unsigned short x0, unsigned short y0, unsigned char r, unsigned short color); //画圆
void lcd_draw_full_circle(st7735s_priv_t *priv, unsigned short Xpos, unsigned short Ypos, unsigned short Radius, unsigned short Color);
void lcd_fill(st7735s_priv_t *priv, unsigned short sx, unsigned short sy, unsigned short ex, unsigned short ey, unsigned short color); //填充区域
void lcd_draw_line(st7735s_priv_t *priv, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color); //画线
void lcd_draw_rectangle(st7735s_priv_t *priv, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color); //画矩形

int lcd_writes_data(st7735s_priv_t *priv, ...);
int lcd_write_data_buf(st7735s_priv_t *priv, const unsigned char* data, size_t len);

static int st7735s_blank(st7735s_priv_t *priv, int blank);
#endif