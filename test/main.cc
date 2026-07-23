#include <iostream>
#include "./chatroom/server/MySQL/MySQL.h"

using namespace std;

int main() {
    MySQL conn;

    /* 1. 连接数据库（改成你自己的环境） */
    bool ret = conn.connect("root",       // user
                            "",           // password
                            "",           // database
                            "127.0.0.1",  // ip
                            3306          // port
    );

    if (!ret) {
        cerr << "MySQL connect failed!" << endl;
        return -1;
    }
    cout << "MySQL connect success!" << endl;

    /* 2. 创建测试表 */
    const string createSql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INT PRIMARY KEY AUTO_INCREMENT, "
        "name VARCHAR(50), "
        "age INT) CHARACTER SET utf8;";

    if (!conn.update(createSql)) {
        cerr << "create table failed!" << endl;
    }

    /* 3. 插入数据 */
    const string insertSql =
        "INSERT INTO users(name, age) VALUES('Alice', 20), ('Bob', 22);";

    if (conn.update(insertSql)) {
        cout << "insert success!" << endl;
    }

    /* 4. 查询数据 */
    const string querySql = "SELECT id, name, age FROM users;";

    if (conn.query(querySql)) {
        cout << "id\tname\tage" << endl;
        while (conn.next()) {
            string id = conn.value(0);
            string name = conn.value(1);
            string age = conn.value(2);
            cout << id << "\t" << name << "\t" << age << endl;
        }
    } else {
        cerr << "query failed!" << endl;
    }

    /* 5. 事务测试 */
    conn.transaction();
    conn.update("UPDATE users SET age = 30 WHERE name = 'Alice';");
    conn.commit();

    cout << "transaction committed." << endl;

    /* 6. 验证事务结果 */
    if (conn.query("SELECT name, age FROM users WHERE name='Alice';") &&
        conn.next()) {
        cout << "Alice's new age: " << conn.value(1) << endl;
    }

    /* 7. 连接存活时间 */
    conn.refreshAliveTime();
    cout << "alive time: " << conn.getAliveTime() << " ms" << endl;

    return 0;
}