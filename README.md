
# selfmesh 技术说明

## 1. 预备知识

下表列出阅读本文需要用到的知识点。

### 1.1 数制与位运算

- **二进制 / 十六进制**：报文里每个字节用两位十六进制表示（`0xDB` = `1101 1011`）。文中 `0x` 前缀表示十六进制。
- **位与 `&`、位或 `|`、异或 `^`**：异或是加密算法里最常用的运算（`A ^ B` 表示对应位相同为 0、不同为 1）。
- **移位 `<<` `>>`**：左移一位相当于「乘以 2」。
- **循环移位 ROL/ROR**：移位时把移出去的位补到另一端，selfsec 的线性层 `ROL6` 就是 32 位循环左移 6 位。
- **大端（Big-Endian）**：多字节整数「高字节在前」。PDU 里的地址、tag 都是大端存储。

### 1.2 C 语言基础

- **结构体 `struct`**：`selfmesh_node_t` / `selfmesh_element_t` / `selfmesh_service_t` 就是用结构体描述一个节点/要素/服务（第 3 节）。
- **数组、指针、`memcpy`**：报文就是一字节数组，加解密在数组上原地操作，`memcpy` 用来复制字节块。
- **函数指针表**：服务通过 `selfmesh_service_handle_t`（init / data_handle / sche_handle 三个函数指针）注册分发（第 3.3 节）。
- **`uint8` / `uint16`**：无符号 8 位（1 字节）/ 16 位（2 字节）整数。

### 1.3 网络 / 协议基础

- **地址**：发送方（源 `src`）和接收方（目的 `dst`）各有一个地址，报文里显式携带（第 4 节）。
- **单播 / 组播 / 广播**：发给一个节点 = 单播；发给一组节点 = 组播；发给所有人 = 广播（第 4.3 节地址空间）。
- **跳数（TTL / hop）**：报文每被转发一次，剩余跳数减 1，减到 0 就丢弃，防止报文在网络里无限传播（第 4.1 节）。
- **序号与去重（防重放）**：发送方给每条消息编号，接收方记录「每个源上一次收到的序号」，重复的丢弃（第 6 节）。
- **报文 / 头部 / 载荷**：一条消息 = 固定长度的「头部」（地址、序号等）+「载荷」（真正的数据）。

### 1.4 BLE 基础

- **广播（Advertising）**：设备周期性向空中发小包，附近设备都能收到，无需先建立连接。selfmesh 的 mesh 通信走的就是广播承载（`SELFMESH_CHANNEL_ADV`）。
- **连接（Connection）与 GATT**：配网时手机/上位机与设备建立一对一连接，通过 GATT 服务读写配置（`SELFMESH_CHANNEL_PBLE`）。
- **SoftDevice**：Nordic 提供的蓝牙协议栈固件，应用通过它调用 BLE 能力。

### 1.5 密码学入门

- **明文 / 密文 / 密钥**：明文是原始数据，密钥是保密的「密码」，密文是加密后的乱码。解密需要密钥。
- **分组密码（Block Cipher）**：把数据切成固定长度的小块，逐块加密。AES 每块 16 字节，selfsec 每块 4 字节（第 7、10 节）。
- **S 盒（S-box）**：一个「查表替换」的非线性变换，输入一个字节、输出一个字节，是混淆（confusion）的来源。
- **混淆（Confusion）与扩散（Diffusion）**：混淆 = 让密文和密钥的关系变复杂（靠 S 盒）；扩散 = 让明文一个比特的变化波及多个输出。
- **工作模式（CBC）**：分组密码不能一块一块独立加密（相同明文会产生相同密文），CBC 把上一块的密文和本块明文先异或再加密（第 7.5 节）。
- **IV（初始向量）**：CBC 加密第一块时，还没有「上一块密文」，用 IV 充当。selfsec 的 IV 固定为 0。
- **完整性 / MAC（消息认证码）**：加密只保证「别人看不懂」，不保证「内容没被篡改」。MAC 是防篡改的附加校验。selfsec 只有 magic 字节做弱校验，没有真正的 MAC（第 9 节）。


---

## 2. 节点类型

selfmesh 中「节点（Node）」指一个带唯一单播地址、参与组网通信的物理设备。节点类型可以从三个维度划分：

### 2.1 按应用角色分

| 角色 | 工程目录 | 承载服务 | 说明 |
|------|----------|----------|------|
| 客户端（client） | `..._client0100` | `service_0000`（场景客户端配置服务） | 用于配置场景、下发控制命令 |
| 服务端（server） | `..._lightctl_server0200` | `service_0001`（灯光控制服务） | 实际执行灯光控制 |

> 命名规律：`server` 版本号为奇数（V211、V213…），`client` 版本号为偶数（V212、V214…）。

### 2.2 按启动模式分

节点启动时由 `start_mode` 决定进入哪种模式（`selfmesh_common.h`）：

| 模式 | 值 | 说明 |
|------|----|------|
| 配网模式 `SELFMESH_START_MODE_CONFIG` | 0 | 打开可连接广播，接受配置（设地址、烧密钥等） |
| 正常模式 `SELFMESH_START_MODE_NORMAL` | 1 | 只工作在 mesh 广播承载，正常收发业务数据 |

### 2.3 按通道类型分

| 通道 | 值 | 说明 |
|------|----|------|
| `SELFMESH_CHANNEL_PBLE` | 0x01 | BLE 外设通道（可连接，用于配网/手机直连） |
| `SELFMESH_CHANNEL_ADV` | 0x02 | mesh 广播承载（组网后正常通信走这里） |
| `SELFMESH_CHANNEL_CBLE` | 0x04 | BLE 中心通道（已定义，未使用） |

### 2.4 节点的能力

- **转发（Relay）**：每个节点收到不是发给自己的消息时，可递减 hop 后转发到下一跳（`selfmesh_util_send_msg_next_hop`）。
- **代理（Proxy）**：可通过 `OPEN_PROXY_CHANNEL` 指令打开 GATT 代理，桥接手机等非 mesh 设备。
- **订阅（Subscription）**：每个要素最多订阅 3 个组播地址。

> 对照标准 BLE Mesh 的「中继/好友/低功耗/代理/配网器」五类节点，selfmesh 简化掉了 Friend（好友）和 Low Power（低功耗）节点，保留了中继、代理与配网能力。

---

## 3. 节点内部结构与要素

selfmesh 采用 **Node（节点）→ Element（要素）→ Service（服务）** 三层结构，对应标准 Mesh 的「网络层 → 接入层 → 模型层」。

```
┌──────────────────────────── Node（节点，唯一单播地址）────────────────────────────┐
│  magic_value / start_mode / net_key / unicast_addr / snd_indx / msg_history        │
│                                                                                    │
│  ┌────────────────── Element 0 ──────────────────┐   ... 最多 4 个 (element_a[4])  │
│  │  magic_value / pub_addr / sub_addr[3] / app_key │                                │
│  │  ┌──────────── Service ────────────┐            │                              │
│  │  │  magic_value / service_uuid      │            │                              │
│  │  │  service_cfg[40]                 │            │                              │
│  │  └──────────────────────────────────┘            │                              │
│  └──────────────────────────────────────────────────┘                              │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 3.1 Node（节点）

结构体 `selfmesh_node_t`（`selfmesh_node.h`）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `magic_value` | uint16 | 节点配置有效标志（`0x5A5A`） |
| `start_mode` | uint8 | 启动模式（配网/正常） |
| `net_key[8]` | uint8 | 网络密钥 NETKEY（网络层加密整包） |
| `unicast_addr` | uint16 | 本节点单播地址 |
| `element_a[4]` | element_t | 要素数组，最多 4 个 |
| `snd_indx` | uint8 | 发送消息序号（0–254 循环） |
| `msg_history[]` | — | 每个源地址最近一次收到的消息序号（防重放） |

节点级命令由 `s_selfmesh_node_handle` 处理（tag 0x0000–0x000B）。

### 3.2 Element（要素）

结构体 `selfmesh_element_t`（`selfmesh_element.h`）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `magic_value` | uint16 | 要素有效标志（`0x5A5A`） |
| `pub_addr` | uint16 | 发布地址（默认发布目标） |
| `sub_addr[3]` | uint16 | 订阅地址表（最多 3 个组播地址） |
| `app_key[8]` | uint8 | 应用密钥 APPKEY（应用层加密 appdata） |
| `service_a[1]` | service_t | 服务数组（每个要素最多 1 个服务） |

- 一个节点最多 **4 个要素**（`SELFMESH_MAX_ELEMENT_CNT = 4`）。
- 要素在 PDU 中的 `element` 字段作为数组下标使用，因此有效值只有 **0–3**，另有特殊值：
  - `0x00` = 配网要素（`SELFMESH_ELEMENT_ID_PROVISION`）
  - `0xFE` = 所有要素（`SELFMESH_ELEMENT_ID_ALL`，广播到全部要素）
  - `0xFF` = 无效/节点级（`SELFMESH_ELEMENT_ID_INVALIDE`，表示节点级命令）

要素级命令由 `selfmesh_element_handle` 处理（tag 0x0080–0x008B）。

### 3.3 Service（服务）

结构体 `selfmesh_service_t`（`selfmesh_service.h`）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `magic_value` | uint16 | 服务有效标志（`0x5A5A`） |
| `service_uuid` | uint16 | 服务 UUID，标识服务类型 |
| `service_cfg[40]` | uint8 | 服务配置数据（最多 40 字节） |

服务 UUID 枚举（`selfmesh_service_uuid_e`）：

| UUID | 值 | 服务 | 对应文件 |
|------|----|------|----------|
| `SELEMESH_SERVICE_UUID0` | 0 | 场景客户端配置服务（scene client setup server） | `selfmesh_service_0000.c` |
| `SELEMESH_SERVICE_UUID1` | 1 | 灯光控制服务（light control service） | `selfmesh_service_0001.c` |

服务通过函数指针表 `selfmesh_service_handle_t`（init / data_handle / sche_handle）注册与分发，业务命令（tag 0x0100 及以上）最终在这里处理。

---

## 4. 承载数据格式

### 4.1 网络 PDU（20 字节）

`SELFMESH_UD_BUF_SIZE = 20`，承载 PDU 固定 20 字节：

```
偏移  0      1           2      3     4     5     6     7     8         19
     ┌──────┬───────────┬──────┬─────┬─────┬─────┬─────┬─────┬─────────────┐
     │magic │ hop | flag │index │ src │ src │ dst │ dst │element│  appdata    │
     │ (8b) │ (4b)|(4b)  │ (8b) │ MSB │ LSB │ MSB │ LSB │ (8b) │  (96b=12B)  │
     └──────┴───────────┴──────┴─────┴─────┴─────┴─────┴─────┴─────────────┘
```

各字段说明（`selfmesh_common.h` 的偏移宏）：

| 字段 | 偏移 | 长度 | 说明 |
|------|------|------|------|
| `magic` | 0 | 1B | 消息魔数，固定 `0xDB`（`SELFMESH_MSG_MAGIC_VAL_M`），解密后作为弱校验 |
| `hop` | 1 | 高 4bit | 剩余跳数（TTL），最大 `SELFMESH_MSG_MAX_HOP = 10`，每转发一跳减 1 |
| `flag` | 1 | 低 4bit | 加密标志：`0x02`=NETKEY 加密整包，`0x01`=APPKEY 加密 appdata |
| `index` | 2 | 1B | 消息序号（0–254 循环），用于接收端去重/防重放 |
| `src` | 3–4 | 2B | 源单播地址（大端） |
| `dst` | 5–6 | 2B | 目的地址（大端） |
| `element` | 7 | 1B | 目的要素 ID（0–3 有效，0xFE=所有，0xFF=节点级） |
| `appdata` | 8–19 | 12B | 应用数据（TLV 格式），`SELFMASH_APP_DATA_LEN = 12` |

### 4.2 appdata 的 TLV 格式（12 字节内）

appdata 采用 **Tag + Length + Value** 结构：

```
┌───────────────┬───────────────┬──────────────────────────────┐
│   tag (2B)    │   len (1B)    │      value (len 字节)        │
└───────────────┴───────────────┴──────────────────────────────┘
```

- `tag`：命令号（2 字节，大端），分三段：
  - `0x0000 – 0x00FF`：网络管理（节点级）命令
  - `0x0080 – 0x008F`：要素管理命令
  - `0x0100 及以上`：应用服务命令
- `len`：value 的字节长度
- `value`：命令参数

示例（`SET_NETKEY`，tag 0x0004）：`00 04 08 <8字节netkey>`；其响应 `00 05 01 00`（tag 0x0005，len 1，结果 0=成功）。

### 4.3 地址空间

| 类型 | 范围 | 说明 |
|------|------|------|
| 单播 | `0x0000 – 0x00F0` | 每个节点一个；`0x0000` 为配网地址，`0x00FF` 无效 |
| 组播 | `0xD000 – 0xDFF0` | `0xD000` = 全网广播，`0xDFFF` 无效 |
| 虚拟 | `0xE000 – 0xEFF0` | 已定义地址段，当前未实现 |
| 特殊 | `0xFFF0` / `0xFFFF` | 当前地址 / 无效地址 |

### 4.4 命令清单（tag）

**节点级（0x0000–0x000B）**

| Tag | 命令 | 说明 |
|-----|------|------|
| 0x0000/0x0001 | SET/GET_LOC_ADDR | 设置/读取本节点单播地址 |
| 0x0004/0x0005 | SET/GET_NETKEY | 设置/读取网络密钥 |
| 0x0008/0x0009 | GET_PIDVID | 读取产品 PID/VID |
| 0x000A | SYSTEM_RESET | 系统复位（可选复位到配网/正常模式） |
| 0x000B | OPEN_PROXY_CHANNEL | 打开代理通道 |

**要素级（0x0080–0x008B）**

| Tag | 命令 | 说明 |
|-----|------|------|
| 0x0080/0x0081 | SET/GET_APPKEY | 设置/读取应用密钥 |
| 0x0084/0x0085 | SET/GET_PUB_ADDR | 设置/读取发布地址 |
| 0x0088/0x0089 | SET/GET_SUB_ADDR | 设置/读取订阅地址（最多 3 个） |

**应用服务级（0x0100 及以上）**

| Tag | 命令 | 说明 |
|-----|------|------|
| 0x0100/0x0101 | NODE_IDENTIFY | 节点识别 |
| 0x0102–0x0105 | SIMPLE_CLIENT_SET/GET | 简易客户端配置读写 |
| 0x0106–0x0110 | SCENE_LIST_* | 场景列表/参数管理、场景发布 |
| 0x0111 | LIGHTNESS_PUBLISH | 发布亮度命令（up/down/switch/on/off） |

---


## 5. selfsec 加解密流程

selfsec 是自研的精简分组密码，作为 selfmesh 的加密算法。

### 5.1 基本参数

| 参数 | 值 |
|------|----|
| 分组长度 | 4 字节 |
| 密钥长度 | 8 字节 |
| 轮数 | 8 轮（每轮 2 个半轮，共 16 个半轮） |
| S 盒 | 直接借用 AES 的 16×16 S 盒 |
| 线性层 | ROL6（循环左移 6 位） |
| 扩散层 | MixColumns（AES MDS 矩阵，**V213/V214 新增**） |
| 密钥编排 | 8×8 位矩阵转置（线性） |
| 工作模式 | CBC，IV = 0 |

### 5.2 密钥编排（子密钥生成）

```
主密钥 key (8 字节)
   │
   ▼  s_8s8_matrix_transpose (转置 T)
subkey[0] = T(key)
   │
   ▼  T
subkey[1] = T²(key)
   │ ... 共 8 个子密钥
   ▼
subkey[7] = T⁸(key)
```

- 转置 `T` 是一个 64 比特的位置换，**阶为 16**（4 个长度 16 的循环），因此 `T^16 = 恒等`，前 8 个子密钥互不相同。
- 每个子密钥 8 字节，恰好供一轮（2 个半轮）使用。

### 5.3 轮函数（加密）

每个半轮执行四步：

```
AddRoundKey → ROL6 → SubBytes
```

具体（`s_encrypt_block`）：

1. **AddRoundKey**：`data ^= subkey[i]`（4 字节）
2. **ROL6**：把 4 字节当 32 位字，循环左移 6 位
3. **SubBytes**：逐字节查 AES S 盒（非线性混淆）

16 个半轮依次消耗 `subkey[0][0..3] → subkey[0][4..7] → subkey[1][0..3] → … → subkey[7][4..7]`。

### 5.4 轮函数（解密）

解密是加密的严格逆序：

```
InvSubBytes → ROR6 → AddRoundKey
```

子密钥从后往前消耗。

### 5.5 工作模式（CBC）

```
加密：  C[i] = E(P[i] ⊕ IV[i])，IV[0]=0，IV[i]=C[i-1]
解密：  P[i] = D(C[i]) ⊕ IV[i]
```

- IV 固定为 0，无 nonce/序号混入 —— 相同明文产生相同密文（教学上需指出这是弱点）。
- selfsec 只提供**机密性**，没有完整性/认证（无 MAC）。

> 注：`selfmesh_util.c` 中 APPKEY/NETKEY 加密目前被注释（`//temp modify for not encrypt`），仅 `send_msg_next_hop` 转发路径的 NETKEY 加密生效。当前 build 源节点应用数据为明文，属调试期临时状态。

---


## 6. selfsec 与 AES 对比

| 维度 | selfsec | AES |
|------|---------|-----|
| 分组长度 | 4 字节 | 16 字节 |
| 密钥长度 | 8 字节 | 16 / 24 / 32 字节 |
| 轮数 | 8 轮（16 半轮） | 10 / 12 / 14 轮 |
| S 盒（混淆） | 借用 AES S 盒 | 自研：GF(2^8) 逆 + 仿射变换 |
| 行移位（扩散） | ROL6（32 位循环左移） | ShiftRows（按行循环左移） |
| 列混合（扩散） | 无 | MixColumns |
| 密钥编排 | 线性矩阵转置（阶 16） | 非线性（RotWord + SubWord + Rcon） |
| 工作模式 | CBC，IV = 0 | 通常 CCM / GCM |
| 完整性/认证 | 无（仅 magic 弱校验） | CCM 模式自带 MAC |
| 安全强度 | 学习级 | 工业标准 |

---

## 7. 附录 A：AES 详细说明

AES（Advanced Encryption Standard，Rijndael 算法）是分组密码标准，分组固定 128 位（16 字节）。

### 7.1 基本参数

| 密钥长度 | 轮数 |
|----------|------|
| 128 位（AES-128） | 10 轮 |
| 192 位（AES-192） | 12 轮 |
| 256 位（AES-256） | 14 轮 |

### 7.2 状态表示

16 字节明文按列优先填入 4×4 字节矩阵（State）：

```
  a0  a4  a8  a12
  a1  a5  a9  a13
  a2  a6  a10 a14
  a3  a7  a11 a15
```

### 7.3 四个变换

**1) SubBytes（字节替换，唯一非线性）**

对每个字节：先求 GF(2^8) 乘法逆元，再作仿射变换：

```
y = A·(x⁻¹) ⊕ c
```

提供「混淆（confusion）」，是密码强度的核心来源。

**2) ShiftRows（行移位）**

第 i 行循环左移 i 字节（i = 0,1,2,3），提供字节级扩散。

**3) MixColumns（列混合）**

每列左乘 MDS 矩阵：

```
|02 03 01 01|   |a0|
|01 02 03 01|   |a1|
|01 01 02 03|   |a2|
|03 01 01 02|   |a3|
```

其中乘法在 GF(2^8) 上进行，`02` 倍即 `xtime`：

```
xtime(a) = (a << 1) ^ ((a >> 7) * 0x1B)
```

该矩阵是 **MDS 矩阵**，分支数 = 5（1 个输入字节改变可影响 4 个输出字节），是 AES 的「扩散（diffusion）」核心。

**4) AddRoundKey（轮密钥加）**

State 与轮密钥逐字节异或。

> 每轮顺序：`SubBytes → ShiftRows → MixColumns → AddRoundKey`；**最后一轮省略 MixColumns**（为保持解密对称）。

### 7.4 密钥编排（Key Schedule）

把 128 位密钥扩展成 44 个字（每字 4 字节），用 `RotWord`（字循环左移 1 字节）、`SubWord`（S 盒替换）、`Rcon[i]`（轮常量）做**非线性**扩展。密钥编排的非线性是 AES 抗相关密钥攻击的关键。

### 7.5 GF(2^8) 域运算

- 字节视为系数在 GF(2) 上的 8 次多项式。
- 加法 = 异或；乘法 = 多项式乘后模不可约多项式 `x⁸+x⁴+x³+x+1`（即 `0x11B`）。
- 实现用「移位相加」（俄式乘法）或查表。（完整入门见第 8 节。）

### 7.6 安全设计思想

AES 采用 **宽轨迹策略（wide trail strategy）**：通过 SubBytes（S 盒）+ ShiftRows/MixColumns（MDS）的组合，使连续几轮的差分/线性活跃 S 盒数量快速增加，从而抵抗差分与线性密码分析。分支数 5 的 MDS 矩阵保证 4 轮内活跃 S 盒数 ≥ 25。

### 7.7 selfsec 与 AES 的结构对应

| AES 步骤 | selfsec 对应 | 备注 |
|----------|--------------|------|
| SubBytes | AES S 盒 | selfsec 直接借用 |
| ShiftRows | ROL6 | selfsec 用 32 位循环左移替代按行移位 |
| MixColumns | MixColumns（V213/V214 新增） | 完全相同的 MDS 矩阵 |
| AddRoundKey | AddRoundKey | 相同 |
| 密钥编排（非线性） | 矩阵转置（线性） | selfsec 刻意简化 |
| CCM（机密性+认证） | CBC + 无 MAC | selfsec 省略了完整性 |

---

## 8. 附录 B：术语表

按首字母（英文）/ 拼音（中文）排序，供快速查阅。

| 术语 | 英文 | 一句话解释 |
|------|------|-----------|
| appdata | application data | PDU 最后 12 字节，承载 TLV 格式的应用数据 |
| APPKEY | application key | 应用密钥，8 字节，用于加密 appdata（应用层） |
| 分组密码 | block cipher | 把数据切固定长度小块逐块加密的算法 |
| CBC | Cipher Block Chaining | 分组密码工作模式，上一块密文与本块明文先异或再加密 |
| 扩散 | diffusion | 让明文一个比特的变化波及多个输出的特性 |
| dst | destination | 目的地址字段（2 字节） |
| Element | element | 要素，节点内的子单元，持有 app_key/发布/订阅地址与服务 |
| flag | flag | PDU 第 1 字节低 4 位，标记是否 NETKEY/APPKEY 加密 |
| GF(2^8) | Galois Field 2^8 | 256 元素的有限域，字节乘法/加法的数学基础 |
| hop | hop | 剩余跳数（TTL），每转发一次减 1，最大 10 |
| index | index | 消息序号，用于接收端去重/防重放 |
| IV | Initialization Vector | CBC 首块加密用的初始向量，selfsec 固定为 0 |
| 密钥编排 | key schedule | 由主密钥生成各轮子密钥的过程 |
| MAC | Message Authentication Code | 消息认证码，防篡改校验；selfsec 未实现 |
| magic | magic | PDU 首字节固定值 0xDB，解密后作弱校验 |
| MixColumns | — | 列混合，GF(2^8) 上的 MDS 矩阵乘法，提供扩散 |
| NETKEY | network key | 网络密钥，8 字节，用于加密整包（网络层） |
| Node | node | 节点，带唯一单播地址的物理设备 |
| nonce | — | 一次性随机数，避免相同明文产生相同密文；selfsec 未使用 |
| PDU | Protocol Data Unit | 协议数据单元，这里指 20 字节网络报文 |
| 中继 | relay | 节点把不是发给自己的消息转发到下一跳 |
| ROL / ROR | rotate left/right | 循环左移/右移，移出的位补到另一端 |
| S 盒 | S-box | 查表替换的非线性变换，混淆的核心来源 |
| Service | service | 服务，挂在要素下，按 service_uuid 分派业务命令 |
| src | source | 源地址字段（2 字节） |
| SubBytes | — | 字节替换，逐字节查 S 盒 |
| 订阅 | subscription | 要素登记感兴趣的组播地址（最多 3 个） |
| tag | tag | TLV 里的命令号（2 字节），标识「要做什么」 |
| TLV | Tag-Length-Value | 「标签-长度-值」三元组，appdata 的组织格式 |
| TTL | Time To Live | 存活时间，即剩余跳数，防止无限转发 |
| 单播/组播/广播 | unicast/multicast/broadcast | 发给一个/一组/所有节点的地址类型 |

---

*（完）*
