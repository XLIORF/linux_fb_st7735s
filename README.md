# 已修改问题

内存映射报错-6
    分配空间太少报错

内存映射报错-22
    更换了内存映射函数

设备树CS引脚复用设置错误
    引脚复用在GPIO使用指南中有

使用io命令控制GPIO熄灭背光灯，表明dc，res，bl引脚复用正常

在驱动中拉低dc，res，bl引脚正常

短接MOSI，MISO在驱动中收发spi正常

尝试spi 模式0和模式3，无效

驱动中直接清屏，无效

确定是高位先行，CPHA，CPOL=00或11都可以

显示指令和之前成功点亮的一样

# 使用逻辑分析仪分析发先引脚dc，cs的信号并不会发生变化，先查发现引脚复用配置不正确

# 引脚连接

<!-- |Luckfox pico plus|st7735s|
|:--:|:--:|
|GPIO1_D2|CS|
|GPIO1_C0|DC|
|GPIO1_A2|RES|
|GPIO0_A4|BL| -->

# 
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

