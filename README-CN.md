# Mini Complex Adaptive Systems（迷你复杂适应系统）

一套**从零开始、零依赖的 C 语言实现**，涵盖复杂适应系统（CAS）的基础模型与计算框架。每个子模块对应 MIT、Stanford 及 Santa Fe Institute 课程，覆盖基于智能体的建模、人工生命、复杂性经济学、群体智能、混沌边缘动力学、遗传算法、NK 适应度景观以及 SFI 综合方法论——全部面向复杂系统科学家构建。

## 子模块

| 子模块 | 主题 | 核心课程 |
|--------|------|----------|
| [mini-agents-adaptation-basics](mini-agents-adaptation-basics/) | 智能体、多臂老虎机、环境模型、博弈论、Q-learning/SARSA/TD(λ)、群体智能原语 | MIT 6.7900, Stanford CS229 |
| [mini-artificial-life-langton](mini-artificial-life-langton/) | 元胞自动机（1D/2D）、Langton 蚂蚁、Langton λ 参数、模式检测（静止型、振荡型、滑翔机）、人工生命度量 | MIT 6.045, Stanford CS358, SFI CSSS |
| [mini-cas-in-economics-arthur](mini-cas-in-economics-arthur/) | 基于智能体的计算经济学、收益递增与路径依赖、Polya 瓮过程、El Farol 酒吧问题、Santa Fe 人工股票市场、组合技术演化 | MIT 14.13, Stanford MS&E 448 |
| [mini-collective-intelligence-swarm](mini-collective-intelligence-swarm/) | 粒子群优化（PSO）、蚁群优化（ACO）、群集/boids、分布式共识、间接协调（stigmergy）与自组织、涌现集体动力学 | MIT 6.821, Stanford AA 174 |
| [mini-edge-of-chaos-criticality](mini-edge-of-chaos-criticality/) | Wolfram CA 分类、Langton λ 相变、自组织临界性（BTW 沙堆模型）、幂律分布检测与拟合、非线性动力学（分岔、Lyapunov）、分形几何 | MIT 6.045, Stanford CS358, Caltech CS154 |
| [mini-genetic-algorithms-holland](mini-genetic-algorithms-holland/) | GA 核心（选择、交叉、变异）、编码方案、适应度景观分析、种群管理、Schema 理论与积木块假说 | MIT 6.7201, Stanford EE 364a |
| [mini-nk-fitness-landscapes-kauffman](mini-nk-fitness-landscapes-kauffman/) | NK 景观生成、上位效应分析（幅度、符号、互逆符号上位）、适应性行走（贪心、最陡上升、Metropolis）、景观统计、NKCS 协同演化 | MIT 6.7201, Stanford MS&E 448 |
| [mini-sfi-methodology-synthesis](mini-sfi-methodology-synthesis/) | 宏观层面 CAS 类型、适应与复制动力学、复杂性度量（统计复杂度、Lempel-Ziv、有效复杂度）、涌现检测、ε-机器、网络科学方法 | SFI CSSS, Stanford CS358 |

## 设计理念

- **零外部依赖** — 纯 C99/C11，仅使用标准库头文件
- **自包含子模块** — 每个子模块拥有独立的 `include/`、`src/`、`CMakeLists.txt` 和冒烟测试
- **理论到代码的映射** — 每个模块内联引用基础文献（Holland、Kauffman、Arthur、Langton、Wolfram、Mitchell）与讲义
- **CAS 经典体系覆盖** — 所有实现与 Santa Fe Institute 复杂系统暑期学校（CSSS）核心课程对齐

## 构建

每个子模块独立构建。使用 CMake 构建：

```bash
cd mini-agents-adaptation-basics
mkdir build && cd build
cmake ..
make
./smoke_test
```

需要 **C99 兼容编译器**和 **CMake ≥ 3.14**。

## 项目结构

```
28. mini-complex-adaptive-systems/
├── mini-agents-adaptation-basics/      # 智能体、老虎机、博弈论、强化学习
├── mini-artificial-life-langton/       # 元胞自动机、Langton 蚂蚁、λ 参数、人工生命度量
├── mini-cas-in-economics-arthur/       # 计算经济学、收益递增、El Farol、Santa Fe 股票市场
├── mini-collective-intelligence-swarm/ # PSO、ACO、群集、共识、间接协调、涌现
├── mini-edge-of-chaos-criticality/     # Wolfram 分类、SOC 沙堆、幂律、分形
├── mini-genetic-algorithms-holland/    # 选择、交叉、变异、Schema 理论
├── mini-nk-fitness-landscapes-kauffman/ # NK 景观、上位效应、适应性行走、协同演化
├── mini-sfi-methodology-synthesis/     # SFI 经典：涌现、ε-机器、复杂度度量
├── .gitignore
├── README.md
└── README-CN.md
```

## 许可证

MIT
