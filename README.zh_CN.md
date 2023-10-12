[English](./README.md)

# 前言
[北极星（PolarisMesh）](https://polarismesh.cn) 是腾讯开源协同共建的服务治理平台，tRPC-Cpp北极星插件将[北极星官方sdk](https://github.com/polarismesh/polaris-cpp) 进行了封装。
tRPC-Cpp北极星插件分为**北极星路由选择插件**和**北极星服务实例注册插件**两种，其中北极星路由选择插件提供服务发现功能，北极星服务实例注册插件提供服务实例注册，心跳上报功能。

# 北极星路由选择插件（selector）
提供服务发现功能：先通过**调度策略**筛选出符合要求的实例节点子集，再通过**负载均衡策略**筛选出符合要求的单个实例节点。
已经支持的调度策略：就近路由，规则路由。
已经支持的负载均衡策略：权重随机，一致性哈希。

## 插件注册位置
```yaml
#插件配置
plugins:
  selector:  #路由选择配置
    polarismesh:  #北极星路由选择插件
```

## 调度策略
北极星路由选择插件配置中，consumer配置项不配置时，框架默认开启下列所有调度策略。
**注意：服务路由链是有顺序的（在上的先执行），所以默认配置筛选顺序为ruleRouter->nearbyRouter.**
```yaml
#插件配置
plugins:
  selector:  #路由选择配置
    polarismesh:  #北极星路由选择插件
      consumer:
        serviceRouter:
          chain: #服务路由链，默认打开下列全部策略
            - ruleRouter  #规则路由插件，用于规则过滤
            - nearbyRouter  #就近路由插件，用于就近访问
```

### 就近路由
配置项：`chain: - nearbyRouter`
根据服务实例所在机器的机房地域信息进行就近访问，优先级：城市 > 地域（华东，华南等）> 全国。
**适用范围：**
只要在北极星管理界面上将对应服务开启了“就近访问”，即可使用。
**使用步骤**：
**step1:** 在北极星管理平台上对服务开启“**就近访问**”
**step2: **框架配置中启用nearbyRouter项，如上述默认配置所示。
**注意：**服务开启就近路由后，主调通过服务发现接口默认路由到同城市的被调，当同城被调全部不可用（**不存在/不健康/被熔断**）会降级到同地域的被调。同地域的被调都不可用时则降级到全国其他范围的被调。

### 规则路由
配置项：`chain: - ruleRouter`
根据用户在北极星上配置的路由规则进行流量调度，使用时通过传递对应的规则路由标签，触发对应的匹配规则，实现流量定向分发。
**使用步骤**：
step1：在北极星上为服务配置路由规则
step2: 框架配置中启用`ruleRouter`项，如上述默认配置所示
step3：主调方在发起调用时，设置使用的规则路由标签：
```
// 设置用于规则路由筛选的label
std::map<std::string, std::string> ruleroute_label;
// 这边key/value为用于规则路由筛选的label
ruleroute_label["key"] = "value";
// 设置过规则路由滤标签到client_context(ClientContext对象)
trpc::naming::polarismesh::SetFilterMetadataOfNaming(ruleroute_label, trpc::NamingMetadataType::kRuleRouteLable);
```

## 负载均衡策略
不指定负载均衡时，框架**默认按权重随机（weightedRandom）**返回服务实例节点。如果要选用其他方式的负载均衡，需要在框架配置中**显示指定**。

### 权重随机
框架默认使用北极星sdk内置的的weight-round-random(wrr)策略。

### 一致性哈希
通过指定特定的hash key调用特定的某个服务实例节点。
**使用流程：**
**step1:** 用户需要将hash key设置到context中，设置成功后框架默认使用**一致性哈希**返回对应服务实例节点，如下：
```
trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>();
trpc::naming::polarismesh::SetSelectorExtendInfo(ctx, std::make_pair("hash_key", "1"));
// 发起rpc调用，prx为服务代理
trpc::Status status = prx->RpcMethod(ctx, ...);
```
**step2: **显示指定哈希策略
**注意：**对于新增业务，一致性哈希算法选择**ringhash**即可。至于modulohash, brpcmurmurhash等算法，一般是存量服务迁移时，希望不更改已经使用的一致性哈希算法才设置的（不同算法中同一个key对应的返回实例节点是不同的）。
ringhash：对应北极星sdk的kLoadBalanceTypeRingHash
modulohash：对应北极星sdk的kLoadBalanceTypeSimpleHash  // hash_key%总实例数 选择服务实例
brpcmurmurhash：对应北极星sdk的kLoadBalanceTypeCMurmurHash // 兼容brpc c_murmur的一致性哈希
```yaml
client:
  service:
    - name: trpc.peggiezhutest.helloworld.Greeter
      load_balance_name: ringhash #区分大小写，支持ringHash,maglev,cMurmurHash(兼容brpc c_murmur),localityAware(兼容brpc locality_aware),simpleHash(取模hash) 默认使用的hash算法为ringHash(所以当选择的hash算法为ringHash时可以不做配置)
```
**step3: **可以通过日志确认填写的hash key是否路由到同一个服务实例节点（日志级别调整为DEBUG或TRACE）。
同步调用方式搜索："Select result of " + 被调服务名
异步调用方式搜索："AsyncSelect result of " + 被调服务名

#### hash环算法，返回对应key的相邻节点
如果期望得到hash环算法（如ringhash）获取相邻节点，请显示调用ClientContext的SetReplicateIndex方法进行设置；不设置时ReplicateIndex默认为0，即返回hash key对应的当前节点。
```cpp
trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>();
// 设置使用的hashkey（不要求为整形数），不为空即可
// 获取该hash key对应节点的第一个相邻节点（顺时针），传入2表示获取第二个相邻节点，以此类推
trpc::naming::polarismesh::SetSelectorExtendInfo(ctx, std::make_pair("hash_key", "abc"), std::make_pair("replicate_index", "1"));
```

#### Locality-aware负载均衡算法
对应框架配置如下：
```yaml
client:
  service:
    - name: trpc.peggiezhutest.helloworld.Greeter
      load_balance_name: localityAware
```

#### 动态权重的配置（不包含服务端的上报功能）
**step1：**框架配置文件，开启sdk的动态权重线程
```yaml
plugins:
  selector:
    polarismesh:
      dynamic_weight:
        isOpenDynamicWeight: true # 设置为true
      consumer:
        loadbalancer:
          enableDynamicWeight: true # 设置为true
          type: weightedRandom # 负载均衡类型(虽然具体使用的负载均衡算法是dynamicWeight，但这里要填基本的权重随机算法weightedRandom)
```
**step2：**客户端调用
```yaml
client:
  service:
    - name: trpc.peggiezhutest.helloworld.Greeter
      load_balance_name: dynamicWeight
```

## 服务熔断
北极星cpp sdk根据用户上报的节点调用情况，统计被调节点的某段时间的失败率和连续失败次数，如果满足熔断条件(连续失败多少次或失败率超标)就将节点加入熔断的实例列表，下次调度的时候就不算上熔断的节点。
同时会对熔断的节点进行探活，当检测节点恢复时（如果不开启探活功能就间隔一段时间后），会设置为半开的状态（实例由不可用变成可用），然后优先返回给调用方，然后又依赖主调方上报，如果满足恢复条件就将熔断实例移出熔断列表。属于一种故障容错功能。
```yaml
# 插件配置
plugins:
  selector:  # 路由选择配置
    polarismesh:  # 北极星路由选择插件
     consumer:
       circuitBreaker:  # 熔断相关配置
	 enable: true   # 是否开启熔断
	 ...
```
熔断的触发及恢复条件如下：
-  **熔断策略具体触发条件**
   连续调用失败10次则进行熔断 或 1分钟内失败率>50%（前提是要有10次调用）则熔断。
-  **恢复条件**
   30s内10次调用有8次以上成功后就对熔断节点进行恢复

## 如何通过插件发起rpc调用
分为配置文件方式和代码指定方式，注意配置的namespace一定要正确，selector_name选择polaris。

# 北极星服务实例注册插件（registry）
提供服务实例注册，心跳上报功能。

## 插件注册位置
```yaml
# 插件配置
plugins:
  registry:  # 服务注册配置
    polarismesh:  # 北极星服务实例注册插件
```

## 心跳上报功能
**框架自动进行的心跳上报**，心跳上报相关配置如下：
**注意：框架配置里的ip一定要配置合理，框架才能进行心跳上报。**
- 如果service配置了监听ip，则优先使用监听ip。
- 如果service没有配置监听ip，则会检查service是否配置了nic（监听网卡名），框架会从对应网卡获取ip。

在global配置中，需要有命名空间。
```yaml
# 全局配置
global:
  namespace: ${namespace}    # 命名空间，如正式Production和非正式Development
```
在server配置中，需要!!#ff0000 **添加配置registry_name为polaris**!!。
```yaml
#服务端配置
server:
  registry_name: polaris
  service:
    - name: ${service_name}    # 服务在北极星上的服务名，注意区分这里和四段式命名的区别
      ip: ${ip}                # 监听ip
      nic: ${nic}              # 监听网卡名，用于通过网卡名获取ip(优先用ip，没有则用网卡获取ip)
      port: ${port}             # 监听port
```
在registry插件(plugins/registry)配置中，配置北极星心跳上报所需的信息(服务命名空间、服务名、token、instanceid)。
```yaml
# 插件配置
plugins:
  registry:  # 服务注册配置
    polarismesh:  # 北极星服务实例注册插件
      service:
        - instance_id: ${instance_id}    # 服务实例的唯一id，不配置时框架采用server配置中同名service的ip:port（选填）
          name: ${service_name}          # 服务在北极星上的服务名（与前文的server/service/- name 一致）
          namespace: ${namespace}        # 命名空间
          token: ${token}                # 服务token
```
## 如何注册服务实例

### 开启框架的自注册

若未使用平台发布服务，且用户对服务实例的权重，心跳检测周期等没有特殊要求，建议开启框架的自注册功能。
**使用流程：**
**step1（申请一个北极星服务名）: **在北极星管理平台页面注册服务。通过登陆[北极星管理平台](http://192.127.0.0:8080/#/login)注册一个服务，记住服务名service_name，命名空间namespace以及token。
**step2（让框架自动注册服务实例）: **填写对应的配置文件配置项
server配置
```yaml
# 服务端配置
server:
  registry_name: polaris       # 指定向哪个名字服务进行注册
  service:
    - name: ${service_name}    # 服务在北极星上的服务名，注意区分这里和四段式命名的区别
      ip: ${ip}                # 监听ip
      port: ${port}             # 监听port
```
register配置
```yaml
# 插件配置
plugins:
  registry:  # 服务注册配置
    polarismesh:  # 北极星服务实例注册插件
      service:
        - name: ${service_name}          # 服务在北极星上的服务名
          namespace: ${namespace}        # 命名空间
          token: ${token}                # 服务token
          metadata:                      # 服务实例的metadata
            key1: value1
            key2: value2
```

### 通过插件接口注册
若业务对服务实例的权重，心跳检测周期等有特殊要求，建议直接调用名字服务插件的Register接口进行注册或者调用Unregister接口进行反注册。
**使用流程：**
**step1：**在北极星管理平台页面注册服务。通过登陆[北极星管理平台](http://192.127.0.0:8080/#/login)注册一个服务，记住服务名service_name，命名空间namespace以及token（界面上像指纹的图标）
**step2：**手动调用插件注册与反注册接口
```
#服务端配置
server:
  registry_name: polaris       # 指定向哪个名字服务进行注册
  service:
    - name: ${service_name}    # 服务在北极星上的服务名，注意区分这里和四段式命名的区别
      ip: ${ip}                # 监听ip
      port:${port}             # 监听port
```
register配置
```registry yaml
#插件配置
plugins:
  registry:  # 服务注册配置
    polarismesh:  # 北极星服务实例注册插件
      register_self: true  # 北极星服务实例启动自注册
      service:
        - name: ${service_name}          # 服务在北极星上的服务名
          namespace: ${namespace}        # 命名空间
          token: ${token}                # 服务token
          metadata:                      # 服务实例的metadata
            key1: value1
            key2: value2
```

### 通过插件接口注册
若业务对服务实例的权重，心跳检测周期等有特殊要求，建议直接调用名字服务插件的Register接口进行注册或者调用Unregister接口进行反注册
**使用流程：**
**step1: **在北极星管理平台页面注册服务。通过登陆[北极星管理平台](http://192.127.0.0:8080/#/login)注册一个服务，记住服务名service_name，命名空间namespace以及token（界面上像指纹的图标）
**step2: **手动调用插件注册与反注册接口
```cpp
trpc::TrpcRegistryInfo registry_info;
// 必填项
registry_info.plugin_name = "polarismesh"; // 使用北极星naming插件
registry_info.registry_info.name = service_name; // 服务在北极星上的服务名
registry_info.registry_info.host = FLAGS_ip; // 服务实例自身ip
registry_info.registry_info.port = FLAGS_port; // 服务实例自身port

// 注册服务实例，成功返回0
int ret = trpc::naming::Register(registry_info);

// 注册成功后，可以从meta中拿到服务实例在北极星上的唯一id
std::cout << registry_info.registry_info.meta["instance_id"];

// 注销服务实例，成功返回0
ret = trpc::naming::Unregister(registry_info);

// trpc::TrpcPlugin::GetInstance()->UnregisterPlugins();
```

# 北极星限流插件（limiter）

## 插件注册位置
```yaml
# 插件配置
plugins:
  limiter: # 限流插件配置
    polarismesh: # 北极星限流插件
      updateCallResult: false # 是否需要上报调用情况（用于动态调整配额），如果需要的话需要配为true
      mode: global  # 限流模式，global或local，其中global模式的话配额所有实例共享，local的话配额不共享(不需要访问后端配额服务器动态计算剩余配额)
      rateLimitCluster:  # 限流统计集群，必填，具体配置参考北极星官方限流文档(请将xxx替换成接入的限流统计集群)
        namespace: Devlopment
        service: xxx
```

## 限流功能
1. 框架实现的polarismesh_limiter filter传入的限流label为`method:RPC接口名`(对于trpc协议，限流插件用的RPC接口名为最后一段，如SayHello) 及 `caller:主调名`，如果使用框架实现的polarismesh_limiter filter来限流的话，在配置限流规则时，需要使用这两个label，才能触发限流。如果需要传递额外的label，用户需要基于polaris limter插件自行实现客户端或服务端的limiter filter，然后将其注册到框架中，再使用自定义的filter限流
2. 一般情况下只需要单测（服务端或客户端）限流即可，同时polarismesh_limiter建议作为service级别的filter使用

### 服务端限流
**开启方式**：
```yaml
server:
  service:
    - name: trpc.app.server.service
      filter:
        - polarismesh_limiter
```

### 客户端限流
**开启方式**：
```yaml
client:
  service:
    - name: trpc.app.server.service
      filter:
        - polarismesh_limiter
```

### 简单的客户端限流示例（caller和method维度均支持，这里以caller维度为例）
**场景：**下游服务为trpc.trpctest.helloworld.Greeter，有两个上游服务trpc.A和trpc.B。现在要根据上游服务名分别对trpc.A和trpc.B限流，trpc.A允许1s最多请求1次，trpc.B允许1s最多请求2次。

**北极星界面配置：**配置限流规则请移步到北极星管理平台（http://192.127.0.0:8080）
**服务配置：**这里选择在客户端做限流配置
```yaml
client:
  service:
    - name: trpc.hanqintrpctest.helloworld.Greeter
      target: trpc.hanqintrpctest.helloworld.Greeter
      namespace: Development
      ...
      filter:
        - polarismesh_limiter
plugins:
  limiter:
    polarismesh:
      updateCallResult: false
      mode: local
```

### 简单的服务端限流示例（caller和method维度均支持，这里以method维度为例）
**场景：**下游服务为trpc.peggiezhutest.limiter.Greeter，有SayHello和SayWorld两个RPC接口。现在要根据RPC接口名分别对SayHello和SayWorld限流，SayHello允许5s最多请求1次，SayWorld允许5s最多请求2次。
以trpc协议为例，对应proto里的
```
service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}

  rpc SayWorld (HelloRequest) returns (HelloReply) {}
}
```
**北极星界面配置：**配置限流规则请移步到北极星管理平台（http://192.127.0.0:8080）
因为是采用PRC接口名做匹配，维度名称填“method”。
**服务配置：**这里选择在服务端做限流配置
```yaml
server:
  service:
    - name: trpc.peggiezhutest.limiter.Greeter
      target: trpc.peggiezhutest.limiter.Greeter
      namespace: Development
       ...
      filter:
        - polarismesh_limiter
plugins:
  limiter:
    polarismesh:
      updateCallResult: false
      mode: local
```