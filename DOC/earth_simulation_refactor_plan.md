# 地球模拟改造方案

## 1. 背景与目标

当前项目是 `DaMaSCUS-SUN Trajectory TXT` 的轨迹文本输出版本，主程序会从暗物质速度分布中采样初始条件，将粒子传播到太阳外边界，再在太阳介质中按散射率进行自由传播、散射、俘获或反射判定，并把每条轨迹输出为 TXT。

本次改造目标是把“太阳模拟”扩展为“地球模拟”，即让同一套轨迹、散射和输出框架可以使用地球的半径、质量、内部密度、温度、元素组成、电子/核子数密度和逃逸速度。

建议目标分两层：

1. 短期目标：在尽量少改动核心算法的前提下，增加一个 `Earth_Model`，让程序可以跑地球版本轨迹。
2. 长期目标：把 `Solar_Model` 抽象为通用天体/介质模型，后续可支持 Sun、Earth、Moon、Jupiter 或自定义表格模型。

## 2. 当前代码现状

### 2.1 主入口

主入口在 `src/main.cpp`，主要流程如下：

1. 读取 libconfig 配置。
2. 构造 `DM_Particle`。
3. 构造 `DM_Distribution`。
4. 构造 `Solar_Model`。
5. 对总散射率做二维插值。
6. MPI 按 rank 并行模拟轨迹。
7. 每条轨迹写入 `trajectory_<id>_task<rank>.txt`。

当前输出列为：

```text
time_s  x_km  y_km  z_km  vx_km_s  vy_km_s  vz_km_s  E_eV
```

### 2.2 太阳模型耦合点

核心耦合集中在这些位置：

- `vendor/damascus/include/Solar_Model.hpp`
- `vendor/damascus/src/Solar_Model.cpp`
- `vendor/damascus/include/Simulation_Trajectory.hpp`
- `vendor/damascus/src/Simulation_Trajectory.cpp`
- `vendor/damascus/include/Simulation_Utilities.hpp`
- `vendor/damascus/src/Simulation_Utilities.cpp`
- `src/main.cpp`
- `config/example.cfg`
- `CMakeLists.txt`

其中太阳假设主要表现为：

- 类名固定为 `Solar_Model`。
- 半径固定使用 `rSun`。
- 质量固定使用 `mSun`。
- 数据文件固定为 `data/model_agss09.dat`。
- 初始边界配置使用 `initial_radius_rsun`。
- 轨迹 bin 常量固定为 `R_SUN_KM` 和 `2 R_sun`。
- 太阳外 Kepler 传播使用 `mSun`。
- 判断进入介质和离开介质时使用 `rSun`。
- 输出和 README 中项目名固定为 `DaMaSCUS-SUN`。

### 2.3 可复用部分

以下部分可以基本复用：

- 暗物质粒子构造：`Build_DM_Particle`。
- 暗物质速度分布构造：`Build_DM_Distribution`。
- `Event` 轨迹状态结构。
- RK45 自由传播框架。
- 散射事件采样流程。
- MPI 并行框架。
- TXT 轨迹输出格式。
- `obscura::DM_Particle`、核散射和电子散射接口。

需要注意：虽然算法框架可复用，但所有“天体半径、质量、内部介质、逃逸速度”的调用都要解除太阳硬编码。

## 3. 地球模型需要补齐的数据

地球模拟至少需要以下物理输入：

### 3.1 全局常量

- 地球半径 `R_earth`，建议先用平均半径 6371 km。
- 地球质量 `M_earth`，建议先用 5.9722e24 kg。
- 外部初始边界，建议默认 `2.0 * R_earth`。
- 渐近采样距离，当前太阳代码使用 `1000 AU`，地球版本不宜照搬，需要改成可配置，例如 `initial_asymptotic_distance_rearth` 或直接使用长度单位。

### 3.2 径向结构表

建议新增地球模型数据文件，例如：

```text
vendor/damascus/data/earth_prem.dat
```

最低字段建议：

```text
r_km  enclosed_mass_kg  density_g_cm3  temperature_K
```

如果没有可靠的 `enclosed_mass_kg`，可以由密度积分得到，但第一版建议离线预处理好，避免每次启动积分产生误差和开销。

### 3.3 元素或同位素组成

太阳模型 `model_agss09.dat` 里内置了随半径变化的元素质量分数。地球需要另建组成模型。可以先分阶段：

1. 第一版使用分层常数丰度：内核、地幔、地壳分别配置主要元素。
2. 第二版支持随半径插值的质量分数表。

建议元素至少覆盖：

- O
- Mg
- Si
- Fe
- Ni
- S
- Ca
- Al

如果只是做轨迹框架验证，可以先用简化 bulk Earth 组成；如果要做物理结果，需要补充 PREM 密度和更严格的地球化学组成。

### 3.4 电子数密度

可以沿用太阳模型的做法：由核素数密度乘以 `Z` 汇总得到电子数密度。

## 4. 推荐架构改造

### 4.1 第一阶段：新增 `Earth_Model`，保留 `Solar_Model`

这是风险最低的路线。

新增文件：

```text
vendor/damascus/include/Earth_Model.hpp
vendor/damascus/src/Earth_Model.cpp
vendor/damascus/data/earth_prem.dat
config/earth_example.cfg
```

`Earth_Model` 第一版可以复制 `Solar_Model` 的公共接口，保持调用侧改动较小：

```cpp
class Earth_Model
{
  public:
    std::string name;
    std::vector<Earth_Isotope> target_isotopes;

    double Radius() const;
    double Total_Mass() const;
    double Mass(double r);
    double Mass_Density(double r);
    double Temperature(double r);
    double Local_Escape_Speed(double r);
    double Debye_Screening_Scale_Squared(double r);

    double Number_Density_Nucleus(double r, unsigned int nucleus_index);
    double Number_Density_Electron(double r);

    double DM_Scattering_Rate_Electron(obscura::DM_Particle& DM, double r, double DM_speed);
    double DM_Scattering_Rate_Nucleus(obscura::DM_Particle& DM, double r, double DM_speed, unsigned int nucleus_index);
    double Total_DM_Scattering_Rate(obscura::DM_Particle& DM, double r, double DM_speed);
    void Interpolate_Total_DM_Scattering_Rate(obscura::DM_Particle& DM, unsigned int N_radius, unsigned int N_speed);
};
```

为减少模板化改造量，可以先让主程序选择：

```text
body = "Earth";
```

然后在 `main.cpp` 中根据 `body` 构造对应模型。缺点是 `Trajectory_Simulator` 当前成员类型是 `Solar_Model`，所以第一阶段仍需要二选一：

1. 复制一个 `Earth_Trajectory_Simulator`，最快但重复多。
2. 立即做通用接口抽象，工作量稍大但更干净。

推荐选第 2 种。

### 4.2 第二阶段：抽象通用天体模型

新增通用接口：

```text
vendor/damascus/include/Celestial_Model.hpp
```

建议接口：

```cpp
class Celestial_Model
{
  public:
    virtual ~Celestial_Model() {}

    virtual const std::string& Name() const = 0;
    virtual double Radius() const = 0;
    virtual double Total_Mass() const = 0;

    virtual double Mass(double r) = 0;
    virtual double Mass_Density(double r) = 0;
    virtual double Temperature(double r) = 0;
    virtual double Local_Escape_Speed(double r) = 0;
    virtual double Debye_Screening_Scale_Squared(double r) = 0;

    virtual unsigned int Target_Count() const = 0;
    virtual const obscura::Isotope& Target_Isotope(unsigned int index) const = 0;
    virtual double Number_Density_Nucleus(double r, unsigned int index) = 0;
    virtual double Number_Density_Electron(double r) = 0;

    virtual double DM_Scattering_Rate_Electron(obscura::DM_Particle& DM, double r, double DM_speed) = 0;
    virtual double DM_Scattering_Rate_Nucleus(obscura::DM_Particle& DM, double r, double DM_speed, unsigned int index) = 0;
    virtual double Total_DM_Scattering_Rate(obscura::DM_Particle& DM, double r, double DM_speed) = 0;
    virtual void Interpolate_Total_DM_Scattering_Rate(obscura::DM_Particle& DM, unsigned int N_radius, unsigned int N_speed) = 0;
};
```

然后把以下签名从 `Solar_Model&` 改为 `Celestial_Model&`：

- `Event::Asymptotic_Speed_Sqr`
- `Initial_Conditions`
- `Outward_Escaping_At_Boundary`
- `Trajectory_Result::Particle_Captured`
- `Trajectory_Result::Print_Summary`
- `Trajectory_Simulator`
- `Free_Particle_Propagator::Runge_Kutta_45_Step`
- `Simulate_Trajectory_To_Txt`
- `Event_Energy_Ev`
- `Write_Trajectory_Row`

### 4.3 替换硬编码太阳常量

需要把这些逻辑替换为模型接口：

| 当前写法 | 改造后 |
| --- | --- |
| `rSun` | `body.Radius()` |
| `mSun` | `body.Total_Mass()` 或 `body.Mass(r)` |
| `initial_radius_rsun` | `initial_radius_body_radius` |
| `R_SUN_KM` | `body_radius_km` 或配置值 |
| `BIN_MAX_KM = 2 R_sun` | `bincount_max_radius_body_radius * body.Radius()` |
| `model_agss09.dat` | `body` 对应的数据文件 |
| `Solar_Model solar_model` | `std::unique_ptr<Celestial_Model> body_model` |

### 4.4 地球外 Kepler 传播

`Hyperbolic_Kepler_Shift` 当前使用 `mSun` 和 `rSun`。建议改为：

```cpp
void Hyperbolic_Kepler_Shift(Event& event, const Celestial_Model& body, double R_final);
```

内部使用：

```cpp
double M = body.Total_Mass();
double R = body.Radius();
```

并把错误信息从 “inside the Sun” 改成 “inside the body”。

### 4.5 初始条件采样

`Initial_Conditions` 当前按“会打到太阳”的粒子采样。地球版本可以沿用同一思想，但参数必须改成天体半径和质量：

- `PDF_Initial_Speed` 使用 `body.Local_Escape_Speed(body.Radius())`。
- 最大撞击参数使用 `body.Radius()`。
- 蓝移速度使用 `body.Local_Escape_Speed(asymptotic_distance)`。
- `asymptotic_distance` 改成配置项，默认值按地球半径设置，例如 `1000 * R_earth`，不要继续使用 `1000 AU`。

## 5. 配置文件改造

建议新增配置：

```text
body = "Earth";
initial_radius_body_radius = 2.0;
asymptotic_distance_body_radius = 1000.0;
bincount_max_radius_body_radius = 2.0;
body_model_file = "vendor/damascus/data/earth_prem.dat";
body_composition = "layered";
```

保留旧配置兼容：

```text
initial_radius_rsun = 2.0;
```

兼容策略：

1. 如果 `body` 不存在，默认 `Sun`。
2. 如果 `body = "Sun"` 且存在 `initial_radius_rsun`，继续使用旧字段。
3. 如果 `body = "Earth"`，优先读取 `initial_radius_body_radius`。
4. 旧字段在 README 中标记为 deprecated。

建议新增文件：

```text
config/earth_example.cfg
config/earth_smoke.cfg
```

## 6. CMake 改造

`CMakeLists.txt` 当前强制检查：

```cmake
data/model_agss09.dat
```

建议改为：

1. 保留太阳数据检查。
2. 增加地球数据检查。
3. 新增源文件：

```cmake
set(DAMASCUS_BODY_MODEL_SOURCES
  "${DAMASCUS_SRC_DIR}/Solar_Model.cpp"
  "${DAMASCUS_SRC_DIR}/Earth_Model.cpp"
)
```

如果做通用抽象，再加入：

```cmake
"${DAMASCUS_SRC_DIR}/Celestial_Model.cpp"
```

可执行文件名建议逐步改：

第一阶段保留旧名，降低脚本兼容成本：

```text
DaMaSCUS-SUN-TrajectoryTXT
```

第二阶段改成通用名：

```text
DaMaSCUS-Body-TrajectoryTXT
```

## 7. 实施路线

### 阶段 A：准备和护栏

目标：在不改变行为的前提下建立验证基线。

任务：

1. 增加太阳 smoke test 的固定随机种子配置。
2. 记录当前 `config/smoke.cfg` 的输出统计。
3. 给 `main.cpp` 的配置读取增加 `body = "Sun"` 默认值，但暂时不改变行为。
4. 给 README 增加“当前默认 body 是 Sun”的说明。

验收：

- `config/smoke.cfg` 输出统计与当前版本一致或在随机采样误差内。
- 旧配置不需要修改即可运行。

### 阶段 B：抽象 `Celestial_Model`

目标：让轨迹代码不再依赖 `Solar_Model` 具体类型。

任务：

1. 新增 `Celestial_Model.hpp`。
2. 让 `Solar_Model` 继承 `Celestial_Model`。
3. 把 `Simulation_Utilities` 和 `Simulation_Trajectory` 中的 `Solar_Model&` 改为 `Celestial_Model&`。
4. 把 `rSun`、`mSun` 替换为 `body.Radius()`、`body.Total_Mass()`。
5. 把 `target_isotopes` 直接访问改为接口访问。

验收：

- 太阳配置仍可编译运行。
- 输出轨迹行格式不变。
- CMake target 不变。

### 阶段 C：接入 `Earth_Model`

目标：可以用地球半径、质量和径向表运行地球模拟。

任务：

1. 新增 `Earth_Model.hpp/.cpp`。
2. 新增 `earth_prem.dat` 或临时简化地球表。
3. 实现地球的 `Mass`、`Mass_Density`、`Temperature`、`Local_Escape_Speed`。
4. 实现地球核素组成和电子密度。
5. 在 `main.cpp` 根据 `body` 创建 `Solar_Model` 或 `Earth_Model`。
6. 新增 `config/earth_smoke.cfg`。

验收：

- `body = "Earth"` 可以完成 1 rank smoke run。
- 轨迹文件目录能明确区分 `earth` 和参数点。
- 地球外部逃逸速度满足数量级检查：表面约 11.2 km/s。

### 阶段 D：输出和文档完善

目标：让使用者清楚知道当前模拟的是哪个天体。

任务：

1. 输出启动信息增加 body 名称、半径、质量、模型文件。
2. 结果目录增加 body 前缀，例如：

```text
output/earth/results_<log10(mass)>_<log10(sigma)>
```

3. README 拆分太阳和地球运行示例。
4. 文档说明地球模型数据来源、字段和单位。

验收：

- 输出目录不会和太阳结果混在一起。
- README 中有完整地球 smoke 命令。

### 阶段 E：物理校验

目标：避免代码能跑但物理含义错误。

建议校验项：

1. `Earth_Model::Mass(0)` 接近 0。
2. `Earth_Model::Mass(R_earth)` 接近 `M_earth`。
3. `Earth_Model::Local_Escape_Speed(R_earth)` 接近 11.2 km/s。
4. `Earth_Model::Local_Escape_Speed(r)` 在地球外满足 `sqrt(2GM/r)`。
5. `Total_DM_Scattering_Rate` 在 `r > R_earth` 为 0。
6. `Number_Density_Electron` 在 `r > R_earth` 为 0。
7. 插值域覆盖 `0 <= r <= R_earth`。
8. `Initial_Conditions` 的最大撞击参数随 `R_earth` 缩放。

## 8. 主要风险

### 8.1 地球组成模型不够精确

如果只用 bulk Earth 常数丰度，散射率会缺少径向结构。可以先作为工程 smoke 版本，但正式结果应使用分层或径向插值丰度。

### 8.2 温度模型影响热速度采样

`Sample_Target_Velocity` 使用局部温度计算热靶速度。地球内部温度远低于太阳，但仍会影响散射后速度分布。温度表不能简单设为 0，否则会导致除零或数值异常。

### 8.3 初始条件的天体尺度

太阳版本使用 `1000 AU` 作为渐近距离，地球版本如果照搬会极大增加无意义传播尺度。建议把渐近距离配置化，并默认使用地球半径倍数。

### 8.4 低速和插值域

地球逃逸速度远小于太阳，暗物质速度分布和重力聚焦条件的数值范围会变化。散射率插值的速度上限 `vMax = 0.75` 当前是自然单位速度，仍可用，但要确认采样不会频繁越界。

### 8.5 命名和兼容

如果一次性把项目名、target 名、输出目录全部改掉，容易影响现有脚本。建议先兼容旧名，再在第二阶段重命名。

## 9. 建议文件变更清单

第一批推荐改动：

```text
vendor/damascus/include/Celestial_Model.hpp
vendor/damascus/include/Solar_Model.hpp
vendor/damascus/src/Solar_Model.cpp
vendor/damascus/include/Simulation_Utilities.hpp
vendor/damascus/src/Simulation_Utilities.cpp
vendor/damascus/include/Simulation_Trajectory.hpp
vendor/damascus/src/Simulation_Trajectory.cpp
src/main.cpp
CMakeLists.txt
config/earth_smoke.cfg
config/earth_example.cfg
README.md
```

第二批推荐新增：

```text
vendor/damascus/include/Earth_Model.hpp
vendor/damascus/src/Earth_Model.cpp
vendor/damascus/data/earth_prem.dat
DOC/earth_model_data_format.md
```

## 10. 推荐最终效果

运行太阳版本：

```bash
mpirun -np 1 ./build/DaMaSCUS-Body-TrajectoryTXT config/smoke.cfg
```

运行地球版本：

```bash
mpirun -np 1 ./build/DaMaSCUS-Body-TrajectoryTXT config/earth_smoke.cfg
```

地球配置核心字段示例：

```text
body = "Earth";
output_dir = "./output";
sample_size = 10;
max_trajectories = 1000;
initial_radius_body_radius = 2.0;
asymptotic_distance_body_radius = 1000.0;
interpolation_points = 300;
```

输出目录示例：

```text
output/earth/results_<log10_dm_mass>_<log10_cross_section>/trajectory_1_task0.txt
```

## 11. 推荐决策

建议不要直接把 `Solar_Model` 改名成 `Earth_Model`，因为这样会破坏现有太阳模拟，也不利于对比验证。更稳妥的做法是：

1. 先抽象 `Celestial_Model`。
2. 让 `Solar_Model` 作为第一个实现继续工作。
3. 增加 `Earth_Model` 作为第二个实现。
4. 用同一套 `Trajectory_Simulator` 跑不同天体。

这样改造后，地球模拟不是一个临时分支，而是变成项目的通用能力。

## 12. 子任务书拆分

本总方案已经拆分为以下可执行子任务书：

1. `DOC/earth_refactor_subtasks_overview.md`：子任务总览、依赖关系和里程碑。
2. `DOC/earth_refactor_task_01_baseline_guardrails.md`：太阳基线与兼容护栏。
3. `DOC/earth_refactor_task_02_celestial_model_abstraction.md`：抽象 `Celestial_Model` 接口。
4. `DOC/earth_refactor_task_03_earth_model_data.md`：新增 `Earth_Model` 和地球数据。
5. `DOC/earth_refactor_task_04_runtime_integration.md`：主程序接入 Earth 运行路径。
6. `DOC/earth_refactor_task_05_validation_and_docs.md`：物理校验、文档与最终验收。

推荐按编号顺序执行。每个子任务都应独立完成一次构建或 smoke 级验证，再进入下一个任务。
