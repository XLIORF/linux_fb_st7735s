#include "st7735s.h"
#include "KernelDebug.h"
#include "asm/page.h"
#include "asm/string.h"
#include "linux/gfp.h"
#include "linux/mm.h"
#include "linux/slab.h"
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/completion.h>
#include <linux/fb.h>
#include <linux/uaccess.h> 

static const struct fb_fix_screeninfo st7735sfb_fix = {
	.id = NAME,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_DIRECTCOLOR,
	.xpanstep = 0,
	.ypanstep = 0,
	.ywrapstep = 0,
	.line_length = ST7735S_WIDTH * 2, // 一个像素两个字节
    // .smem_start = ,
    .smem_len = ST7735S_SBUF_SIZE, //lvgl_driver骗子，就是显存的大小
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo st7735sfb_var = {
	.xres = ST7735S_WIDTH,
	.yres = ST7735S_HEIGHT,
	.xres_virtual = ST7735S_WIDTH,
	.yres_virtual = ST7735S_HEIGHT, 
    .xoffset = 0,
    .yoffset = 0,
	.bits_per_pixel = 16,
	.red = { 11, 5, 0 },
    .green = { 5, 6, 0 },
    .blue = { 0, 5, 0 },
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.vmode = FB_VMODE_NONINTERLACED,
};

static int st7735sfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    int ret = 0;
    st7735s_priv_t* priv = info->par;

    /* 获得物理地址 */
    unsigned long phy = virt_to_phys(priv->map_buffer);
    /* 设置属性：cache, buffer*/
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    vma->vm_flags |= VM_IO;//表示对设备IO空间的映射  
    // vma->vm_flags |= VM_RESERVED;//标志该内存区不能被换出，在设备驱动中虚拟页和物理页的关系应该是长期的，应该保留起来，不能随便被别的虚拟页换出  
    /* map */

    ret = remap_pfn_range(vma, vma->vm_start, phy >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);

    if(ret)
    {
        DEBUG("remap_pfn_range error!");
        return -ENOBUFS;
    }
	return ret;
}

static int st7735s_fb_blank(int blank, struct fb_info *info)
{
    st7735s_priv_t* priv = info->par;
    switch (blank) {
        case FB_BLANK_UNBLANK:
            // 屏幕开启的代码
            printk(KERN_INFO "Screen ON\n");
            gpio_set_value(priv->bl_pin, 1);
            break;
        case FB_BLANK_POWERDOWN:
            // 屏幕关闭的代码
            printk(KERN_INFO "Screen OFF\n");
            gpio_set_value(priv->bl_pin, 0);
            break;
        default:
            return -EINVAL;  // 不支持的 blank 选项
    }
    return 0;
}

static const struct fb_ops st7735sfb_ops = {
	.owner = THIS_MODULE,
	.fb_read = fb_sys_read,
	.fb_write = fb_sys_write,
	.fb_fillrect = sys_fillrect,
	.fb_copyarea = sys_copyarea,
	.fb_imageblit = sys_imageblit,
	.fb_mmap = st7735sfb_mmap,
    .fb_blank = st7735s_fb_blank,
};

static int st7735s_display(st7735s_priv_t *priv)
{
	int ret = -1;
    lcd_set_address(priv, 0, 0, ST7735S_WIDTH, ST7735S_HEIGHT);
    SET_LCD_A0;
    ret = spi_write(priv->spi, priv->map_buffer, ST7735S_SBUF_SIZE);
	if(ret < 0) 
    {
        DEBUG("spi transfer fail!,code:%d", ret);
        return -EINVAL;
	}
    return 0;
}

static void st7735s_delay_work(struct work_struct *work)
{
    st7735s_priv_t *priv = container_of(work, st7735s_priv_t, work.work);//获得私有数据
    st7735s_display(priv);//更新显示
    schedule_delayed_work(&priv->work, msecs_to_jiffies(priv->interval));//再次开始任务
}

int ST7735S_Probe(struct spi_device *spi)
{
    int ret = 0;
    st7735s_priv_t* priv = NULL;
    struct fb_info* info = NULL;
 
    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
    memset(priv, 0, sizeof(*priv));

    priv->map_buffer = (char *) kmalloc(ST7735S_SBUF_SIZE, GFP_KERNEL);
	if (priv->map_buffer == NULL) {
		DEBUG("can't get a free page");
		ret = -ENOMEM;
        return ret;
	}

    DEBUG("total alloc pages number of the display memory is %ld", ST7735S_SBUF_SIZE / PAGE_SIZE );

    info = &priv->fbinfo;
    info->screen_base = (char __iomem *) priv->map_buffer;
	info->screen_size = ST7735S_SBUF_SIZE;
	info->fbops = &st7735sfb_ops;
	info->fix = st7735sfb_fix;
	info->var = st7735sfb_var;
    info->fix.smem_start = (unsigned long )priv->map_buffer;
	info->pseudo_palette = NULL;
	info->par = priv;
	info->flags = FBINFO_FLAG_DEFAULT;
    
    DEBUG("register framebuffer");
    ret = register_framebuffer(info);
    if (ret < 0){
        kfree(priv->map_buffer);
        DEBUG("register_framebuffer is error!");
		return ret;
    }
    fb_info(info, "%s frame buffer device\n", info->fix.id);

    priv->spi = spi;
    spi_set_drvdata(spi, priv);

    DEBUG("initialize st7735s");
    lcd_st7735s_init(priv);
    SET_LCD_BL;
    lcd_clear(priv, WHITE);

    DEBUG("config delay work");

    priv->interval = 30;
    INIT_DELAYED_WORK(&priv->work, st7735s_delay_work);
    schedule_delayed_work(&priv->work, msecs_to_jiffies(priv->interval));

    return ret;
}

int ST7735S_Remove(struct spi_device *spi)
{
    st7735s_priv_t *priv = spi_get_drvdata(spi);
    // lcd_clear(priv, WHITE);
    lcd_display_off(priv);
    CLR_LCD_BL;

    gpio_free(priv->res_pin);
    gpio_free(priv->dc_pin );
    gpio_free(priv->bl_pin );

    kfree(priv->map_buffer);
    cancel_delayed_work_sync(&priv->work);

	if (&(priv->fbinfo) != NULL) 
    {
		unregister_framebuffer(&priv->fbinfo);
	}
    return 0;
}

void ST7735S_Shutdown(struct spi_device *spi)
{
    st7735s_priv_t *priv = spi_get_drvdata(spi);
    lcd_clear(priv, WHITE);
    lcd_display_off(priv);
    CLR_LCD_BL;
}

static const struct spi_device_id st7735s_id[] = {
	{"WLX,st7735s", 0},  
	{} // 表示结束，最好加上，概率在probe运行前报段错误，可能是这个的原因
};

/* 设备树匹配列表 */
static const struct of_device_id st7735s_of_match[] = {
    {.compatible = "WLX,st7735s"},
    {} // 表示结束，最好加上，概率在probe运行前报段错误，可能是这个的原因
};

static struct spi_driver st7735s_driver = {
    .driver = {
        .name = NAME,
        .owner = THIS_MODULE,
        .of_match_table = st7735s_of_match,
    },
    .probe = ST7735S_Probe,       //匹配设备成功时执行
    .remove = ST7735S_Remove,     //模块卸载或设备掉线时执行
    .shutdown = ST7735S_Shutdown, //当系统关机时执行
	.id_table = st7735s_id,
};

module_spi_driver(st7735s_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WangLingXin");

/***********************************硬件操作*******************************************/
int lcd_gpio_init(st7735s_priv_t* priv)
{
    int ret = -1;
    priv->nd = of_find_node_by_path(DTS_PATH);
    if(priv->nd == NULL) 
    {
		DEBUG("st7735 gpio node not find!");
		return -EINVAL;
	} 
    else 
    {
		DEBUG("gpio node find!");
	}
    priv->res_pin = of_get_named_gpio(priv->nd, "res-gpios", 0);
    priv->dc_pin = of_get_named_gpio(priv->nd, "dc-gpios", 0);
    priv->bl_pin = of_get_named_gpio(priv->nd, "bl-gpios", 0);
	if(priv->dc_pin < 0 || priv->res_pin < 0 || priv->bl_pin < 0) {
		DEBUG("can't get gpio, res:%d, dc: %d, bl:%d", priv->res_pin, priv->dc_pin, priv->bl_pin);
		return -EINVAL;
	}
    gpio_request(priv->res_pin, NULL);
    gpio_request(priv->dc_pin , NULL);
    gpio_request(priv->bl_pin , NULL);

	DEBUG("dc_pin num = %d", priv->dc_pin);
    ret = gpio_direction_output(priv->dc_pin, 1);
	if(ret < 0) {
		DEBUG("can't set gpio!");
	}
    ret = gpio_direction_output(priv->res_pin, 1);
	if(ret < 0) {
		DEBUG("can't set gpio!");
	}
    ret = gpio_direction_output(priv->bl_pin, 1);
	if(ret < 0) {
		DEBUG("can't set gpio!");
	}
    return 0;
}

void lcd_st7735s_reset(st7735s_priv_t* priv)
{
    SET_LCD_RES;
    msleep(10);
	CLR_LCD_RES;
	msleep(10);
	SET_LCD_RES;
	msleep(200);
}

void lcd_st7735s_init(st7735s_priv_t* priv)
{
    lcd_gpio_init(priv);
    
    lcd_st7735s_reset(priv);
    
    lcd_write_register(priv, 0x11); //Sleep out 
    msleep(120);   
    
    lcd_write_register(priv, 0xB1); 
    lcd_writes_data(priv, 0x05, 0x3C, 0x3C, 0);
    
    lcd_write_register(priv, 0xB2); 
    lcd_writes_data(priv, 0x05, 0x3C, 0x3C, 0);
    
    lcd_write_register(priv, 0xB3); 
    lcd_writes_data(priv, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C, 0);
    
    lcd_write_register(priv, 0xB4);
    lcd_write_data(priv, 0x03); 
    
    lcd_write_register(priv, 0xC0); 
    lcd_writes_data(priv, 0x28, 0x08, 0x04, 0);
    
    lcd_write_register(priv, 0xC1); 
    lcd_write_data(priv, 0XC0); 
    
    lcd_write_register(priv, 0xC2); 
    lcd_write_data(priv, 0x0D); 
    lcd_write_data(priv, 0x00); 
    
    lcd_write_register(priv, 0xC3); 
    lcd_write_data(priv, 0x8D); 
    lcd_write_data(priv, 0x2A); 
    
    lcd_write_register(priv, 0xC4); 
    lcd_write_data(priv, 0x8D); 
    lcd_write_data(priv, 0xEE); 
    
    lcd_write_register(priv, 0xC5);
    lcd_write_data(priv, 0x1A); 
    
    lcd_write_register(priv, 0x36);
    lcd_write_data(priv, 0xC0); 
    
    lcd_write_register(priv, 0xE0); 
    lcd_writes_data(priv, 0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25, 0x2A, 0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01, 0x03, 0x13, 0);
    
    lcd_write_register(priv, 0xE1); 
    lcd_writes_data(priv, 0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23, 0x27, 0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01, 0x04, 0x13, 0);
    
    lcd_write_register(priv, 0x3A);
    lcd_write_data(priv, 0x05);
    lcd_write_register(priv, 0x29); 
}

int lcd_write_register(st7735s_priv_t *priv, unsigned char data)
{
	int ret = -1;
    CLR_LCD_A0;
    CLR_LCD_CS;
    ret = spi_write(priv->spi, &data, 1);
	if(ret < 0) 
    {
        DEBUG("spi transfer fail!");
        return -EINVAL;
	}
    SET_LCD_CS;
    return 0;
}

int lcd_writes_data(st7735s_priv_t *priv, ...)
{
	int ret = -1;
    int arg_count = 0, i;
    u8* data = NULL;
    va_list vi;
    va_start(vi, priv);
    while (va_arg(vi, int) != 0) {
        arg_count++;
    }
    va_start(vi, priv);
    data = kmalloc(arg_count+1, GFP_KERNEL);
    if (data == NULL)
    {
        DEBUG("spi transfer fail!");
        return -1;
    }
    for(i = 0;i < arg_count;i++)
    {
        data[i] = va_arg(vi, int);
    }
    SET_LCD_A0;
    CLR_LCD_CS;
    ret = spi_write(priv->spi, data, arg_count);
	if(ret < 0) 
    {
        DEBUG("spi transfer fail!");
        return -EINVAL;
	}
    SET_LCD_CS;
    return 0;
}

int lcd_write_data(st7735s_priv_t *priv, unsigned char data)
{
	int ret = -1;
    SET_LCD_A0;
    CLR_LCD_CS;
    ret = spi_write(priv->spi, &data, 1);
	if(ret < 0) 
    {
        DEBUG("spi transfer fail!");
        return -EINVAL;
	}
    SET_LCD_CS;
    return 0;
}

int lcd_write_data_u16(st7735s_priv_t *priv, unsigned short data)
{
    int ret = -1;
    uint8_t buf[] = {data>>8, data&0xff};
    SET_LCD_A0;
    CLR_LCD_CS;
    ret = spi_write(priv->spi, buf, sizeof(buf));
	if(ret < 0) 
    {
        DEBUG("spi transfer fail!");
        return -EINVAL;
	}
    SET_LCD_CS;
    return 0;
}

int lcd_write_data_buf(st7735s_priv_t *priv, const unsigned char* data, size_t len)
{
    int ret = -1;
    SET_LCD_A0;
    CLR_LCD_CS;
    ret = spi_write(priv->spi, data, len);
	if(ret < 0) 
    {
        DEBUG("spi transfer fail!");
        return -EINVAL;
	}
    SET_LCD_CS;
    return 0;
}

void lcd_set_address(st7735s_priv_t *priv, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{	     
    lcd_write_register(priv, 0x2A); 
    lcd_write_data_u16(priv, x1);
    lcd_write_data_u16(priv, x2);

    lcd_write_register(priv, 0x2B); 
    lcd_write_data_u16(priv, y1);
    lcd_write_data_u16(priv, y2);
    
    lcd_write_register(priv, 0x2C);
}


//LCD开启显示
void lcd_display_on(st7735s_priv_t *priv)
{					   
	lcd_write_register(priv, 0x29);	//开启显示
}	 
//LCD关闭显示
void lcd_display_off(st7735s_priv_t *priv)
{	   
	lcd_write_register(priv, 0x28);	//关闭显示
}   

void lcd_clear(st7735s_priv_t *priv, unsigned short color)
{
    memset(priv->map_buffer, 0xff, ST7735S_SBUF_SIZE);
    // unsigned char i, j;
    // lcd_set_address(priv, 0, 0, 130, 170);
    // for(i = 0; i < 130; i++)
    // {
    //     for (j = 0; j < 170; j++)
	//    	{
    //         lcd_write_data_u16(priv, color);
	//     }
    // }
} 

//LCD画点
void lcd_draw_point(st7735s_priv_t *priv, unsigned short x, unsigned short y, unsigned short color)
{
	 lcd_set_address(priv, x, y, x, y);//设置光标位置 
	 lcd_write_data_u16(priv, color);
}

//快速画点
//x,y:坐标
//color:颜色
void lcd_fast_draw_point(st7735s_priv_t *priv, unsigned short x, unsigned short y, unsigned short color)
{
	lcd_write_data_u16(priv, color);
}	


//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void lcd_fill(st7735s_priv_t *priv, unsigned short sx, unsigned short sy, unsigned short ex, unsigned short ey, unsigned short color)
{          
	unsigned short i, j;
	unsigned short xlen = 0;
	unsigned short ylen = 0;
	
	xlen = ex - sx + 1;
	ylen = ey - sy + 1;
		
	lcd_set_address(priv, sx, sy, ex, ey);
    for(i = 0; i < xlen; i++)
	{
		for(j = 0; j < ylen; j++)
		{
			lcd_write_data_u16(priv, color);
		}
	}		 
}  

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void lcd_draw_line(st7735s_priv_t *priv, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color)
{
	unsigned short t; 
	int xerr = 0, yerr = 0, delta_x, delta_y, distance; 
	int incx, incy, uRow, uCol; 
	delta_x = x2 - x1; //计算坐标增量 
	delta_y = y2 - y1; 
	uRow = x1; 
	uCol = y1; 
	
	//设置单步方向 
	if( delta_x > 0 )
	{
		incx=1; 
	}
	else if( delta_x == 0 )//垂直线 
	{
		incx=0;
	}
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}

	
	if( delta_y > 0 )
	{
		incy=1; 
	}
	else if( delta_y == 0 )//水平线 
	{
		incy=0;
	}
	else
	{
		incy = -1;
		delta_y = -delta_y;
	} 
	
	if( delta_x > delta_y )//选取基本增量坐标轴 
	{
		distance = delta_x; 
	}
	else
	{
		distance=delta_y; 
	}
	
	for(t = 0; t <= distance + 1; t++ )//画线输出 
	{  
		lcd_draw_point(priv, uRow, uCol, color);//画点 
		xerr += delta_x ; 
		yerr += delta_y ; 
		if( xerr > distance ) 
		{ 
			xerr -= distance; 
			uRow += incx; 
		} 
		
		if( yerr > distance ) 
		{ 
			yerr -= distance; 
			uCol += incy; 
		} 
	}  
}  

void lcd_draw_rectangle(st7735s_priv_t *priv, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, unsigned short color)
{
	lcd_draw_line(priv, x1, y1, x2, y1, color);
	lcd_draw_line(priv, x1, y1, x1, y2, color);
	lcd_draw_line(priv, x1, y2, x2, y2, color);
	lcd_draw_line(priv, x2, y1, x2, y2, color);
}

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void lcd_draw_circle(st7735s_priv_t *priv, unsigned short x0, unsigned short y0, unsigned char r, unsigned short color)
{
	int a, b;
	int di;
	a = 0;
	b = r;	  
	di = 3 - ( r<<1 );             //判断下个点位置的标志
	while( a <= b )
	{
		lcd_draw_point(priv, x0+a, y0-b, color);
 		lcd_draw_point(priv, x0+b, y0-a, color);        
		lcd_draw_point(priv, x0+b, y0+a, color);       
		lcd_draw_point(priv, x0+a, y0+b, color);
		lcd_draw_point(priv, x0-a, y0+b, color);
 		lcd_draw_point(priv, x0-b, y0+a, color);
		lcd_draw_point(priv, x0-a, y0-b, color);
  		lcd_draw_point(priv, x0-b, y0-a, color);
		a++;
		//使用Bresenham算法画圆     
		if( di < 0 )
		{
			di += 4 * a + 6;	 
		} 
		else
		{
			di += 10 + 4 * ( a - b );   
			b--;
		} 						    
	}
} 	

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void lcd_draw_full_circle(st7735s_priv_t *priv, unsigned short Xpos, unsigned short Ypos, unsigned short Radius, unsigned short Color)
{
	uint16_t x, y, r = Radius;
	for(y = Ypos - r; y < Ypos + r; y++)
	{
		for(x = Xpos - r;x < Xpos + r; x++)
		{
			if(((x - Xpos) * (x - Xpos) + (y - Ypos) * (y - Ypos)) <= r * r)
			{
				lcd_draw_point(priv, x, y, Color);
			}
		}
	}
}
