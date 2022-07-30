
Pod的基本概念:
- 最小的调度/部署单元
- 包括一个或多个容器(一组应用的集合)

- 一个Pod共享同一网络(namespace, ip, mac addr), 存储(volumn)
  - 每个Pod都有唯一根容器(Pause/Info容器), 会为其分配网络命名空间namespace, ip, mac addr等
  - 业务容器, 在创建后会加入根容器, 使其与根容器在同一namespace下, 从而实现网络共享
  - Pod可以用volumns申明数据卷, 容器可以用volumnMounts来挂载, 从而实理数据共享
- Pod是短暂的, 随时可以从一个Node中下线, 有另一个Node拉起

Pod多容器设计的优点:
- Pod是多进程设计, 一个容器运行一个应用, 多个容器运行一组应用
- Pod存在是为了亲密性应用
  - 两个应用之间进行交互
  - 网络之间调用
  - 两个应用李频繁调用



- env
  - name:
  - value: 
- imagePullPolicy: 镜像拉起策略, 可以取如下三个值
  - IfNotPresent, 默认值, 不存在就拉取
  - Always, 每次Pod启动都会拉启一次
  - Never, 不主动拉启
- resources: 资源限制 (由docker实现)
  - requests:
    - cpu: "250m"
    - memory": "64Mi"
  - limits:
    - cpu: "500m"
    - memory": "128Mi"
- restartPolicy, 重启机制, 可以取如下三个值
  - Always: 总是重启, 一般用于一直提供服务的容器
  - OnFailure: 退出码非0时才重启, 一般用于一次性运行应用
  - Never: 不重启
- livenessProbe, 存活检查, 如果失败, 就kill容器, 然后根据Pod的restartPolicy操作
  - exec, 0
  - httpGet, 200~400
  - tcpGet, 成功建立连接
  - initialDelaySeconds
  - periodSeconds
- readinessProbe, 就绪检查, 如果失败, K8s会将Pod从service endpoints中剔除





用于强制约束将Pod调度到指定的Node节点上，这里说是“调度”，但其实指定了nodeName的Pod会直接跳过Scheduler的调度逻辑，直接写入PodList列表，该匹配规则是强制匹配

```yaml
apiVersion: extensions/v1beta1
kind: Deployment
metadata:
  name: tomcat-deploy
spec:
  replicas: 1
  template:
    metadata:
      labels:
        app: tomcat-app
    spec:
      nodeName: k8s.node1 #指定调度节点为k8s.node1
      containers:
      - name: tomcat
        image: tomcat:8.0
        ports:
        - containerPort: 8080
```

>> kubectl label nodes <node-name> <label-key>=<label-value>
>> kubectl get nodes <node-name> --show-labels

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: with-node-affinity
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: kubernetes.io/os
            operator: In
            values:
            - linux
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 1
        preference:
          matchExpressions:
          - key: another-node-label-key
            operator: In
            values:
            - another-node-label-value
  containers:
  - name: with-node-affinity
    image: k8s.gcr.io/pause:2.0
```

nodeSelector:
  label_name: value
nodeAffinity:
  requiredDringSchedulingIgnoredDringExecution:
  nodeSelectorTerms:
  - matchExpressions:
    - key: env_role
      operator: In
      values:
      - dev
      - test
  preferredDringSchedulingIgnoredDringExecution:
  - weight: 1
    preference:
      matchExpressions:
        key: env_role
        operator: In
        values:
        - other_pod

Taint
kubectl traint node [node] key=value:(NoSchedule|PreferNoSchedule|NoExecute)
kubectl describe node [node] | grep Traint
kubectl traint node [node] key=value:(NoSchedule|PreferNoSchedule|NoExecute)-

- NoSchedule: 一定不被调度
- PreferNoSchedule: 尽量不被调度
- NoExecute: 不会调度, 对已调度上去的会驱逐Node已有Pod

tolerations:
- key: "key"
  value: "value"
  operator: "Equal"
  effect: "NoSchedule"


你可以约束一个 Pod 只能在特定的节点上运行。 有几种方法可以实现这点，推荐的方法都是用 标签选择算符来进行选择。 

通常这样的约束不是必须的，因为调度器将自动进行合理的放置（比如，将 Pod 分散到节点上， 而不是将 Pod 放置在可用资源不足的节点上等等）。但

在某些情况下，可能需要控制 Pod 被部署到的节点。例如，确保 Pod 最终落在连接了 SSD 的机器上， 或者将来自两个不同的服务且有大量通信的 Pods 被放置在同一个可用区。

节点标签

与很多其他 Kubernetes 对象类似，节点也有标签。 通过为节点添加标签，可以让 Pod 调度到特定节点或节点组上, 从而确保特定的 Pod 只能运行在具有一定隔离性，安全性或监管属性的节点上。

```bash
kubectl node node1 env_role=dev
kubectl get nodes [node_name] --show-labels
```

nodeSelector 是节点选择约束的最简单推荐形式