# ChanlunX New TDX Interface

This document describes the new specification-driven function range. All functions use the same call shape:

```text
TDXDLL2(FuncNo, H, L, C)
```

Each function returns one `float` series. Empty bars are `0`.

## Function Map

笔级中枢要求已确认笔按索引、方向及共享端点价格连续。离开笔本身不直接终结中枢；第一条已确认反向回试严格位于原区间外时才终结，触及边界仍算重叠。候选笔不延伸方框。内部记录构成笔索引、反向笔提供的确认依据位置和终结依据位置；这些位置不是分型右侧 K 线的精确确认时刻。与前一中枢重叠的后续结构保留为关联候选，不经 `210-216` 画成独立已确认方框；高级别递归扩展尚未实现。

当前版本暂时移除新核心线段识别：`120`、段级中枢 `200-205` 及其依赖信号 `300/400` 返回全零。笔 `110`、包含标记 `102-105`、笔级中枢 `210-216` 和 MACD `320` 保持可用。编号保留供公式兼容；旧接口 `3/4` 的独立历史算法未删除，不属于新核心计算链。

| Function | Output |
| --- | --- |
| `100` | Merged K-line direction: `1=up`, `-1=down` |
| `101` | Fractal marker: `1=top`, `-1=bottom` |
| `102` | Containment group merged high line (initial containment defaults upward) |
| `103` | Containment group merged low line (initial containment defaults upward) |
| `104` | Containment group start marker |
| `105` | Containment group end marker |
| `110` | Stroke endpoint marker: `1=top`, `-1=bottom` |
| `120` | Confirmed segment endpoint marker: `1=top`, `-1=bottom`; pending candidates output `0` |
| `200` | Pivot `ZG` |
| `201` | Pivot `ZD` |
| `202` | Pivot start/end marker: `1=start`, `2=end` |
| `203` | Pivot direction: `1=up`, `-1=down` |
| `204` | Trend kind: `1=up`, `-1=down`, `2=balance` |
| `205` | Trend completion: `1=not formed`, `2=forming`, `3=can complete`, `4=completed`, `5=extending` |
| `210` | Stroke-level pivot `ZG` (three overlapping strokes) |
| `211` | Stroke-level pivot `ZD` |
| `212` | Stroke-level pivot boundary: `1=start`, `2=end` |
| `213` | Stroke pivot left boundary upper price |
| `214` | Stroke pivot left boundary lower price |
| `215` | Stroke pivot right boundary upper price |
| `216` | Stroke pivot right boundary lower price |
| `300` | Third buy/sell marker: `3=third buy`, `-3=third sell` |
| `320` | Default MACD histogram |
| `400` | Operation state derived from implemented buy/sell signals: `1=hold`, `-1=cash`, `0=neutral` |
| `900` | Debug merged K-line id |

`120` uses confirmed strokes to form a three-stroke segment candidate, then standardizes the opposite-direction feature sequence. A no-gap feature fractal confirms the endpoint; a gap requires a confirming fractal in the reverse feature sequence. Candidates are not projected by `120` and do not contribute to segment-level pivots (`200-205`). Legacy functions `3` and `4` retain their separate drawing algorithms and are not interchangeable with `120`.

## Example

```text
FRAC:=TDXDLL2(101,H,L,C);
NOTEXT_TOP:IF(FRAC=1,H,DRAWNULL),COLORRED;
NOTEXT_BOTTOM:IF(FRAC=-1,L,DRAWNULL),COLORGREEN;

BI:=TDXDLL2(110,H,L,C);
NOTEXT_UP_BI:DRAWLINE(BI=-1,L,BI=1,H,0),COLORRED;
NOTEXT_DOWN_BI:DRAWLINE(BI=1,H,BI=-1,L,0),COLORGREEN;
DRAWNUMBER(BI=1,H,H),COLORRED;
DRAWNUMBER(BI=-1,L,L),COLORGREEN;

SEG:=TDXDLL2(120,H,L,C);
NOTEXT_UP_SEG:DRAWLINE(SEG=-1,L,SEG=1,H,0),COLORFF8000;
NOTEXT_DOWN_SEG:DRAWLINE(SEG=1,H,SEG=-1,L,0),COLORFF8000;

ZG:=TDXDLL2(200,H,L,C);
ZD:=TDXDLL2(201,H,L,C);
ZSSE:=TDXDLL2(202,H,L,C);
NOTEXT_ZG:IF(ZG,ZG,DRAWNULL),COLORYELLOW;
NOTEXT_ZD:IF(ZD,ZD,DRAWNULL),COLORYELLOW;
NOTEXT_ZS:STICKLINE(ZSSE,ZD,ZG,0,0),COLORYELLOW;

{笔级中枢。200-202 是段级中枢，不适合直接代替笔级中枢。}
BIZG:=TDXDLL2(210,H,L,C);
BIZD:=TDXDLL2(211,H,L,C);
LZH:=TDXDLL2(213,H,L,C);
LZD:=TDXDLL2(214,H,L,C);
RZH:=TDXDLL2(215,H,L,C);
RZD:=TDXDLL2(216,H,L,C);
NOTEXT_BIZG:STICKLINE(BIZG>0,BIZG,BIZG,2,0),COLORWHITE;
NOTEXT_BIZD:STICKLINE(BIZD>0,BIZD,BIZD,2,0),COLORWHITE;
NOTEXT_ZS_LEFT:STICKLINE(LZH>0 AND LZD>0,LZD,LZH,0,0),COLORWHITE;
NOTEXT_ZS_RIGHT:STICKLINE(RZH>0 AND RZD>0,RZD,RZH,0,0),COLORWHITE;

BS3:=TDXDLL2(300,H,L,C);
DRAWTEXT(BS3=3,L,'3B'),COLORGREEN;
DRAWTEXT(BS3=-3,H,'3S'),COLORRED;

KXHIGH:=TDXDLL2(102,H,L,C);
KXLOW:=TDXDLL2(103,H,L,C);
KXSTART:=TDXDLL2(104,H,L,C);
KXEND:=TDXDLL2(105,H,L,C);
KXDOT:=KXHIGH>0;
NOTEXT_KX_HIGH:STICKLINE(KXDOT,KXHIGH,KXHIGH,1,0),COLORGREEN;
NOTEXT_KX_LOW:STICKLINE(KXDOT,KXLOW,KXLOW,1,0),COLORGREEN;
```

`DRAWLINE` 不适用于相邻包含组：当上一组的结束柱紧接下一组的开始柱时，通达信会按序连接端点。这里改用 `STICKLINE` 在每一根组内 K 线上画短水平段，因此两根或多根包含 K 线都可见，也不会跨不同价格的组连线。该示例独立于笔段中枢；`200/201` 当前仅由三个以上已识别的段生成中枢，不能代替原主图公式的笔中枢 `5/6/7`。若要保留原有笔、段、笔中枢和段中枢，请使用 README 中的 `1-9` 主图公式，再追加本节 `102-105` 的包含组线。

当前 `buildStrokePivots` 采用连续三笔重叠形成笔级中枢：`ZG` 为三笔高点的最小值，`ZD` 为三笔低点的最大值；无重叠则不形成中枢。后续笔与已确定的 `[ZD,ZG]` 重叠时延长方框，不改变初始上下沿。主图推荐直接使用 README 的新接口公式；本节上方混合列出的段级接口为独立示例，不要与笔级中枢同时绘制。
