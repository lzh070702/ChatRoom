-- ChatRoom 数据库初始化脚本
-- 使用方法: sudo mysql < init.sql

-- 创建数据库
CREATE DATABASE IF NOT EXISTS chatroom
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

-- 创建用户并授权
CREATE USER IF NOT EXISTS 'chatserver'@'localhost' IDENTIFIED BY '123456';
GRANT ALL PRIVILEGES ON chatroom.* TO 'chatserver'@'localhost';
FLUSH PRIVILEGES;

USE chatroom;

-- 用户表
CREATE TABLE IF NOT EXISTS `user` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `name` VARCHAR(50) NOT NULL,
    `email` VARCHAR(100) NOT NULL UNIQUE,
    `password` VARCHAR(64) NOT NULL,  -- SHA256
    `state` TINYINT DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 好友表 (status: 1=申请中, 2=好友, 3=拉黑)
CREATE TABLE IF NOT EXISTS `friend` (
    `user_id` INT NOT NULL,
    `friend_id` INT NOT NULL,
    `status` TINYINT DEFAULT 1,
    PRIMARY KEY (`user_id`, `friend_id`),
    FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`friend_id`) REFERENCES `user`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群组表
CREATE TABLE IF NOT EXISTS `group` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `name` VARCHAR(100) NOT NULL,
    `creator` INT NOT NULL,
    FOREIGN KEY (`creator`) REFERENCES `user`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群成员表 (role: 1=普通, 2=管理员, 3=群主)
CREATE TABLE IF NOT EXISTS `group_member` (
    `group_id` INT NOT NULL,
    `user_id` INT NOT NULL,
    `role` TINYINT DEFAULT 1,
    PRIMARY KEY (`group_id`, `user_id`),
    FOREIGN KEY (`group_id`) REFERENCES `group`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群申请表 (status: 0=待处理, 1=同意, 2=拒绝)
CREATE TABLE IF NOT EXISTS `group_apply` (
    `group_id` INT NOT NULL,
    `user_id` INT NOT NULL,
    `status` TINYINT DEFAULT 0,
    PRIMARY KEY (`group_id`, `user_id`),
    FOREIGN KEY (`group_id`) REFERENCES `group`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 消息表
CREATE TABLE IF NOT EXISTS `message` (
    `id` BIGINT PRIMARY KEY AUTO_INCREMENT,
    `from_id` INT NOT NULL,
    `to_id` INT NOT NULL,
    `content` TEXT NOT NULL,
    `is_file` TINYINT DEFAULT 0,
    `is_group` TINYINT DEFAULT 0,
    `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (`from_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    INDEX idx_to_id (`to_id`),
    INDEX idx_create_time (`create_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;