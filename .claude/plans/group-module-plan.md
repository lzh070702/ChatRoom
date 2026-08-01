# 群模块完善计划

## 当前已完成
- GroupModel: createGroup, queryGroups, groupExist, isInGroup, applyGroup, processGroupRequest
- ChatService: type 9/10/11/12 四个handler
- queryMembers 空函数体待写

## Bug修复
1. [GroupModel::createGroup](server/model/GroupModel.cc:7) — `int group_id` 应为 `int&`，当前值传递导致 `mysql_insert_id` 无法传回
2. [ChatService::signIn](server/service/ChatService.cc:61) — login 时未投递 `offline_group_request` 中的离线群申请

## 待完成

### 1. 查询群成员列表 (queryMembers)
- 已有一个空函数体，只需填充实现
- SQL: `SELECT user_id FROM group_user WHERE group_id = ? AND role > 0;`
- 调用方校验：请求者必须在群里

### 2. 群主添加/删除管理员 (promoteAdmin / demoteAdmin)
- 权限校验：操作者必须是群主(role=3)
- 被操作者必须在群里且不是群主
- promote: role → 2
- demote: role → 1

### 3. 管理员/群主移除成员 (kickMember)
- 权限校验：操作者 role >= 2
- 不能踢群主(role=3)
- 不能踢比自己权限高的人（管理员不能踢群主或其他管理员，但可以踢普通成员——这个要根据你想要的设计）
- 被移除者不能继续在该群发消息

### 4. 用户主动退出群聊 (quitGroup)
- 群主不能直接退出(需先解散)
- 退出后删除 group_user 记录

### 5. 解散群组 (dissolveGroup)
- 仅群主可操作
- 事务：删除 group_user 所有记录 + 删除 group_info

### 6. processGroupRequest 权限校验增强
- 当前缺少：判断操作人是否仍拥有管理员或群主权限
- 当前缺少：判断申请用户是否仍处于待验证状态(role=0)
- 当前缺少：防止重复处理

### 7. 群聊天 (groupChat)
- 消息发送/转发/历史/离线存储

### 8. 其他待补功能
- 注销（logout命令）
- 好友屏蔽
- 离线群申请投递（signIn中）
- 心跳检测
