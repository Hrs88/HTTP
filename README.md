#  自主实现HTTP服务器
##  1.设计模式与技术
1. 面向对象
2. 日志
3. ET模式EPOLL
4. 线程池/CP模型
5. 守护进程化
6. 定时清理超时连接
7. 基于HTTP/1.0协议，采用短连接
8. CGI技术
9. 连接本地MYSQL 服务器
## 2.待实现技术延展
1. HTTP/1.1 长链接技术
	1. 连接管理
	2. 粘包问题
2. 能否接入redis
3. 代理功能
## 3.待实现应用延展
1. MYSQL插入、更新、查找
2. 支持更多请求方法
	1. HEAD
	2. PUT
	3. DELETE
	4. ...
3. 将宏处理配置文件化
4. 3XX重定向功能
5. 支持更多错误码响应   
##  4.在线网站测试：[冬青の个人主页](http://www.fosing.xyz)