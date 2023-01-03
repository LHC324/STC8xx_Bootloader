; 测试工作频率为 24MHz
UARTBAUD EQU 0FFCCH ; 定义串口波特率 (65536-24000000/4/115200)
AUXR DATA 08EH ; 附加功能控制寄存器
S2CON DATA 9AH ;串口2控制寄存器
S2BUF DATA 9BH ;串口2收发缓冲区
; S2RI  BIT  9AH.0 ;串口2接收中断位
; S2TI  BIT  09AH.1 ;串口2发送中断位
; S2RI  EQU  09AH.0 ;串口2接收中断位
; S2TI  EQU  09AH.1 ;串口2发送中断位
T2H   DATA 0D6H ;定时器2高位寄存器
T2L   DATA 0D7H ;定时器2高位寄存器
IE2   DATA 0AFH ;串口2中断允许位
WDT_CONTR DATA 0C1H ; 看门狗控制寄存器
IAP_DATA DATA 0C2H ;IAP 数据寄存器
IAP_ADDRH DATA 0C3H ;IAP 高地址寄存器
IAP_ADDRL DATA 0C4H ;IAP 低地址寄存器
IAP_CMD DATA 0C5H ;IAP 命令寄存器
IAP_TRIG DATA 0C6H ;IAP 命令触发寄存器
IAP_CONTR DATA 0C7H ;IAP 控制寄存器,设置等待时间为24MHz下时间
IAP_TPS DATA 0F5H ;IAP 等待时间控制寄存器
ISPCODE EQU 0FA00H ;ISP 模块入口地址 (1 页 ), 同时也是外部接口地址
APENTRY EQU 0FC00H ; 应用程序入口 地址数据 (1 页 )
P1M1  DATA  091H
P1M0  DATA  092H

ORG 0000H
	MOV P1M1, #00H ;初始化引脚
	MOV P1M0, #00H
	LJMP ISP_ENTRY ; 系统复位入口

RESET:
	MOV S2CON,#10H ; 设置串口模式 (8 位数据位 , 无校验位 )
        MOV AUXR,#14H ; 定时器 2 为 1T 模式
        MOV TMOD,#00H ; 定时器 2 工作于模式 0(16 位重装载 )
        MOV T2H,#HIGH UARTBAUD ; 设置重载值
        MOV T2L,#LOW UARTBAUD
        ;SETB T2R ; 启动定时器 2

NEXT1:
        MOV R0,#16

NEXT2:
        CLR A
        MOV A,S2CON ;S2CON不可位寻址，移到A中使用
        JNB ACC.0, NEXT2 ; 等待串口数据
        ANL S2CON, #NOT 01H ;清除串口2接收中断
        ; CLR S2RI

        MOV A,S2BUF
        CJNE A,#7FH,NEXT1 ; 判断是否为 7F
        DJNZ R0,NEXT2
        LJMP ISP_DOWNLOAD ; 跳转到下载界面


ORG ISPCODE
ISP_DOWNLOAD:
        CLR A
        MOV PSW,A ;ISP 模块使用第 0 组寄存器
        MOV IE,A ; 关闭所有中断

        ANL S2CON, #NOT 01H ;清除串口2接收中断
        ORL S2CON, #02H     ;设置串口2发送中断
        CLR TR0
        MOV SP,#5FH ; 设置堆栈指针
        MOV B,#5AH ; 返回 5A 55 到 PC, 表示 ISP 擦除模块已准备就绪
        LCALL ISP_SENDUART
        MOV B,#055H
        LCALL ISP_SENDUART
        LCALL ISP_RECVACK ; 接收应答数据
        MOV IAP_ADDRL,#0 ; 首先在第 2 页起始地址写 "LJMP ISP_ENTRY" 指令
        MOV IAP_ADDRH,#02H
        LCALL ISP_ERASEIAP

        MOV A,#02H
        LCALL ISP_PROGRAMIAP ; 编程用户代码复位向量代码
        MOV A,#HIGH ISP_ENTRY
        LCALL ISP_PROGRAMIAP ; 编程用户代码复位向量代码
        MOV A,#LOW ISP_ENTRY
        LCALL ISP_PROGRAMIAP ; 编程用户代码复位向量代码
        MOV IAP_ADDRL,#0 ; 用户代码地址从 0 开始
        MOV IAP_ADDRH,#0
        LCALL ISP_ERASEIAP
        MOV A,#02H
        LCALL ISP_PROGRAMIAP ; 编程用户代码复位向量代码
        MOV A,#HIGH ISP_ENTRY
        LCALL ISP_PROGRAMIAP ; 编程用户代码复位向量代码
        MOV A,#LOW ISP_ENTRY
        LCALL ISP_PROGRAMIAP ; 编程用户代码复位向量代码
        MOV IAP_ADDRL,#0 ; 新代码缓冲区地址
        MOV IAP_ADDRH,#02H
        MOV R7,#124 ; 擦除 62.5K 字节

ISP_ERASEAP:
        LCALL ISP_ERASEIAP
        INC IAP_ADDRH ; 目标地址 +512
        INC IAP_ADDRH
        DJNZ R7,ISP_ERASEAP ; 判断是否擦除完成
        MOV IAP_ADDRL,#LOW APENTRY
        MOV IAP_ADDRH,#HIGH APENTRY
        LCALL ISP_ERASEIAP
        MOV B,#5AH ; 返回 5A A5 到 PC, 表示 ISP 编程模块已准备就绪
        LCALL ISP_SENDUART
        MOV B,#0A5H
        LCALL ISP_SENDUART
        LCALL ISP_RECVACK ; 接收应答数据
        LCALL ISP_RECVUART ; 接收长度高字节
        MOV R0,A
        LCALL ISP_RECVUART ; 接收长度低字节
        MOV R1,A
        CLR C ; 将总长度 -3
        MOV A,#03H
        SUBB A,R1
        MOV DPL,A
        CLR A
        SUBB A,R0
        MOV DPH,A ; 总长度补码存入 DPTR
        LCALL ISP_RECVUART ; 映射用户代码复位入口代码到映射区
        LCALL ISP_PROGRAMIAP ;0000
        LCALL ISP_RECVUART
        LCALL ISP_PROGRAMIAP ;0001
        LCALL ISP_RECVUART
        LCALL ISP_PROGRAMIAP ;0002
        MOV IAP_ADDRL,#03H ; 用户代码起始地址
        MOV IAP_ADDRH,#00H

ISP_PROGRAMNEXT:
        LCALL ISP_RECVUART ; 接收代码数据
        LCALL ISP_PROGRAMIAP ; 编程到用户代码区
        INC DPTR
        MOV A,DPL
        ORL A,DPH
        JNZ ISP_PROGRAMNEXT ; 长度检测

ISP_SOFTRESET:
        MOV IAP_CONTR,#20H ; 软件复位系统,复位后从用户代码执行
        SJMP $
        
ISP_ENTRY:
        MOV WDT_CONTR,#17H ; 清看门狗
        MOV IAP_CONTR,#80H ; 使能 IAP 功能
        MOV IAP_TPS,#11 ; 设置 IAP 等待时间参数
        MOV IAP_ADDRL,#LOW ISP_DOWNLOAD
        MOV IAP_ADDRH,#HIGH ISP_DOWNLOAD
        MOV IAP_DATA,#00H ; 测试数据 1
        MOV IAP_CMD,#1 ; 读命令
        MOV IAP_TRIG,#5AH ; 触发 ISP 命令
        MOV IAP_TRIG,#0A5H
        MOV A,IAP_DATA
        CJNE A,#0E4H,ISP_ENTRY ; 若无法读出数据则需要等待电压稳定
        INC IAP_ADDRL ; 测试地址 FC01H
        MOV IAP_DATA,#45H ; 测试数据 2
        MOV IAP_CMD,#1 ; 读命令
        MOV IAP_TRIG,#5AH ; 触发 ISP 命令
        MOV IAP_TRIG,#0A5H
        MOV A,IAP_DATA
        CJNE A,#0F5H,ISP_ENTRY ; 若无法读出数据则需要等待电压稳定
        ; MOV SCON,#50H ; 设置串口模式 (8 位数据位 , 无校验位 )
        ; MOV AUXR,#40H ; 定时器 1 为 1T 模式
        ; MOV TMOD,#00H ; 定时器 1 工作于模式 0(16 位重装载 )
        ; MOV TH1,#HIGH UARTBAUD ; 设置重载值
        ; MOV TL1,#LOW UARTBAUD
        ; SETB TR1 ; 启动定时器 1

        MOV S2CON,#10H ; 设置串口模式 (8 位数据位 , 无校验位 )
        MOV AUXR,#14H ; 定时器 2 为 1T 模式
        MOV TMOD,#00H ; 定时器 2 工作于模式 0(16 位重装载 )
        MOV T2H,#HIGH UARTBAUD ; 设置重载值
        MOV T2L,#LOW UARTBAUD

        SETB TR0
        LCALL ISP_RECVUART ; 检测是否有串口数据
        JC GOTOAP
        MOV R0,#16

ISP_CHECKNEXT:
        LCALL ISP_RECVUART ; 接收同步数据
        JC GOTOAP
        CJNE A,#7FH,GOTOAP ; 判断是否为 7F
        DJNZ R0,ISP_CHECKNEXT
        MOV B,#5AH ; 返回 5A 69 到 PC, 表示 ISP 模块已准备就绪
        LCALL ISP_SENDUART
        MOV B,#69H
        LCALL ISP_SENDUART
        LCALL ISP_RECVACK ; 接收应答数据
        LJMP ISP_DOWNLOAD ; 跳转到下载界面

GOTOAP:
        CLR A ; 将 SFR 恢复为复位值
        MOV TCON,A
        MOV TMOD,A
        MOV TL0,A
        MOV TH0,A
        ; MOV TL1,A
        ; MOV TH1,A
        ; MOV SCON,A
        MOV S2CON,A
        MOV T2L,A
        MOV T2H,A

        MOV AUXR,A
        LJMP APENTRY ; 正常运行用户程序

ISP_RECVACK:
        LCALL ISP_RECVUART
        JC GOTOAP
        XRL A,#7FH
        JZ ISP_RECVACK ; 跳过同步数据
        CJNE A,#25H,GOTOAP ; 应答数据 1 检测
        LCALL ISP_RECVUART
        JC GOTOAP
        CJNE A,#69H,GOTOAP ; 应答数据 2 检测
        RET

ISP_RECVUART:
        CLR A
        MOV TL0,A ; 初始化超时定时器
        MOV TH0,A
        CLR TF0
        MOV WDT_CONTR,#17H ; 清看门狗
		
ISP_RECVWAIT:
        JBC TF0,ISP_RECVTIMEOUT ; 超时检测

        MOV R2, #00H ;清零R2
        CLR A
        MOV A,S2CON ;S2CON不可位寻址，移到A中使用
        JNB ACC.0,ISP_RECVWAIT ; 等待接收完成
        MOV A,S2BUF ; 读取串口数据
        ; CLR S2RI ; 清除标志
        ANL S2CON, #NOT 01H ;清除串口2接收中断
        CLR C ; 正确接收串口数据
        RET

ISP_RECVTIMEOUT:
        INC R2 ;增加超时次数(32.768ms/次)
        CJNE R2, #14H , ISP_RECVTIMEOUT 
        MOV R2, #00H
        SETB C ; 超时退出
        RET

ISP_SENDUART:
        MOV WDT_CONTR,#17H ; 清看门狗

        CLR A
        MOV A,S2CON ;S2CON不可位寻址，移到A中使用
        JNB ACC.1,ISP_SENDUART ; 等待前一个数据发送完成
        ; CLR S2TI ; 清除标志
        ANL S2CON, #NOT 02H ;清除串口2发送中断
        MOV S2BUF,B ; 发送当前数据
        RET

ISP_ERASEIAP:
        MOV WDT_CONTR,#17H ; 清看门狗
        MOV IAP_CMD,#3 ; 擦除命令
        MOV IAP_TRIG,#5AH ; 触发 ISP 命令
        MOV IAP_TRIG,#0A5H
        NOP
        NOP
        NOP
        NOP
        RET

ISP_PROGRAMIAP:
        MOV WDT_CONTR,#17H ; 清看门狗
        MOV IAP_CMD,#2 ; 编程命令
        MOV IAP_DATA,A ; 将当前数据送 IAP 数据寄存器
        MOV IAP_TRIG,#5AH ; 触发 ISP 命令
        MOV IAP_TRIG,#0A5H
        NOP
        NOP
        NOP
        NOP
        MOV A,IAP_ADDRL ;IAP 地址 +1
        ADD A,#01H
        MOV IAP_ADDRL,A
        MOV A,IAP_ADDRH
        ADDC A,#00H
        MOV IAP_ADDRH,A
        RET


ORG APENTRY
        LJMP RESET
END