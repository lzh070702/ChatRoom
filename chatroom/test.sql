CREATE DATABASE IF NOT EXISTS `chatroom`
DEFAULT CHARSET utf8mb4
COLLATE utf8mb4_unicode_ci;

USE `chatroom`;

CREATE TABLE IF NOT EXISTS `user` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '用户ID',
    `name` VARCHAR(50) NOT NULL COMMENT '用户名',
    `email` VARCHAR(25) UNIQUE NOT NULL COMMENT '邮箱（唯一）',
    `password` CHAR(64) NOT NULL COMMENT '密码哈希',
    `state` TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0.离线 1.在线',
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

CREATE TABLE IF NOT EXISTS `friend` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '关系ID',
    `user_id` INT UNSIGNED NOT NULL COMMENT '用户ID',
    `friend_id` INT UNSIGNED NOT NULL COMMENT '好友ID',
    `status` TINYINT NOT NULL DEFAULT 0 COMMENT '关系状态: 0.待验证 1.正常 2.拉黑',
    `remark` VARCHAR(50) DEFAULT NULL COMMENT '好友备注',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_user_friend` (`user_id`, `friend_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='好友关系表';

CREATE TABLE IF NOT EXISTS `group_info` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '群聊ID',
    `name` VARCHAR(50) NOT NULL COMMENT '群聊名',
    `owner_id` INT UNSIGNED NOT NULL COMMENT '群主ID',
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='群聊信息表';

CREATE TABLE IF NOT EXISTS `group_user` (
    `group_id` INT UNSIGNED NOT NULL COMMENT '群聊ID',
    `user_id` INT UNSIGNED NOT NULL COMMENT '群聊成员ID',
    `role` TINYINT NOT NULL DEFAULT 0 COMMENT '角色: 0.成员 1.管理员 2.群主',
    PRIMARY KEY (`group_id`, `user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='群聊成员表';

CREATE TABLE IF NOT EXISTS `message` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '消息ID',
    `sender_id` INT UNSIGNED NOT NULL COMMENT '发送者ID',
    `receiver_id` INT UNSIGNED NOT NULL COMMENT '接受者ID',
    `type` TINYINT NOT NULL DEFAULT 0 COMMENT '聊天类型: 0.私聊 1.群聊',
    `content` TEXT NOT NULL COMMENT '发送信息',
    `send_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '发送时间',
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='聊天记录表';