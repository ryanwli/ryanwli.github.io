---
layout:     post
title:      "Mysql之锁与事务"
subtitle:   "该文章总结Mysql并发处理中使用到的事务和锁的概念，以及实践Demo支撑概念"
date:       2016-07-20 12:00:00
author:     "ryan"
header-img: "img/post-bg-06.jpg"
---



## 1.锁是什么

我们刚学习编程的时候，锁这个概念让你特别的抽象，锁在计算里面是用来控制共享资源并发访问的一种手段，有些资源在并发访问的时候你想串行执行就必须加锁控制，因为计算机的CPU是性能优先无序执行，所以只有通过加锁来让CPU按照你希望的顺序来执行。在Mysql中，我们希望用锁来保证我们数据的完整性和一致性，说具体点，就是我们电商系统中的库存，在高并发同一个产品库存时，为了避免被扣超出，我们需要数据库的锁机制来保证库存数据的完整性和一致行。



## 2.Mysql Innodb表空间

在了解Mysql Innodb锁之前，我们先来了解Mysql Innodb的表的存储方式。Mysql Innodb中的表都是索引表，索引表的意思就是他的数据存储是按照主键索引的顺序存书的，找到主键ID就找到了该行数据，就算没有显示设定主键，Innodb也会给你找个唯一列来作为主键索引，就算找不到也会给你自动生成一列，说完这个，我们来看看表数据是怎么存储的，看下图：

![20160721_7_tablespace](https://ryanwli.github.io/img/2016/20160721_7_tablespace.png)

脑子里面要有这个结构，后学，我们说锁的类型的时候会联想到该图；



## 3.锁类型

一般我们说的锁类型一般就两种，读锁(S)和写锁(X)，但是如果算上加锁过程中，我们要对锁粒度的支持呢，还有读意向锁(IS-Intention S Lock)和写意向锁(IX-Intention X Lock)。他们的兼容性如下表：

|      |  IS  |  IX  |  S   |  X   |
| :--: | :--: | :--: | :--: | :--: |
|  IS  |  兼   |  兼   |  兼   |  不兼  |
|  IX  |  兼   |  兼   |  不兼  |  不兼  |
|  S   |  兼   |  不兼  |  兼   |  不兼  |
|  X   |  不兼  |  不兼  |  不兼  |  不兼  |

### S锁：

实际例子，一个事务里面使用了select lock in share mode(S锁)，那么其他事务里面用lock in share mode可以获取数据，但是如果使用select from for update, delete, update, insert这些写锁操作相同资源都会被block，不兼容；

### X锁：

实际例子，一个事务里面使用了select from for update(S锁), 那么其他事务里面使用select from for update, select lock in share mode, delete, update, insert这些写锁操作相同资源都会被block，所有都不兼容；

### IS和IX锁：

这个其实算事一个过程锁，之前我们描绘了表的结构组成，是一层一层包到行级别的，如果我们对该表中一行上了一个行级X锁，那么innodb会从内到外，在页/区/段/表都加上IX锁。在innodb中，我们只有能操作到表锁和行级锁，不能操作页/区/段锁，这个还是举一个实际例子，一个事务使用了select for update锁住了A表的一行数据，这时给A表也套上了IX锁，如果另外一个事务正在执行alter的改表操作，该操作会导致给表加一个S锁，S锁和之前select for update套的IX锁是不兼容的，所以这个时候改表就会等待；

> TRX1:
>
> begin
>
> select * from test_id2 where id=1 for update
>
> commit
>
> 
>
> TRX2:
>
> ALTER TABLE test_id2 MODIFY refId bigint;





## 4.Snapshot read & Current read

### Snapshot read

Snapshot read直译过来就是快照读，mysql innodb实现了大多数数据库都有的“多版本并发控制”(英文简写为MVCC-Multiple Version Concurrency Control)，我们大部分的查询都是使用的快照读，他读到的不是该行真正的数据，而是内存中一个副本，所以我们在执行事务中各种X锁定之后还能读取数据(读写都不冲突)，就归功于该机制，他使得数据库的并发性能得到巨大的提升，所以基本市面上的关系型数据库都支持该机制，下面给一些列子：

> 不在Serializable(后面会说)的事务隔离级别的其他任何地方使用下面语句都是使用snapshot read
>
> select * from test_id2 where id=10

### Current read

Current read直译过来就是当前读，我们在事务中执行S和X锁定，都是对当前行的操作，也肯定要读相关行的原始数据，这样才能完成并发想要的控制，这个当前读也有一个术语“锁并发控制”(英文简写为LBCC-Lock Based Concurrency Control)，这个机制会导致，只要有行级X锁，所有读都会被block，注意是所有的！所以LBCC并发很弱；

> select * from test_id2 where id=10 lock in share mode
>
> select * from test_id2 where id=10 lock for update
>
> delete from test_id2 where id=10;
>
> update test_id2 set  refId=12 where id=10;
>
> insert into test_id2 (refId) value (12);

为什么上面例子中delete/update/insert都使用了当前读呢，因为delete/update都需要先根据后面的where条件做一次select操作并执行X Lock，所以肯定有当前读的操作了。由于主键或者unique index，或者外键约束所以insert也会触发当前读；



## 5.事务隔离级别

### Read Uncommited&脏读

读未提交，就是一个事务可以读取到另外一个事务没有commit的数据修改，有可能另外这个事务修改的数据会rollback，所以第一个事务读取的值压根就是不存在的，所以产生了脏读；一般很少使用该隔离级别。

### Read Commited&不可重复读

读已提交，oracle/ms sql的默认隔离级别，一个事务只能读取另外一个事务已经提交的修改，如果另外一个事务还在执行中，他读取应该前一个snapshot，就是另外一个事务修改数据之前的快照；但是在提交之后，再读取，snapshot也更新了，读取到了修改后的值，所以到这里第一个事务两次读取是不一致的，所以就产生了不可重复读，同样在Read Commited也会产生该不可重复读；

### Repeatable Read&幻读

可重复读取，mysql的默认隔离级别，mysql/oracle/ms sql如果在不是用读行锁的形式，两次读取中的第二次都是使用的在该事务环境下第一次读取的snapshot的快照，这样就做到可重复读取了。但是如果在使用select for update/select lock in share mode这种当前读，而不是快照读了，oracle/ms sql在其他事务有insert的场景下就不能保证可重复读了，而mysql使用了一个间隙锁算法(gap lock，后续会讲到)也保证了在这种模式下也能保证可重复读，其实幻读就是不可重复读的一种特例子，我的理解就是在这种当前读的模式下还有不重复读取的现象就叫幻读；

### Serializable

串行化，这个隔离级别是最高的，并发也是最弱的，其他事务隔离级别中的一般快照读(select * from test_id2)都会被编程带S锁当前读，不会有脏读，也不会有不重复读，也不会有幻读，整个都串行化。



## 6.锁算法

待续