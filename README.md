[中文](./README.zh_CN.md)

# Overview
[PolarisMesh](https://polarismesh.cn) is a service governance platform developed by Tencent in collaboration with the open-source community. The tRPC-Cpp Polaris plugin encapsulates the [official Polaris SDK](https://github.com/polarismesh/polaris-cpp).
The tRPC-Cpp Polaris plugin is divided into two types:**Polaris routing selection plugin** and **Polaris service instance registration plugin**, The Polaris routing selection plugin provides service discovery functionality, and the Polaris service instance registration plugin provides service instance registration and heartbeat reporting functionality.

# Polaris Routing Selection Plugin(selector)
The plugin provides service discovery functionality: first, it filters a subset of instance nodes that meet the requirements through **scheduling strategies** and then filters a single instance node that meets the requirements through **load balancing strategies**
The supported scheduling strategies include proximity routing and rule routing.
The supported load balancing strategies include weighted random and consistent hashing.

## Plugin Registration Location
```yaml
# Plugin configuration
plugins:
  selector:  # Routing selection configuration
    polarismesh:  # Polaris routing selection plugin
```

## Scheduling Strategy
In the Polaris routing selection plugin configuration, if the consumer configuration item is not configured, the framework will enable all the following scheduling strategies by default.
**Note: The service routing chain has an order (the ones above are executed first), so the default filtering order is ruleRouter->nearbyRouter.**
```yaml
# Plugin configuration
plugins:
  selector:  # Routing selection configuration
    polarismesh:  # Polaris routing selection plugin
      consumer:
        serviceRouter:
          chain: # Service routing chain, all the following strategies are enabled by default
            - ruleRouter  # Rule routing plugin, used for rule filtering
            - nearbyRouter  # Proximity routing plugin, used for proximity access
```

### Proximity Routing
Configuration item: `chain: - nearbyRouter`
Access the nearest service instance according to the machine room region information of the service instance. Priority: City > Region (East China, South China, etc.) > National.
**Applicable scope**
As long as the corresponding service is enabled for "proximity access" on the Polaris management interface, it can be used.
**Usage steps**
**step1:** Enable **Proximity Access**
**step2:** Enable the nearbyRouter item in the framework configuration, as shown in the default configuration above.
**Note** After the service is enabled for proximity routing, the caller will route to the callee in the same city by default through the service discovery interface. When all the callee in the same city are unavailable(**non-existent/unhealthy/fused**)it will downgrade to the callee in the same region. When all the callee in the same region are unavailable, it will downgrade to the callee in other national ranges.

### Rule Routing
Configuration item: `chain: - ruleRouter`
Traffic scheduling based on the routing rules configured by the user on Polaris. When using, pass the corresponding rule routing label, trigger the corresponding matching rules, and achieve targeted traffic distribution.
**Usage steps**
step1: Configure routing rules for the service on Polaris
step2: Enable the`ruleRouter`item in the framework configuration, as shown in the default configuration above
step3: When the caller initiates a call, set the rule routing label to be used
```cpp
// Set the label for rule routing filtering
std::map<std::string, std::string> ruleroute_label;
// key/value is the label for rule routing filtering
ruleroute_label["key"] = "value";
// Set the rule routing filter label to the client_context (ClientContext object)
trpc::naming::polarismesh::SetFilterMetadataOfNaming(ruleroute_label, trpc::NamingMetadataType::kRuleRouteLable);
```

## Load Balancing Strategy
By default, the framework uses the **weighted random**to return service instance nodes if no load balancing is specified. If you want to use other load balancing methods, you need to **explicitly specify** them in the framework configuration.

### Weighted Random
The framework uses the built-in weight-round-random (wrr) strategy of the Polaris SDK by default.

### Consistent Hashing
Select a specific service instance node by specifying a specific hash key.
**Usage process**
**step1:** Users need to set the hash key to the context. After setting it successfully, the framework will use **consistent hashing ** by default to return the corresponding service instance node, as follows:
```cpp
trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>();
trpc::naming::polarismesh::SetSelectorExtendInfo(ctx, std::make_pair("hash_key", "1"));
// Initiate rpc call, prx is the service proxy
trpc::Status status = prx->RpcMethod(ctx, ...);
```
**step2:** Explicitly specify the hash strategy
**Note** For new businesses, the consistent hashing algorithm should be set to **ringhash** . As for modulohash, brpcmurmurhash, and other algorithms, they are generally set when migrating existing services that do not want to change the already used consistent hashing algorithm (different algorithms have different instance nodes corresponding to the same key).
ringhash: corresponds to the Polaris SDK's kLoadBalanceTypeRingHash.
modulohash: corresponds to the Polaris SDK's kLoadBalanceTypeSimpleHash // hash_key% total number of instances to select the service instance.
brpcmurmurhash: corresponds to the Polaris SDK's kLoadBalanceTypeCMurmurHash // compatible with brpc c_murmur consistent hashing.
```yaml
client:
  service:
    - name: trpc.peggiezhutest.helloworld.Greeter
      load_balance_name: ringhash
```
**step3:**You can check the logs to confirm whether the filled hash key is routed to the same service instance node (adjust the log level to DEBUG or TRACE).
For synchronous invocation, search: "Select result of " + called service name.
For asynchronous invocation, search: "AsyncSelect result of " + called service name.

#### Hash ring algorithm, returning the adjacent node corresponding to the key
If you want to get the adjacent nodes of the hash ring algorithm (such as ringhash), please explicitly call the SetReplicateIndex method of ClientContext to set it; if not set, the ReplicateIndex defaults to 0, which means returning the current node corresponding to the hash key.
```cpp
trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>();
// Set the hash key to be used (not required to be an integer), as long as it is not empty
// Get the first adjacent node (clockwise) corresponding to the hash key, and pass in 2 to get the second adjacent node, and so on
trpc::naming::polarismesh::SetSelectorExtendInfo(ctx, std::make_pair("hash_key", "abc"), std::make_pair("replicate_index", "1"));
```

#### Locality-aware load balancing algorithm
The corresponding framework configuration is as follows:
```yaml
client:
  service:
    - name: trpc.peggiezhutest.helloworld.Greeter
      load_balance_name: localityAware
```

#### Dynamic weight configuration (excluding server-side reporting functionality)
**step1:** Framework configuration file, enable the dynamic weight thread of the SDK
```yaml
plugins:
  selector:
    polarismesh:
      dynamic_weight:
        isOpenDynamicWeight: true # Set to true
      consumer:
        loadbalancer:
          enableDynamicWeight: true # Set to true
          type: weightedRandom # Load balancing type (although the specific load balancing algorithm used is dynamicWeight, the basic weighted random algorithm weightedRandom should be filled here)
```
**step2:** Client call
```yaml
client:
  service:
    - name: trpc.peggiezhutest.helloworld.Greeter
      load_balance_name: dynamicWeight
```

## Service Circuit Breaker
The Polaris cpp SDK calculates the failure rate and consecutive failures of the called nodes for a certain period based on the node call situation reported by the user. If the circuit breaker conditions (consecutive failures or failure rate exceeding the standard) are met, the node will be added to the list of circuit breaker instances, and the next scheduling will not include the circuit breaker nodes. At the same time, the circuit breaker nodes will be probed. When the node is detected to recover (if the probe function is not enabled, it will be after a period), it will be set to a semi-open state. If the call is successful during the semi-open state, the node will be restored to the closed state (normal); if the call fails, it will be restored to the open state (circuit breaker).
The circuit breaker function is enabled by default. If you want to disable it, you need to set the configuration item enableCircuitBreaker to false.
```yaml
plugins:
  selector:
    polarismesh:
      consumer:
        circuitBreaker:
          enableCircuitBreaker: true # Whether to enable the circuit breaker function, default: true
          errorPercentThreshold: 50 # The percentage of errors that triggers the circuit breaker, default: 50
          consecutiveErrorThreshold: 5 # The number of consecutive errors that triggers the circuit breaker, default: 5
          halfOpenMaxFailedCount: 5 # The maximum number of consecutive failures in the semi-open state, default: 5
          checkPeriod: 1000 # The time interval for checking the circuit breaker status, in milliseconds, default: 1000
          halfOpenRecoverTime: 3000 # The time required for the open state to transition to the semi-open state, in milliseconds, default: 3000
          closeRecoverTime: 5000 # The time required for the semi-open state to transition to the closed state, in milliseconds, default: 5000
```
The triggering and recovery conditions for circuit breaking are as follows:
-  **Specific triggering conditions for circuit breaking strategy**
   Circuit breaking occurs after 10 consecutive call failures or if the failure rate is >50% within 1 minute (provided that there are 10 calls).
-  **Recovery condition**
   If more than 8 out of 10 calls are successful within 30s, the circuit breaking node will be restored.

## How to initiate rpc calls through plugins
It can be done by configuration file or by specifying the code. Make sure the namespace is correct and the selector_name is set to polaris.

# Polaris service instance registration plugin (registry)
Provides service instance registration and heartbeat reporting functions.

## Plugin registration location
```yaml
# Plugin configuration
plugins:
  registry:  # Service registration configuration
    polarismesh:  # Polaris service instance registration plugin
```

## Heartbeat reporting function
**The tRPC-Cpp framework automatically performs heartbeat reporting**, The heartbeat reporting related configuration is as follows:
**Note: The IP in the framework configuration must be configured reasonably for the framework to perform heartbeat reporting.**
- If the service is configured with a listening IP, it will be used first.
- If the service is not configured with a listening IP, the framework will check if the service is configured with a nic (listening network card name) and obtain the IP from the corresponding network card.

In the global configuration, a namespace is required.
```global yaml
#Global configuration
global:
  namespace: ${namespace}    #Namespace, such as Production and Development
```
In the server configuration, you need to !!#ff0000 **add the configuration registry_name as polaris**
```yaml
# Server configuration
server:
  registry_name: polaris
  service:
    - name: ${service_name}    # The service name on Polaris, note the difference between this and the four-segment naming
      ip: ${ip}                # Listening IP
      nic: ${nic}              # Listening network card name, used to obtain IP through the network card name (priority is given to IP, otherwise use the network card to obtain IP)
      port: ${port}             # Listening port
```
In the registry plugin (plugins/registry) configuration, configure the information required for Polaris heartbeat reporting (service namespace, service name, token, instanceid).
```yaml
# Plugin configuration
plugins:
  registry:  # Service registration configuration
    polarismesh:  # Polaris service instance registration plugin
      service:
        - instance_id: ${instance_id}    # The unique ID of the service instance, if not configured, the framework will use the ip:port of the same name service in the server configuration (optional)
          name: ${service_name}          # The service name on Polaris
          namespace: ${namespace}        # Namespace
          token: ${token}                # Service token
```
## How to register a service instance

### Enable the tRPC-Cpp framework's self-registration
If the service is not published using the platform and the user has no special requirements for the service instance's weight, heartbeat detection cycle, etc., it is recommended to enable the framework's self-registration function.
**Usage process:**
- step1 (Apply for a Polaris service name): Register a service on the Polaris management platform page. Log in to the [Polaris management platform](http://192.127.0.0:8080/#/login)to register a service, remember the service_name, namespace, and token.
- step2 (Let the framework automatically register the service instance): Fill in the corresponding configuration file configuration items server configuration
```yaml
# Server configuration
server:
  registry_name: polaris       # Specify which name service to register with
  service:
    - name: ${service_name}    # The service name on Polaris, note the difference between this and the four-segment naming
      ip: ${ip}                # Listening IP
      port: ${port}             # Listening port
```
register configuration
```yaml
# Plugin configuration
plugins:
  registry:  # Service registration configuration
    polarismesh:  # Polaris service instance registration plugin
      register_self: true # The service instance starts self-registration
      service:
        - name: ${service_name}          # The service name on Polaris
          namespace: ${namespace}        # Namespace
          token: ${token}                # Service token
          metadata:                      # Metadata of the service instance
            key1
```

### Register through the plugin interface
If the business has special requirements for the service instance's weight, heartbeat detection cycle, etc., it is recommended to directly call the naming service plugin's Register interface for registration or call the Unregister interface for deregistration.
**Usage process:**
- Step1: Register a service on the Polaris management platform page. Log in to the [Polaris management platform](http://192.127.0.0:8080/#/login) to register a service, remember the service_name, namespace, and token (the icon that looks like a fingerprint on the interface).
- Step2: Manually call the plugin registration and deregistration interface
```cpp
trpc::TrpcRegistryInfo registry_info;
// Required items
registry_info.plugin_name = "polarismesh"; // Use the Polaris naming plugin
registry_info.registry_info.name = service_name; // The service name on Polaris
registry_info.registry_info.host = FLAGS_ip; // Service instance's own IP
registry_info.registry_info.port = FLAGS_port; // Service instance's own port

// Register the service instance, return 0 on success
int ret = trpc::naming::Register(registry_info);

// After successful registration, you can get the unique ID of the service instance on Polaris from the meta
std::cout << registry_info.registry_info.meta["instance_id"];

// Deregister the service instance, return 0 on success
ret = trpc::naming::Unregister(registry_info);
```

# Polaris rate limiting plugin (limiter)
## Plugin registration location
```yaml
# Plugin configuration
plugins:
  limiter: # Rate limiting plugin configuration
    polarismesh: # Polaris rate limiting plugin
      updateCallResult: false # Whether to report call results (used for dynamic quota adjustment), if needed, set to true
      mode: global  # Rate limiting mode, global or local, where global mode shares quotas among all instances, local mode does not share quotas (no need to access the backend quota server for dynamic calculation of remaining quotas)
      rateLimitCluster:  # Rate limiting statistics cluster, required, specific configuration reference Polaris official rate limiting documentation (please replace xxx with the rate limiting statistics cluster you are accessing)
        namespace: Devlopment
        service: xxx
```

## Rate limiting function
1. The polarismesh_limiter filter implemented by the framework passes in the rate limiting label as `method: RPC interface name`(for trpc protocol, the RPC interface name used by the rate limiting plugin is the last segment, such as SayHello and `caller: main caller name`. If you use the polarismesh_limiter filter implemented by the framework for rate limiting, you need to use these two labels when configuring rate limiting rules to trigger rate limiting. If you need to pass additional labels, the user needs to implement the client or server side limiter filter based on the polaris limter plugin, register it in the framework, and then use the custom filter for rate limiting.
2. In general, only single test (server-side or client-side) rate limiting is needed, and polarismesh_limiter is recommended as a service-level filter.

### Server-side rate limiting
**How to enable:**
```yaml
server:
  service:
    - name: trpc.app.server.service
      filter:
        - polarismesh_limiter
```

### Client-side rate limiting
**How to enable:**
```yaml
client:
  service:
    - name: trpc.app.server.service
      filter:
        - polarismesh_limiter
```

### Simple client-side rate limiting example (supports both caller and method dimensions, here using caller dimension as an example)
- Scenario: The downstream service is trpc.trpctest.helloworld.Greeter, with two upstream services trpc.A and trpc.B. Now, based on the upstream service name, rate limiting is applied separately for trpc.A and trpc.B, with trpc.A allowing a maximum of 1 request per second and trpc.B allowing a maximum of 2 requests per second.
- Polaris interface configuration: **To configure rate limiting rules, please go to the Polaris management platform（http://192.127.0.0:8080）
- Service configuration: **Here, choose to configure rate limiting on the client side
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

### Simple server-side rate limiting example (supports both caller and method dimensions, here using method dimension as an example)
- Scenario: The downstream service is trpc.peggiezhutest.limiter.Greeter, with SayHello and SayWorld as two RPC interfaces. Now, based on the RPC interface name, rate limiting is applied separately for SayHello and SayWorld, with SayHello allowing a maximum of 1 request per 5 seconds and SayWorld allowing a maximum of 2 requests per 5 seconds. Taking the trpc protocol as an example, the corresponding proto is:
```
service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}

  rpc SayWorld (HelloRequest) returns (HelloReply) {}
}
```
- Polaris interface configuration: **To configure rate limiting rules, please go to the Polaris management platform(http://192.127.0.0:8080).
Since the PRC interface name is used for matching, fill in the dimension name as "method".
- Service configuration: **Here, choose to configure rate limiting on the server side
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