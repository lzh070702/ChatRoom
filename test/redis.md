# Redis 基础命令笔记

## 一、String（字符串）

String 是 Redis 最基础的类型，一个 key 对应一个 value。value 可以是字符串、整数或浮点数，最大 512MB。

### 基本读写

```bash
# 设置 key 的值（已存在则覆盖）
SET key value

# 获取 key 的值，key 不存在返回 nil
GET key

# 仅当 key 不存在时设置（分布式锁常用）
SETNX key value
# 等价于: SET key value NX

# 设置 key 并同时指定过期时间（秒）
SETEX key seconds value
# 等价于: SET key value EX seconds

# 设置 key 并同时指定过期时间（毫秒）
PSETEX key milliseconds value
# 等价于: SET key value PX milliseconds

# 获取 key 的旧值并设置新值
GETSET key value
# （Redis 6.2.0 起已废弃，建议用 GETEX 或 SET 搭配 GET）
```

### SET 命令的扩展参数（Redis 2.6.12+）

```bash
SET key value [EX seconds] [PX milliseconds] [NX|XX] [KEEPTTL] [GET]

# EX seconds    — 过期时间（秒）
# PX milliseconds — 过期时间（毫秒）
# NX            — 仅当 key 不存在时设置
# XX            — 仅当 key 已存在时设置
# KEEPTTL       — 保留原有 TTL（Redis 6.0+）
# GET           — 返回旧值，若 key 不存在返回 nil（Redis 6.2+）

# 示例：分布式锁（获取锁）
SET lock:order:1001 unique_value NX EX 30
# 仅当 lock:order:1001 不存在时设置成功，30 秒自动释放
```

### 批量操作

```bash
# 批量设置
MSET key1 value1 key2 value2 key3 value3

# 批量设置（仅当所有 key 都不存在时）
MSETNX key1 value1 key2 value2

# 批量获取（不存在的 key 返回 nil）
MGET key1 key2 key3
```

### 字符串操作

```bash
# 获取值的长度
STRLEN key

# 追加字符串到末尾，返回追加后的总长度
APPEND key "追加的内容"

# 获取子字符串（索引从 0 开始，支持负数从末尾算）
GETRANGE key 0 4       # 获取第 0 到第 4 个字符
GETRANGE key 0 -1      # 获取全部
GETRANGE key -3 -1     # 获取最后 3 个字符

# 覆盖指定位置的子字符串
SETRANGE key offset value
# 示例: SET msg "hello world" → SETRANGE msg 6 "Redis" → "hello Redis"
```

### 计数器操作

```bash
# 原子递增 1，返回递增后的值
INCR key
# 如果 key 不存在则先初始化为 0 再加 1，返回 1

# 原子递增指定值
INCRBY key 10

# 原子递增浮点数
INCRBYFLOAT key 0.5

# 原子递减 1
DECR key

# 原子递减指定值
DECRBY key 10
```

### 过期与生存时间

```bash
# 设置过期时间（秒）
EXPIRE key 60

# 设置过期时间（毫秒）
PEXPIRE key 60000

# 设置在某个 Unix 时间戳过期（秒）
EXPIREAT key 1700000000

# 设置在某个 Unix 时间戳过期（毫秒）
PEXPIREAT key 1700000000000

# 查看剩余生存时间（秒），不存在返回 -2，永不过期返回 -1
TTL key

# 查看剩余生存时间（毫秒）
PTTL key

# 移除过期时间，变为永久
PERSIST key
```

### 获取并设置过期

```bash
# 获取值并设置新的过期时间（秒）
GETEX key EX 60

# 获取值并移除过期时间（变永久）
GETEX key PERSIST

# 获取值并删除 key（等价于 GET + DEL 的组合原子操作）
GETDEL key
```

---

## 二、List（列表）

List 是一个双向链表，元素有序可重复。可以从两端压入和弹出，适合做栈、队列、阻塞队列。

### 插入元素

```bash
# 从左侧（头部）插入一个或多个元素
LPUSH list_key element1 element2 element3
# 如果 key 不存在则创建空列表再插入
# 插入后顺序：element3 element2 element1（后插入的在前面）

# 从右侧（尾部）插入一个或多个元素
RPUSH list_key element1 element2 element3
# 插入后顺序：element1 element2 element3（按插入顺序）

# 仅当 key 存在时才从左侧插入
LPUSHX list_key element

# 仅当 key 存在时才从右侧插入
RPUSHX list_key element

# 在指定元素 pivot 之前插入新元素
LINSERT list_key BEFORE pivot new_element

# 在指定元素 pivot 之后插入新元素
LINSERT list_key AFTER pivot new_element
# 如果 pivot 不存在返回 -1，如果有多个匹配的 pivot 只插入第一个找到的
```

### 弹出元素

```bash
# 从左侧弹出一个元素（返回并删除）
LPOP list_key

# 从左侧弹出多个元素（Redis 6.2+）
LPOP list_key 3

# 从右侧弹出一个元素
RPOP list_key

# 从右侧弹出多个元素（Redis 6.2+）
RPOP list_key 3

# 阻塞式左侧弹出
BLPOP list_key timeout
# timeout 为 0 表示无限等待
# 返回格式: [key, value] 或 nil（超时）

# 阻塞式右侧弹出
BRPOP list_key timeout

# 从 source 的右侧弹出，压入 destination 的左侧
RPOPLPUSH source destination
# 原子操作，常用于可靠队列

# RPOPLPUSH 的阻塞版本（Redis 6.2+）
BRPOPLPUSH source destination timeout

# 从 source 右侧弹出，压入 destination 左侧（Redis 6.2+，方向可选）
LMOVE source destination LEFT RIGHT

# LMOVE 的阻塞版本（Redis 6.2+）
BLMOVE source destination LEFT RIGHT timeout
```

### 查看与修改

```bash
# 获取指定范围的元素（索引支持负数，-1 表示最后一个）
LRANGE list_key 0 -1      # 获取全部
LRANGE list_key 0 4       # 获取前 5 个
LRANGE list_key -3 -1     # 获取最后 3 个

# 通过索引获取元素
LINDEX list_key 0         # 第一个
LINDEX list_key -1        # 最后一个

# 设置指定索引的元素值
LSET list_key 0 new_value

# 获取列表长度
LLEN list_key
```

### 删除元素

```bash
# 删除指定值的元素
LREM list_key count value
# count > 0: 从左到右删除 count 个值为 value 的元素
# count < 0: 从右到左删除 |count| 个值为 value 的元素
# count = 0: 删除所有值为 value 的元素

# 只保留指定范围的元素，其余删除
LTRIM list_key 0 99       # 只保留前 100 个
LTRIM list_key -10 -1     # 只保留最后 10 个
```

---

## 三、Set（集合）

Set 是一个无序、不重复的字符串集合，底层是哈希表，支持集合间运算（交、并、差）。

### 基本操作

```bash
# 添加一个或多个元素（重复元素自动忽略）
SADD set_key member1 member2 member3

# 删除一个或多个元素
SREM set_key member1 member2

# 随机弹出一个元素（返回并删除）
SPOP set_key

# 随机弹出多个元素（Redis 3.2+）
SPOP set_key 3

# 随机获取元素（不删除）
SRANDMEMBER set_key         # 随机 1 个
SRANDMEMBER set_key 3       # 随机 3 个（不重复）
SRANDMEMBER set_key -3      # 随机 3 个（可能重复）

# 将 member 从 source 移动到 destination
SMOVE source destination member
# 原子操作，如果 source 中不存在或 destination 已存在则不做任何操作
```

### 查询操作

```bash
# 获取集合中所有元素
SMEMBERS set_key
# 注意：元素很多时可能阻塞，建议用 SSCAN 代替

# 判断元素是否在集合中
SISMEMBER set_key member
# 存在返回 1，不存在返回 0

# 获取集合元素个数
SCARD set_key

# 渐进式遍历集合（生产环境推荐代替 SMEMBERS）
SSCAN set_key cursor [MATCH pattern] [COUNT count]
# cursor 初始为 0，返回新的游标，游标为 0 时遍历结束
# 示例: SSCAN myset 0 MATCH user:* COUNT 100
```

### 集合运算

```bash
# 交集（多个集合共有的元素）
SINTER set1 set2 set3 ...

# 交集并存储到目标集合
SINTERSTORE dest_set set1 set2 set3 ...

# 并集（所有集合的元素合并去重）
SUNION set1 set2 set3 ...

# 并集并存储到目标集合
SUNIONSTORE dest_set set1 set2 set3 ...

# 差集（第一个集合有而其他集合没有的元素）
SDIFF set1 set2 set3 ...

# 差集并存储到目标集合
SDIFFSTORE dest_set set1 set2 set3 ...
```

---

## 四、Sorted Set（有序集合）

有序集合每个元素关联一个 score（分数），按 score 从小到大排序。score 相同则按字典序排序。元素不重复，score 可以重复。

### 添加与删除

```bash
# 添加一个或多个元素
ZADD zset_key score1 member1 score2 member2 score3 member3

# ZADD 扩展参数（Redis 3.0.2+）:
ZADD zset_key [NX|XX] [GT|LT] [CH] [INCR] score member

# NX — 仅添加新元素（不更新已存在的）
# XX — 仅更新已存在的元素（不添加新的）
# GT — 仅当新 score 大于当前 score 时更新
# LT — 仅当新 score 小于当前 score 时更新
# CH — 返回值改为被修改的元素数量（默认只计新增）
# INCR — 对 member 的 score 做增量，等价于 ZINCRBY

# 删除一个或多个元素
ZREM zset_key member1 member2

# 根据排名删除（Redis 5.0+）
ZREMRANGEBYRANK zset_key 0 2    # 删除排名前 3 的元素

# 根据分数范围删除
ZREMRANGEBYSCORE zset_key 0 100
```

### 查询操作

```bash
# 获取指定排名范围的元素（按 score 升序，索引从 0 开始）
ZRANGE zset_key 0 -1            # 全部元素（从小到大）
ZRANGE zset_key 0 9             # 前 10 个
ZRANGE zset_key -5 -1           # 最后 5 个

# ZRANGE 扩展参数（Redis 6.2+）:
ZRANGE zset_key start stop [BYSCORE|BYLEX] [REV] [LIMIT offset count] [WITHSCORES]

# 按 score 降序获取
ZRANGE zset_key 0 -1 REV              # 等价于 ZREVRANGE

# 按分数范围获取
ZRANGE zset_key 80 100 BYSCORE WITHSCORES

# 限制返回数量
ZRANGE zset_key 0 -1 LIMIT 0 10       # 跳过 0 个，取 10 个

# 按分数范围降序获取
ZREVRANGEBYSCORE zset_key max min [WITHSCORES] [LIMIT offset count]
ZREVRANGEBYSCORE zset_key 100 0 WITHSCORES LIMIT 0 10

# 获取元素排名（升序，最小从 0 开始）
ZRANK zset_key member

# 获取元素排名（降序，最大从 0 开始）
ZREVRANK zset_key member

# 获取元素的分数
ZSCORE zset_key member

# 获取元素数量
ZCARD zset_key

# 统计分数范围内的元素数量
ZCOUNT zset_key 0 100

# 返回第一个 member1 与 member2 之间的元素（按字典序）
ZRANGEBYLEX zset_key [member1 [member2
# [ 表示闭区间，( 表示开区间，- 表示最小，+ 表示最大

# 统计字典序区间内的元素数量
ZLEXCOUNT zset_key [member1 [member2

# 渐进式遍历
ZSCAN zset_key cursor [MATCH pattern] [COUNT count]
```

### 分数操作

```bash
# 增加元素的分数
ZINCRBY zset_key increment member
# increment 可以是负数（减少）
# 返回新的分数

# 获取多个元素的分数（Redis 6.2+）
ZMSCORE zset_key member1 member2 member3
```

### 集合运算（Redis 6.2+）

```bash
# 计算交集
ZINTER numkeys key1 key2 ... [WEIGHTS w1 w2] [AGGREGATE SUM|MIN|MAX] [WITHSCORES]

# 计算并集
ZUNION numkeys key1 key2 ... [WEIGHTS w1 w2] [AGGREGATE SUM|MIN|MAX] [WITHSCORES]

# 运算结果存储到目标集合
ZINTERSTORE dest numkeys key1 key2 ... [WEIGHTS w1 w2] [AGGREGATE SUM|MIN|MAX]
ZUNIONSTORE dest numkeys key1 key2 ... [WEIGHTS w1 w2] [AGGREGATE SUM|MIN|MAX]

# WEIGHTS — 每个集合的分数乘数
# AGGREGATE — 重复元素的分数合并方式：SUM（默认，相加）、MIN（取最小）、MAX（取最大）
```

### 阻塞弹出（Redis 5.0+）

```bash
# 从第一个非空有序集合中弹出最小分数的元素
BZPOPMIN key1 key2 ... timeout

# 从第一个非空有序集合中弹出最大分数的元素
BZPOPMAX key1 key2 ... timeout

# 弹出最小分数的元素（非阻塞）
ZPOPMIN zset_key [count]

# 弹出最大分数的元素（非阻塞）
ZPOPMAX zset_key [count]
```

---

## 五、Hash（哈希）

Hash 是一个 string 类型的 field 和 value 的映射表，特别适合存储对象。

### 基本操作

```bash
# 设置一个或多个字段的值
HSET hash_key field1 value1 field2 value2

# 仅当字段不存在时设置（Redis 4.0+）
HSETNX hash_key field value

# 获取指定字段的值
HGET hash_key field

# 获取多个字段的值
HMGET hash_key field1 field2 field3

# 获取所有字段和值
HGETALL hash_key
# 返回格式: field1, value1, field2, value2, ...

# 删除一个或多个字段
HDEL hash_key field1 field2
```

### 查询操作

```bash
# 判断字段是否存在
HEXISTS hash_key field

# 获取所有字段名
HKEYS hash_key

# 获取所有值
HVALS hash_key

# 获取字段数量
HLEN hash_key

# 获取字段值的字符串长度
HSTRLEN hash_key field

# 渐进式遍历
HSCAN hash_key cursor [MATCH pattern] [COUNT count]
```

### 计数器操作

```bash
# 对字段的数值进行递增
HINCRBY hash_key field increment
# increment 可以为负数

# 对字段的数值进行浮点递增
HINCRBYFLOAT hash_key field 0.5
```

---

## 六、Stream（流）

Stream 是 Redis 5.0 引入的持久化消息队列结构。每条消息有唯一 ID 和一组 field-value 对。

### 添加消息

```bash
# 添加一条消息
XADD stream_key [MAXLEN|MINID [=|~] threshold [LIMIT count]] *|ID field1 value1 field2 value2 ...

# * 表示由 Redis 自动生成 ID（格式: <毫秒时间戳>-<序号>）
# 示例：添加一条消息
XADD mystream * sensor_id 1234 temperature 25.6

# 限制流长度（高效裁剪旧消息，近似修整 ~ 表示不精确）
XADD mystream MAXLEN ~ 1000 * msg "hello"
# MAXLEN ~ 1000 — 保留大约 1000 条，执行效率更高
# MAXLEN = 1000 — 精确保留 1000 条，性能开销稍大

# 基于最小 ID 裁剪
XADD mystream MINID ~ 1699000000000-0 * msg "hello"
```

### 读取消息

```bash
# 按 ID 范围查找消息
XRANGE stream_key start end [COUNT count]

# 查看所有消息
XRANGE mystream - + COUNT 10
# - 是最小 ID，+ 是最大 ID

# 查看某个时间之后的消息
XRANGE mystream 1699000000000-0 + COUNT 10

# 反向查找
XREVRANGE stream_key end start [COUNT count]

# 获取流长度（消息数量）
XLEN stream_key
```

### 消费者组

```bash
# 创建消费者组
XGROUP CREATE stream_key group_name start_id [MKSTREAM]
# start_id 为 0 表示从头开始消费，$ 表示只消费新消息
# MKSTREAM — 如果流不存在则自动创建

# 删除消费者组
XGROUP DESTROY stream_key group_name

# 创建消费者（组内自动注册）
XGROUP CREATECONSUMER stream_key group_name consumer_name

# 删除消费者
XGROUP DELCONSUMER stream_key group_name consumer_name
```

### 消费消息

```bash
# 消费者组模式读取（推荐的消费方式）
XREADGROUP GROUP group_name consumer_name [COUNT count] [BLOCK ms] STREAMS key1 key2 ... id1 id2 ...
# id 使用 > 表示只读取从未被组内其他消费者读取过的新消息
# id 使用 0 或其他具体 ID 可以读取历史未确认消息

# 示例：阻塞等待新消息
XREADGROUP GROUP mygroup consumer1 COUNT 10 BLOCK 5000 STREAMS mystream >

# 确认消息（ACK）
XACK stream_key group_name message_id1 message_id2 ...

# 查看待处理消息（已读取但未 ACK）
XPENDING stream_key group_name [start end count [consumer_name]]

# 转移待处理消息的所有权
XCLAIM stream_key group_name consumer_name min_idle_time id1 id2 ...
# 超过 min_idle_time 毫秒未被处理的消息转移给 consumer_name
```

### 其他操作

```bash
# 删除消息
XDEL stream_key message_id1 message_id2

# 裁剪流到指定长度
XTRIM stream_key MAXLEN [=|~] count

# 查看流信息
XINFO STREAM stream_key
XINFO GROUPS stream_key
XINFO CONSUMERS stream_key group_name
```

---

## 七、Geospatial（地理位置）

GEO 类型在 Redis 3.2 引入，用于存储经纬度坐标并实现地理位置计算。底层使用 Sorted Set。

### 添加坐标

```bash
# 添加一个或多个地理位置
GEOADD key longitude latitude member [longitude latitude member ...]

# 示例
GEOADD cities 116.397 39.908 "北京" 121.473 31.230 "上海" 113.264 23.129 "广州"
# 经度范围: -180 ~ 180，纬度范围: -85.051 ~ 85.051
```

### 查询操作

```bash
# 获取指定位置的经纬度
GEOPOS key member1 member2 ...

# 获取两个位置之间的距离
GEODIST key member1 member2 [m|km|mi|ft]
# m  — 米（默认）
# km — 千米
# mi — 英里
# ft — 英尺

# 获取指定成员的 Geohash 字符串
GEOHASH key member1 member2 ...
```

### 范围查询

```bash
# 根据经纬度查询指定半径内的位置
GEOSEARCH key FROMMEMBER member|FROMLONLAT longitude latitude
           BYRADIUS radius m|km|mi|ft
           [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC]

# 按成员查询：从"北京"出发，半径 1000 公里内的城市
GEOSEARCH cities FROMMEMBER "北京" BYRADIUS 1000 km WITHCOORD WITHDIST

# 按坐标查询：从指定坐标出发
GEOSEARCH cities FROMLONLAT 116.397 39.908 BYRADIUS 500 km

# 按矩形范围查询（Redis 6.2+）
GEOSEARCH key FROMMEMBER member|FROMLONLAT lon lat
           BYBOX width height m|km|mi|ft [ASC|DESC] [COUNT count] [WITHCOORD] [WITHDIST] [WITHHASH]

# 查询结果存储到新集合（返回结果数量）
GEOSEARCHSTORE dest_key source_key FROMMEMBER member BYRADIUS 100 km
```

### 旧版半径查询（Redis 3.2 - 6.1，6.2 后可用但推荐 GEOSEARCH）

```bash
# 按坐标查询半径
GEORADIUS key longitude latitude radius m|km|mi|ft
          [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC] [STORE dest] [STOREDIST dest]

# 按已有成员查询半径
GEORADIUSBYMEMBER key member radius m|km|mi|ft
                  [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC] [STORE dest] [STOREDIST dest]

# WITHDIST  — 返回距离
# WITHCOORD — 返回经纬度
# WITHHASH  — 返回 Geohash 值
# ASC/DESC  — 按距离排序
# STORE     — 结果存储到目标 key
# STOREDIST — 结果存储，且 score 设为距离
```

---

## 八、HyperLogLog（基数统计）

HyperLogLog 是一种概率性数据结构，用于估算集合的基数（不重复元素数量）。标准误差 0.81%，每个 HyperLogLog 仅占约 12KB 内存。

```bash
# 添加一个或多个元素
PFADD key element1 element2 element3 ...
# 如果 HyperLogLog 内部发生变化返回 1，否则返回 0

# 快速获取估算的基数（不重复元素数量）
PFCOUNT key1 key2 ...
# 传入多个 key 时返回它们的并集基数

# 合并多个 HyperLogLog 到目标 key
PFMERGE dest_key source1 source2 source3 ...
# 将多个 HyperLogLog 合并为一个，dest_key 不存在则创建
```

**注意事项**：
- HyperLogLog 不存储元素本身，不能获取元素的具体值
- 适合海量 UV 统计、去重计数等场景
- 内存固定 12KB，无论存储多少元素

---

## 九、Bitmap（位图）

Bitmap 不是一个独立的数据类型，而是基于 String 的位操作，可以按位存取信息。一个 String 最大 512MB，可表示 2^32 个 bit 位。

### 基本位操作

```bash
# 设置指定偏移量的位值
SETBIT key offset value
# offset 从 0 开始
# value 只能是 0 或 1

# 获取指定偏移量的位值
GETBIT key offset
# 返回 0 或 1，如果 offset 超出范围返回 0

# 统计值为 1 的 bit 数量
BITCOUNT key [start end]
# start/end 是字节偏移量，非 bit 偏移量
# 示例: BITCOUNT mykey 0 0  表示统计第一个字节中 1 的数量
```

### 位操作

```bash
# 多个 bitmap 进行 AND / OR / XOR / NOT 运算，结果存入 dest
BITOP AND dest_key key1 key2 key3 ...
BITOP OR dest_key key1 key2 key3 ...
BITOP XOR dest_key key1 key2 key3 ...
BITOP NOT dest_key key1
# NOT 操作只支持一个源 key
```

### 查找位

```bash
# 查找第一个值为 bit（0 或 1）的位置
BITPOS key bit [start end]
# start/end 是字节偏移量
# 示例: BITPOS mykey 1     查找第一个 1 的位置
# 示例: BITPOS mykey 0 2 4 在字节 2 到 4 之间查找第一个 0
```

**典型应用**：
```bash
# 用户签到：user:1:2026:07
SETBIT user:1:2026:07 0 1    # 7月1日签到
SETBIT user:1:2026:07 13 1   # 7月14日签到
BITCOUNT user:1:2026:07      # 本月签到次数
GETBIT user:1:2026:07 0      # 7月1日是否签到

# 活跃用户统计：SETBIT online 10001 1（用户 ID 为 10001 在线）
```

---

## 十、Bitfield（位域）

Bitfield 允许在一个字符串中编码多个不同宽度的整数，可以同时对多个位域进行 GET/SET/INCRBY 操作。Redis 3.2 引入。

```bash
BITFIELD key [GET type offset] [SET type offset value] [INCRBY type offset increment] ...
```

### 类型参数

```bash
# type 格式: [u|i]<bits>
# u — 无符号整数
# i — 有符号整数（最高位为符号位）
# bits — 位数，从 1 到 64

# 示例
u8   # 无符号 8 位整数（0 ~ 255）
i8   # 有符号 8 位整数（-128 ~ 127）
u16  # 无符号 16 位整数（0 ~ 65535）
i32  # 有符号 32 位整数
u1   # 无符号 1 位（即单个 bit）
```

### GET — 读取位域

```bash
# 读取指定偏移量处的有符号 8 位整数
BITFIELD mykey GET i8 0
# GET 返回该位置的值

# 读取多个位域
BITFIELD mykey GET u4 0 GET u4 4 GET u4 8
# 返回数组: [value1, value2, value3]
```

### SET — 设置位域

```bash
# 设置指定偏移量处的无符号 8 位整数
BITFIELD mykey SET u8 0 200

# GET 和 SET 组合
BITFIELD mykey SET u8 0 100 GET u8 0
# 返回之前设置的值
```

### INCRBY — 递增/递减

```bash
# 对指定偏移量处的整数做递增（负数表示递减）
BITFIELD mykey INCRBY u8 0 1    # 递增 1
BITFIELD mykey INCRBY i32 8 -5  # 递减 5

# 溢出控制（overflow）:
BITFIELD mykey OVERFLOW WRAP INCRBY i8 0 1     # WRAP（默认）: 回绕，i8 的 127 + 1 = -128
BITFIELD mykey OVERFLOW SAT INCRBY u8 0 1      # SAT（截断）: u8 的 255 + 1 = 255
BITFIELD mykey OVERFLOW FAIL INCRBY i8 0 1     # FAIL: 溢出时返回 nil 不修改
# OVERFLOW 只影响其后同一命令中的 INCRBY，不影响 GET 和 SET
```

### 复杂示例

```bash
# 存储一个玩家的游戏状态：等级(u8, offset 0)、金币(u16, offset 8)、经验值(u32, offset 24)
BITFIELD player:1 SET u8 0 50        # 等级 50
BITFIELD player:1 SET u16 8 30000    # 金币 30000
BITFIELD player:1 SET u32 24 999999  # 经验值 999999

# 一次读取全部信息
BITFIELD player:1 GET u8 0 GET u16 8 GET u32 24
# 返回: [50, 30000, 999999]

# 一次操作：加金币并扣经验
BITFIELD player:1 INCRBY u16 8 200 INCRBY u32 24 -100
```

---

## 通用命令速查

```bash
# key 操作
DEL key1 key2 ...          # 删除 key
EXISTS key                 # 判断 key 是否存在
TYPE key                   # 查看 key 的类型
RENAME old new             # 重命名 key
RENAMENX old new           # 仅当 new 不存在时重命名
COPY source dest           # 复制 key（Redis 6.2+）
KEYS pattern               # 查找匹配的 key（危险，生产禁用）
SCAN cursor [MATCH p] ...  # 渐进式遍历所有 key（推荐）

# 过期
EXPIRE key seconds
PEXPIRE key milliseconds
EXPIREAT key timestamp
PEXPIREAT key ms_timestamp
TTL key
PTTL key
PERSIST key

# 序列化
DUMP key                   # 序列化 key 的值（RDB 格式）
RESTORE key ttl value      # 反序列化恢复

# 对象信息
OBJECT ENCODING key         # 查看值的内部编码
OBJECT FREQ key             # 查看 LFU 访问频率
OBJECT IDLETIME key         # 查看空闲时间（秒）
OBJECT REFCOUNT key         # 查看引用计数
```
