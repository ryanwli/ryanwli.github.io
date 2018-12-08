---
layout:     post
title:      "Mysql之索引"
subtitle:   "对于查询性能，索引也是能最快最方便提升一个查询速度的捷径，这个99%的开发人员都知道这货是干这个事情的，但是仅仅不到10%的开发人员能真正了解和用好它，该篇文章总结一下mysql中的索引使用"
date:       2016-07-01 12:00:00
author:     "ryan"
header-img: "img/post-bg-06.jpg"
---


# 1. 为什么会有这篇文章？
选择DB作为Training的第一个系列，而不是其他，是因为这货太重要，而且也是大多数开发人员忽略的最多的地方。为什么一开始就选择索引，是因为查询是我们写的最多的SQL语句，索引也是能最快最方便提升一个查询速度的捷径，这个99%的开发人员都知道这货是干这个事情的，但是仅仅不到10%的开发人员能真正了解和用好它。所以有了这个系列已经这个文章的想法。

# 2. 索引类型
对索引最简单的理解就是一本书的页码，可以通过书的目录（这个就相当于是数据库中的索引）对应的页码可以找到这页的所有内容。在这节中我们从MySql中索引类型来说一下数据库中的索引工作原理，从索引的数据结构算法上拆分，在MySql的Innodb存储引擎里面有两个类型，BTree和Hash类型索引，从索引顺序和整行数据物理顺序一致或不一致来区分的话，分为聚族(聚集)索引和非聚族(非聚集)索引。下面我们首先来说一下B+Tree和Hash类型。

### 2.1 BTree索引
BTree的全称是Balance Tree，中文名叫平衡多路搜索树，分为B-Tree和B+Tree，他们两最主要的区别是，B-Tree每个节点除了索引值还带有数据，而B+Tree只有叶子节点带有数据；B+Tree索引在我们MySql的Innodb中使用的是最多的索引类型。下面我们就用一张图来说明一下B+Tree在MySql的工作原理；

![图1，非聚族索引](https://ryanwli.github.io/img/2016/20160801-Non-Cluster-Index.jpg)

红色代表本次查找的结果，这里以查询Zorro名为例，首先从硬盘拿第一层这个块，加在到内存中然后依次对名字进行比较，最后比较后没有找到，但是发现Peter后的Block Pointer还可以进行下一级查找，再将下一级的Block从硬盘加在到内存，重复刚才比较，最后就一直重复这个直到找到或者查找完毕都没有找到；

这种平衡多路搜索树的数据结构可以以极其少的内存空间，以及随机IO次数就可以找到我们想要的数据；

### 2.2 Hash索引

Hash索引我们用的不是特别多，但是在某些场景下我们选择Hash索引可能更适用一点儿，我们还是先用一张图来看一下Hash索引在Mysql里面的工作原理，他工作原理和Java中的HashMap有些类似：
![图2，Hash索引](http://upload-images.jianshu.io/upload_images/16597-fae4b1ab4d6610d3.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

### 2.3 B+Tree和Hash索引各自特点
1.从顺序性上来说，B+Tree是有序的，所以在Order By的时候B+Tree索引会被使用上，而Hash不会被使用；Hash索引的无序性也让它在执行范围查找的时候也不能被使用，而B+Tree可以；但是这种顺序性在插入新数据的时候Hash索引是优于B+Tree索引的，因为插入的时候B+Tree需要遍历树，最坏的时候遍历整个树来创建对应的索引来保证顺序，而Hash本身的无序性，不需要这样做；

2.从查询条件符使用限制来说，B+Tree所以可以用在=，<>, >, <, IN, Between, Like(这里说的Like指Like 'ryan%'，是后匹配，前匹配不支持的)，而Hash索引还是因为它的无序性，它不支持范围查询条件符，它仅仅支持等值查询条件=, IN不支持范围条件>, <, <=, >=, Between, Like之类的；

3.索引效率上来说，假如都用到了索引Hash索引比B+Tree索引更加快的，为什么？这主要从两点来说，第一是Hash索引的存储很紧凑，它可以把较长的字符串转换成一串较短数字，另外它只存储了hash值和对Row的指针，这也是为什么它无序了，而B+Tree存储了整个索引列的值，所以从这块讲他的效率也是相当快的；第二是从结构算法来讲，不知道你们看到右边的复杂度没有，一个是O(log2n)，一个O(1+(bucket capacity-1))，B+Tree随着表的变大他的树的深度也会随着增加，最坏是遍历整个树的深度；而Hash索引如果没有Hash值的碰撞他的复杂度只有O(1)，大部分是O(1)，如果有碰撞，也只是(bucket capacity-1)的增加，所以从算法的复杂度来说，Hash索引执行效率也是优于B+Tree的；

4.在多列组合上面来说，B+Tree索引优于Hash索引，如果(A, B, C)3列作为一个组合索引，如果使用A作为搜索条件，B+Tree索引可以利用上，而Hash索引不会用上，这个是因为Hash索引是将ABC这3列给Hash value了，所以A的部分索引的话是不能使用的；

### 2.4 聚族索引
聚族索引他的索引顺序和数据的存储顺序是一致的，可以理解成他们是在一起的，就像一本书的页码和该页码中的内容；通常我们一个表的主键就是聚族索引，如果一个表没有定义主键，那么MySql的Innodb会自动生成一个主键列，以及主键索引；另外在innodb中聚族索引是按顺序进行物理存储层面存数据的，所以建立聚族索引最好选择自增长的数字作为聚族索引，这样正好在每次最后一位索引值增加新的索引值，如果换成GUID列作为聚族索引，因为GUID是随即生成，并且无序的所以每次插入一行纪录的时候，创建聚族索引为了保证聚族索引的顺序，会去查找指定的顺序位置，产生额外的开销，另外GUID占用36位unicode字符串，不管是比较字符串，还是存储需要的长度都是开销挺大的，所以聚族索引优先选择整数列作为聚族索引；

![图3，聚族索引](https://ryanwli.github.io/img/2016/20160801-Cluster-Index.jpg)



### 2.5 非聚族索引

在一个表只有一个聚族索引，除开聚族索引，该表剩下的索引都是非聚族索引；非聚族索引是基于聚族索引的，这话怎么讲呢？看下图，我们在name上建立了一个B+Tree的非聚族索引，最终匹配到对应Name以后，该非聚族索引还存储了主键，我们使用主键再在聚族索引B+Tree找到真正对应的该Row的数据，所以非聚集索引还有另外一个称号叫“二级索引”；

![图4，非聚族索引](https://ryanwli.github.io/img/2016/20160801-Non-Cluster-Index.jpg)



# 3. MySql的执行计划

由于下一节我们会用到执行计划来判断我们查询语句是否用了合理的索引，所以这节我们先来了解执行计划，首先我们来看一个很简单的执行计划：

![图5，执行计划](http://upload-images.jianshu.io/upload_images/16597-cc467aaae3d27f7d.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
我们看计划重点要关注type和rows(更多查看这里[explain-output.html#explain-join-types](http://dev.mysql.com/doc/refman/5.7/en/explain-output.html#explain-join-types))，rows自然就是该语句查询的行数是多少，重点说说type，type类型一共有这几种，按执行效率排序是这样system > const > eq_ref > ref > fulltext > ref_or_null > index_merge > unique_subquery > index_subquery > range > index > all;
我这里重点说这几个，也是我们平时用的比较多的：const > eq_ref > ref > index_merge > range > index > all;


1.const一般是聚族索引或者Unique索引，直接Where条件或者多表联合查询是只只返回一条数据的情况下使用，如下：
>select * from ORDER_ITEM where ID=12422;

2.eq_ref是用于聚族索引或者Unique索引在作为多表联合查询的关联列的时候，如下：
>select * from ORDER A LEFT JOIN ORDER_ITEM B ON A.ID=B.ORDER_ID WHERE B.ID (123, 4242);
>这里会用到两个索引，其中第二个索引A表就是用eq_ref索引；

3.ref是用在用在重复的非聚族索引上面，作为Where条件，或者多表联合查询的条件时会使用到，如下：
>select * from ORDER_ITEM where ORDER_ID=1;
>select * from ORDER A LEFT JOIN ORDER_ITEM B ON B.ORDER_ID=A.ID where A.ID IN (1,2);
>这里会用到两个索引，其中第二个索引B表就是用ref索引；

4.index_merge这个类型一般是优于建立了两个单列索引，innodb为了执行效率，帮助你优化讲两个单列索引合并成1个多列索引，如下:
>select * from ORDER_ITEM WHERE ORDER_ID=1 OR ITEM_ID=1301;

5.range这个类型估计我们看的最多最多了，使用了索引列的作为Where后的范围查询条件都会是这种类型，Between, IN, >, <, >=, <=，如下：
>select * from ORDER_ITEM WHERE ID < 22234;

6.index这个就是扫描整个索引值；
>select * ORDER_ID from ORDER_ITEM ORDER BY ORDER_ID;
>这个就是ORDER BY和返回列使用了整个索引值，由于没有加任何条件，就走了这种类型；

7.all这个是最慢最慢的，在没有建立索引的列作为条件语句使用，以及ON条件中使用，以及没有任何条件返回列中没有索引(那怕是一列没有索引)，都会走这个all类型；
>select \* from ORDER_ITEM;
>select \* from ORDER_ITEM where ITEM_NAME='apple';

# 4. 索引使用技巧
### 4.1 索引的选择性
我们使用SHOW INDEX执行语句来显示INDEX的一个情况，如下：
![图5，Index属性状态](http://upload-images.jianshu.io/upload_images/16597-41684df639f3d514.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
我们主要关注Cardinality这个属性，对于主键列聚族索引来说它的值等于表的Row Size的，那Cardinality这个属性是什么意思呢？看看官方的解释：
>An estimate of the number of unique values in the index. This is updated by running [ANALYZE TABLE
>](http://dev.mysql.com/doc/refman/5.7/en/analyze-table.html) or [**myisamchk -a**](http://dev.mysql.com/doc/refman/5.7/en/myisamchk.html). Cardinality
> is counted based on statistics stored as integers, so the value is not necessarily exact even for small tables. The higher the cardinality, the greater the chance that MySQL uses the index when doing joins.

从第一句已经说明它的意思，它是一个索引唯一的个数值，所以我说Primary Key的这个值就是Row Szie，那么对于Primary Key的选择性就是100%，那对于非聚族索引而且是非唯一的索引，那么它的选择性就是小于100%，选择性越高，那么如果该索引作为查询条件那么它的查询速度就越快。


### 4.2 多列组合索引
1.多列组合索引一般适用于查询条件有多列，而且同时出现在很多查询场景中；或者说有一部分列在条件中，另外一部分用于了Order By；还有一种场景就是返回字段很少列，也可以把这些少部分的列作为组合索引中的部分列，这样索引就可以直接返回值，而无序回表中获取；
2.多列组合索引是从左至右来应用索引的，是有顺序的，如果直接使用列中或者列尾作为索引条件，是不会走组合索引列的；所以在这里我们应该把使用度和选择性都比较高的列作为组合列的第一个列；
>首先我们建立该索引(name, email, phone)，id为主键
>下面SQL语句都会很好的使用索引：
>select id,name from user where name='ryan' order by id;
>select id,name from user where name='ryan' and email='ryan@..';
>则下面的SQL语句不会使用该索引，而会走scan table的操作：
>select id,name from user where email ='ryan@..';
>select id,name from user where phone='152322....';

### 4.3 覆盖索引
覆盖索引其实就是指查询条件中用到了索引，并且查询的结果直接用了索引中的列值，而没有回表去查找数据；覆盖索引一般是用B+Tree实现，而且一般都是一个组合索引，看下面的示例：
>如果我们建立一个name列的索引，然后返回的是PK ID，如下：
>select id from user where name='ryan';
>那么这个查询的执行效率是相当高的，根据图4的展示，我们可以知道，它不需要回表查询ID，而是直接从name列上的索引返回了，而且大部分MySql索引都存储在数据库服务器内存里面的；

再看一个示例：

>如果我们建立一个(name, phone, email)的索引，该索引在如下查询语句里面也可以看作覆盖索引：
>select id,name,phone,email from where name='ryan' order by name;

### 4.4 三星索引
三星索引是值，一个好索引应该满足，一是查询条件中使用了索引；二是返回的列中都是索引中的值，没有回表里面查找；三是我们很好的利用组合索引的顺序，来放在Order By中也使排序使用了索引；但是我们很难造出三星索引，因为我们索引不单单是用在一条SQL语句，甚至我们有时候连覆盖索引很难造出来。上面覆盖索引这一节，最后那个示例就一个三星索引的使用，另外我们在这个查询语句使用的完美，但是该索引对其它查询语句不一定就工作的很好，所以有时候我们要全局考虑，用尽可能少的索引去满足整个系统查询需要。而不是纠结一个查询，除非这个查询使用频繁度非常高，当然这个查询语句就另开小灶为他单独定制了；


### 4.5 延迟关联
我们在使用MySql分页的时候都会写这样的语句：
>select * from ORDER  limit 10,10

这种使用limit的语句在翻前面几页的时候十分的块，但是如果执行到下面这种语句的时候就会非常的慢：
>select * from ORDER limit 10000,10

这是因为scan了表10000条出来，结果抛弃了9990条没用，只显示了10行，这就会让人感觉只有10行为什么都这么慢，就不知道它scan了9990条所耗费的时间。为了解决这种问题，我们需要使用延迟关联来解决，用聚族索引来加速查询，再看下面的写法：
>select * from ORDER AS T1 INNER JOIN  (select id from ORDER limit 10000,10) AS T2 ON T1.ID=T2.ID

这样里面那个查询，和外面查询都走了索引，而且里面把返回列尽量的减少，这样查询速度是相当快的，里面那个select走的是index，外面走的是eq_ref;

### 4.6 索引与锁
在数据库执行事务处理的时候，如果你用到了
>select ... for update或者select ... lock in share mode

一个是X(排他所)，一个是S(读共享锁)，这两个锁都会lock住表或者查询结果几行数据，这个取决于你where后面查询条件建立了索引没有，如果走的是聚族索引，那么锁定的肯定是一行数据，如果走的是非聚族索引可能是查询的几行也可能是一个范围；如果没有走任何索引那将是很糟糕的结果锁定整个表；还有就算走了索引，不适查询了显示结果几行就锁定了几行数据，而且要看执行计划中的rows，用该索引走了几行数据，这个才是锁定的真正行数，如下面SQL：
> select * from ORDER where ID < 5 and ID <> 1
> 显示的结果肯定没有ID等于1的那行数据，但是ID等于1的那行数据也被锁定了，ID走的聚族索引，但是实际走的range 4行的查询计划；
