# 简介

本仓库为保存学习Framebuffer驱动过程的代码

## 目录结构说明

app:测试用的应用程序
driver：驱动程序
lv_port_linux:lvgl官方的在Linux上的示例

# 使用

## 使用驱动
1. 下载仓库

```
git clone https://github.com/XLIORF/linux_fb_st7735s.git
```

2. 修改Makefile

打开driver/Makefile
修改SDK_DIR变量，改为你的SDK路径
修改CROSS_COMPILE变量，改为你的交叉编译工具路径

3. 编译
```
cd st7735s/driver
make -j
```
4. 上传并加载
```
make upload
```
在开发板上
```
cd
insmod st7735s.ko
```

5. 查看内核输出
在开发板上
```
dmesg
```

## 测试驱动
1. 修改Makefile

2. 编译

3. 上传并运行

# 引脚连接

<!-- |Luckfox pico plus|st7735s|
|:--:|:--:|
|GPIO1_D2|CS|
|GPIO1_C0|DC|
|GPIO1_A2|RES|
|GPIO0_A4|BL| -->

# 命令行工具调试引脚
io -4 0xFF388004 0x00070000  0A4—BL       17
io -4 0xFF380008 0x00100010	out
io -4 0xFF380000 0x00100000	low
io -4 0xFF380000 0x00100010	high

io -4 0xFF538018 0x70000000  1D3—DC       10
io -4 0xFF53000C 0x08000800	out mode
io -4 0xFF530004 0x08000000	low
io -4 0xFF530004 0x08000800	high

io -4 0xFF538000 0x07000000  1A2—RES      11
io -4 0xFF530008 0x00040004	out
io -4 0xFF530000 0x00040000 	low
io -4 0xFF530000 0x00040004	high

io -4 0xFF538010 0x00700040  1C1—CLK     14

io -4 0xFF538010 0x07000600  1C2—MOSI    15

io -4 0xFF538010 0x70006000  1C3—MISO    16

io -4 0xFF538018 0x07000500  1D2—CS       9
io -4 0xFF53000C 0x04000400	out
io -4 0xFF530004 0x04000000	low
io -4 0xFF530004 0x04000400	high

