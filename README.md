# ChanlunX

## 如何编译

### 编译 32 位版本

适用于通达信 32 位版本：

```cmd
mkdir build
cd build
cmake -A Win32 ..
cmake --build . --config Release
```

### 编译 64 位版本

适用于通达信 64 位版本：

```cmd
mkdir build
cd build
cmake -A x64 ..
cmake --build . --config Release
```

> **注意**: 请根据通达信软件的位数选择对应的 DLL 版本。32 位通达信需使用 32 位 DLL，64 位通达信需使用 64 位 DLL。

## 主图代码

把编译好的DLL放到通达信的T0002\dlls目录，绑定为2号函数，下面的代码做成通达信主图公式。

```text
FRAC:=TDXDLL2(2,H,L,0);{标准笔}
NOTEXT画上升笔2:DRAWLINE(FRAC=-1,L,FRAC=+1,H,0),COLORYELLOW;
NOTEXT画下降笔2:DRAWLINE(FRAC=+1,H,FRAC=-1,L,0),COLORYELLOW;

BIZG:=TDXDLL2(5,FRAC,H,L);{输出BI中枢高}
BIZD:=TDXDLL2(6,FRAC,H,L);{输出BI中枢低}
BISE:=TDXDLL2(7,FRAC,H,L);{输出BI中枢开始和结束}

NOTEXT_BIZG:IF(BIZG,BIZG,DRAWNULL),COLORYELLOW;{画BI中枢高}
NOTEXT_BIZD:IF(BIZD,BIZD,DRAWNULL),COLORYELLOW;{画BI中枢低}
NOTEXT_BISE:STICKLINE(BISE,BIZD,BIZG,0,0),COLORYELLOW;{画BI中枢起始结束};

DUAN1:=TDXDLL2(3,FRAC,H,L);{计算段的端点,3改成4是1+1终结画法}
NOTEXT画上升段1:DRAWLINE(DUAN1=-1,L,DUAN1=+1,H,0), COLORFF8000;
NOTEXT画下降段1:DRAWLINE(DUAN1=+1,H,DUAN1=-1,L,0), COLORFF8000;

DUANZG1:=TDXDLL2(5,DUAN1,H,L);{输出段中枢高}
DUANZD1:=TDXDLL2(6,DUAN1,H,L);{输出段中枢低}
DUANSE1:=TDXDLL2(7,DUAN1,H,L);{输出段中枢开始和结束}

NOTEXT_DDUANZG1:IF(DUANZG1,DUANZG1,DRAWNULL),COLORFF8000;{画段中枢高}
NOTEXT_DDUANZD1:IF(DUANZD1,DUANZD1,DRAWNULL),COLORFF8000;{画段中枢低}
NOTEXT_DDUANSE1:STICKLINE(DUANSE1,DUANZD1,DUANZG1,0,0),COLORFF8000;{画段中枢起始结束};
```

## 包含K线水平虚线标记

以下代码追加在上面的笔、段、中枢主图公式末尾；不要只粘贴本节，否则不会绘制笔、段、中枢。`102-105` 输出每个已确认方向的包含组合并后的高、低价及组起止位置。每组只绘制两条水平标记，长度由该组的原始 K 线数量决定；尚未确认方向时不绘制，后续确认后回填。

```text
KXHIGH:=TDXDLL2(102,H,L,C);{包含合并后的高点}
KXLOW:=TDXDLL2(103,H,L,C);{包含合并后的低点}
KXSTART:=TDXDLL2(104,H,L,C);{包含组起点}
KXEND:=TDXDLL2(105,H,L,C);{包含组终点}

{不要用 DRAWLINE：相邻包含组没有空柱时会把前后组端点连接起来}
KXDOT:=KXHIGH>0;
NOTEXT_KX_HIGH:STICKLINE(KXDOT,KXHIGH,KXHIGH,1,0),COLORGREEN;
NOTEXT_KX_LOW:STICKLINE(KXDOT,KXLOW,KXLOW,1,0),COLORGREEN;
```

`KXDOT` 在包含组的每一根原始 K 线上绘制短水平段，确保两根或多根包含 K 线都能看见；因为没有调用 `DRAWLINE`，不同价格的连续包含组不会互相连接。不要改为 `DRAWLINE(KXHIGH>0,H,KXHIGH>0,H,0)`、`POLYLINE(KXHIGH>0,H)` 或直接输出 `KXHIGH:H`；这些画的是原始高点连线，不是包含组水平线。公式绑定 2 号 DLL 必须使用 `TDXDLL2`。更换 DLL 后退出并重新启动通达信，确保旧 DLL 不再驻留。可暂时在主图加入 `TEST2:TDXDLL2(2,H,L,0),NODRAW;` 与 `TEST102:TDXDLL2(102,H,L,C),NODRAW;`，在十字光标数据中分别检查旧笔端点及包含组高价是否有非零值；两者都为零时先检查 DLL 路径、绑定和加载状态。

### 新接口：笔级中枢

新版 `110` 输出笔端点；`210/211` 输出笔级中枢上、下沿，`213-216` 分别输出左上、左下、右上、右下边界价格。共享 K 线上的左右边界独立，旧 `212` 不再用于方框绘制。`200/201/202` 是段级中枢，不应作为笔级中枢使用。以下为绑定 2 号 DLL 的完整主图示例：

```text
BI:=TDXDLL2(110,H,L,C);
NOTEXT_UP_BI:DRAWLINE(BI=-1,L,BI=1,H,0),COLORRED;
NOTEXT_DOWN_BI:DRAWLINE(BI=1,H,BI=-1,L,0),COLORGREEN;
DRAWNUMBER(BI=1,H,H),COLORRED;
DRAWNUMBER(BI=-1,L,L),COLORGREEN;

BIZG:=TDXDLL2(210,H,L,C);
BIZD:=TDXDLL2(211,H,L,C);
LZH:=TDXDLL2(213,H,L,C);
LZD:=TDXDLL2(214,H,L,C);
RZH:=TDXDLL2(215,H,L,C);
RZD:=TDXDLL2(216,H,L,C);
{中枢区间方框：上下边和各自的起止竖边}
NOTEXT_BIZG:STICKLINE(BIZG>0,BIZG,BIZG,2,0),COLORWHITE;
NOTEXT_BIZD:STICKLINE(BIZD>0,BIZD,BIZD,2,0),COLORWHITE;
NOTEXT_ZS_LEFT:STICKLINE(LZH>0 AND LZD>0,LZD,LZH,0,0),COLORWHITE;
NOTEXT_ZS_RIGHT:STICKLINE(RZH>0 AND RZD>0,RZD,RZH,0,0),COLORWHITE;

KXHIGH:=TDXDLL2(102,H,L,C);
KXLOW:=TDXDLL2(103,H,L,C);
KXDOT:=KXHIGH>0;
NOTEXT_KX_HIGH:STICKLINE(KXDOT,KXHIGH,KXHIGH,1,0),COLORGREEN;
NOTEXT_KX_LOW:STICKLINE(KXDOT,KXLOW,KXLOW,1,0),COLORGREEN;
```

此前公式的蓝色线来自 `SEG:=TDXDLL2(120,H,L,C)` 配合 `COLORFF8000`，代表线段，不是中枢；当前线段划分尚未按完整特征序列法校验，所以默认主图不画蓝线。中枢白色方框的上下边位于 `ZG/ZD`，左右竖边由独立输出 `213-216` 给出。共享边界上两条竖线可能重叠，这是预期效果；不要与段级 `200-202` 混用。

## 社区

如果开源版不知道自己怎么动手搞定的，可以参与我的社区，我会协助你安装。

### 安装指南

- [番茄缠论插件 FqChan04 TDX 安装指南 20260405 更新](https://mp.weixin.qq.com/s/v3uchXfQLACdeEoV6CN65Q)

并且我也有全能的量化版本开发计划，可以一起研究冲浪。

### 更多

- 支持作者：https://mp.weixin.qq.com/s/xKBIlmBp9iyYg7wpLc5bPw
- 给作者充电：http://s.a0c.top/lJ1c8WZ/4KHZ
- 缠论X星球: https://t.zsxq.com/0aDUuhQC5
- 纷传圈子：http://s.a0c.top/uHvyCEy/4KHZ
- WeChat: kldcty
- QQ: 1106628276
- 微信公众号: kldctymp
