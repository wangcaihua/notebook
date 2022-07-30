
# docker 中下载 mysql
```bash
docker pull mysql
docker run --name mysql -p 3306:3306 -e MYSQL_ROOT_PASSWORD=123456! -d mysql
```

#进入容器
docker exec -it mysql bash

#登录mysql
mysql -u root -p
ALTER USER 'root'@'localhost' IDENTIFIED BY '123456';

#添加远程登录用户
CREATE USER 'liaozesong'@'%' IDENTIFIED WITH mysql_native_password BY 'Lzslov123!';
GRANT ALL PRIVILEGES ON *.* TO 'liaozesong'@'%';


```sql
CREATE DATABASE 数据库名;
DROP DATABASE 数据库名;
USE 数据库名;
```

CREATE TABLE new_table_name AS
    SELECT column1, column2,...
    FROM existing_table_name
    WHERE ....;



```sql
CREATE TABLE Persons (
    ID int AUTO_INCREMENT,
    LastName varchar(255) NOT NULL,
    FirstName varchar(255) NOT NULL,
    Address varchar(255),
    City varchar(255) DEFAULT 'beijin',
    BrithDay date DEFAULT CURRENT_DATE(),
    UNIQUE (LastName)
    CONSTRAINT UC_Person UNIQUE (ID,LastName)
    PRIMARY KEY (ID)
    CHECK (Age>=18)
    CONSTRAINT CHK_Person CHECK (Age>=18 AND City='Sandnes')
);
```

The following constraints are commonly used in SQL:

NOT NULL - Ensures that a column cannot have a NULL value
UNIQUE - Ensures that all values in a column are different
PRIMARY KEY - A combination of a NOT NULL and UNIQUE. Uniquely identifies each row in a table
FOREIGN KEY - Prevents actions that would destroy links between tables
CHECK - Ensures that the values in a column satisfies a specific condition
DEFAULT - Sets a default value for a column if no value is specified
CREATE INDEX - Used to create and retrieve data from the database very quickly


create table table_name (column_name column_type);

ALTER TABLE testalter_tbl RENAME TO alter_tbl;
DROP TABLE table_name ;
ALTER TABLE testalter_tbl  DROP i;
ALTER TABLE testalter_tbl ADD i INT;
ALTER TABLE testalter_tbl DROP i;
ALTER TABLE testalter_tbl ADD i INT FIRST;


ALTER TABLE testalter_tbl DROP 列名;
ALTER TABLE testalter_tbl ADD i INT AFTER c;
ALTER TABLE testalter_tbl MODIFY c CHAR(10);
ALTER TABLE testalter_tbl CHANGE 老列名 新列名 数据类型;
ALTER TABLE testalter_tbl MODIFY 列名 数据类型 [限制] [默认值];
ALTER TABLE testalter_tbl ALTER i SET DEFAULT 1000;
SHOW COLUMNS FROM testalter_tbl;

SHOW TABLE STATUS LIKE 'testalter_tbl'\G



CREATE TABLE IF NOT EXISTS `runoob_tbl`(
   `runoob_id` INT UNSIGNED AUTO_INCREMENT,
   `runoob_title` VARCHAR(100) NOT NULL,
   `runoob_author` VARCHAR(40) NOT NULL,
   `submission_date` DATE,
   PRIMARY KEY ( `runoob_id` )
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE table_name ;

insert into table_name(field1, field2, ... fieldN) VALUES
                       ( value1, value2,...valueN );

INSERT INTO runoob_tbl(runoob_title, runoob_author, submission_date) VALUES
 ("学习 PHP", "菜鸟教程", NOW());


UPDATE table_name SET field1=new-value1, field2=new-value2
[WHERE Clause]



CREATE INDEX indexName ON table_name (column_name)
    CREATE TABLE mytable(  
    ID INT NOT NULL,   
    username VARCHAR(16) NOT NULL,  
    INDEX [indexName] (username(length))  
);
DROP INDEX [indexName] ON mytable; 
CREATE UNIQUE INDEX indexName ON mytable(username(length)) 
CREATE TABLE mytable(  
 
ID INT NOT NULL,   
 
username VARCHAR(16) NOT NULL,  
 
UNIQUE [indexName] (username(length))  
 
);  
ALTER TABLE tbl_name ADD PRIMARY KEY (column_list): 该语句添加一个主键，这意味着索引值必须是唯一的，且不能为NULL。
ALTER TABLE tbl_name ADD UNIQUE index_name (column_list): 这条语句创建索引的值必须是唯一的（除了NULL外，NULL可能会出现多次）。
ALTER TABLE tbl_name ADD INDEX index_name (column_list): 添加普通索引，索引值可出现多次。
ALTER TABLE tbl_name ADD FULLTEXT index_name (column_list):该语句指定了索引为 FULLTEXT ，用于全文索引。
ALTER TABLE testalter_tbl MODIFY i INT NOT NULL;
ALTER TABLE testalter_tbl ADD PRIMARY KEY (i);
