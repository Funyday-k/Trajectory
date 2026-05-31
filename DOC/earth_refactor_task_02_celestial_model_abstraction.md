# 子任务 02：抽象 Celestial_Model 接口

## 1. 目标

把轨迹传播、初始条件、散射率插值和输出辅助逻辑从 `Solar_Model` 具体类型解耦，改为依赖通用天体模型接口。完成后，太阳模型仍是唯一运行实现，但核心模拟代码应已经可以接受任意实现了接口的天体模型。

## 2. 范围

建议涉及文件：

- `vendor/damascus/include/Celestial_Model.hpp`
- `vendor/damascus/include/Solar_Model.hpp`
- `vendor/damascus/src/Solar_Model.cpp`
- `vendor/damascus/include/Simulation_Utilities.hpp`
- `vendor/damascus/src/Simulation_Utilities.cpp`
- `vendor/damascus/include/Simulation_Trajectory.hpp`
- `vendor/damascus/src/Simulation_Trajectory.cpp`
- `src/main.cpp`
- `CMakeLists.txt`

不在本任务中做：

- 不新增地球数据表。
- 不实现 `Earth_Model`。
- 不改变输出目录结构。
- 不把可执行文件改名为通用 body 名称。

## 3. 接口建议

新增 `Celestial_Model.hpp`，提供轨迹代码需要的最小虚接口：

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

接口可以按实际编译需求微调，但应避免把太阳专属字段暴露给轨迹层。注意：`DM_Scattering_Rate_*`、`Total_DM_Scattering_Rate`、`Mass`、`Temperature` 等查询函数暂时不强制做成 `const`，因为现有 `Solar_Model` 内部插值和缓存接口未必满足 const 调用；优先保证小步可编译。

## 4. 运行参数同步抽象

本任务除了类型签名，还必须把轨迹层的“太阳尺度运行参数”改成 body 尺度参数，否则任务 04 接入地球时会返工：

- `Initial_Conditions` 应新增 `asymptotic_distance` 参数，调用侧暂时传入旧值 `1000.0 * AU`，任务 04 再改成配置化。
- `Hyperbolic_Kepler_Shift` 应接收 `Celestial_Model& body`，使用 `body.Total_Mass()` 和 `body.Radius()`。
- `Free_Propagation_Time_Step_Cap` 应接收中心天体总质量或 `Celestial_Model& body`，不再内部使用 `mSun`。
- `TrajectoryBincount` 当前依赖 `R_SUN_KM`、`BIN_MAX_KM`、`BIN_WIDTH_KM` 编译期常量；应改成保存 `bin_max_km` 和 `bin_width_km` 的运行期配置，或由 `Trajectory_Simulator` 按 body 半径设置。
- `Trajectory_Result::Print_Summary` 中的 `Final radius [rSun]` 应改成 body 半径单位或通用文本。

## 5. 实施步骤

1. 新增 `Celestial_Model.hpp`，只放轨迹层真正需要的接口。
2. 让 `Solar_Model` 继承 `Celestial_Model`，并补齐 `Name()`、`Radius()`、`Total_Mass()`、`Target_Count()`、`Target_Isotope()` 等访问函数。
3. 把 `Simulation_Utilities` 和 `Simulation_Trajectory` 中接受 `Solar_Model&` 的签名改为 `Celestial_Model&` 或 `const Celestial_Model&`。
4. 把轨迹层直接访问 `solar_model.target_isotopes` 的地方改为 `body.Target_Count()` / `body.Target_Isotope(index)`。
5. 把轨迹层中的 `rSun` 替换为 `body.Radius()`。
6. 把轨迹层中的 `mSun` 替换为 `body.Total_Mass()`，如果函数处在天体内部并且应使用包围质量，则使用 `body.Mass(r)`。
7. 把 `Initial_Conditions` 的渐近距离和撞击参数计算改为由参数和 `body.Radius()` 控制，太阳路径先传旧默认值以保持行为。
8. 把 `TrajectoryBincount` 的 bin 最大半径从编译期太阳常量改成运行期参数；如果 `Data_Generation` 仍依赖固定数组长度，数组长度可保留 `NUM_BINS`，但 bin 宽度必须由当前 body 决定。
9. 调整 `Hyperbolic_Kepler_Shift`、`Initial_Conditions`、`Outward_Escaping_At_Boundary`、`Particle_Captured` 等函数的错误信息，从 `Sun` 语义改成 `body` 语义。
10. 更新 CMake，确保新增头文件路径可见；如果接口只有纯虚声明，不需要新增 `.cpp`。

## 6. 重点替换清单

| 原依赖 | 替换方向 |
| --- | --- |
| `Solar_Model& solar_model` | `Celestial_Model& body` |
| `rSun` | `body.Radius()` |
| `mSun` | `body.Total_Mass()` 或 `body.Mass(r)` |
| `target_isotopes.size()` | `body.Target_Count()` |
| `target_isotopes[index]` | `body.Target_Isotope(index)` |
| `R_SUN_KM` / `BIN_MAX_KM` / `BIN_WIDTH_KM` | 运行期 `bincount_max_radius` / `bin_width` |
| `1000.0 * AU` | `asymptotic_distance` 参数，太阳默认保持旧值 |
| `inside the Sun` | `inside the body` 或带 `body.Name()` 的信息 |

## 7. 验收标准

- 太阳 smoke 配置仍能编译运行。
- `Simulation_Trajectory` 和 `Simulation_Utilities` 的公开签名不再依赖 `Solar_Model&`。
- `Solar_Model` 仍能完成散射率插值和轨迹模拟。
- 轨迹 TXT 列格式不变。
- CMake target 名称保持 `DaMaSCUS-SUN-TrajectoryTXT`。
- 通用轨迹算法内部不再出现 `rSun`、`mSun`、`R_SUN_KM`、`BIN_MAX_KM` 这类太阳专属常量。
- `Initial_Conditions` 和 `Hyperbolic_Kepler_Shift` 已经可以接收当前 body 的尺度参数。

## 8. 建议验证命令

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
```

建议额外用搜索确认核心层不再出现不该出现的具体类型依赖：

```bash
rg "Solar_Model&|Solar_Model \\*|target_isotopes|rSun|mSun|R_SUN_KM|BIN_MAX_KM|1000.0 \\* AU" vendor/damascus/include vendor/damascus/src src/main.cpp
```

搜索结果允许在 `Solar_Model.hpp/.cpp`、太阳专属构造代码和兼容配置说明中出现，但不应继续出现在通用轨迹算法内部。

## 9. 风险与注意事项

- `const` 正确性可能会导致接口签名反复调整；优先让物理查询函数保持和现有实现一致，避免一次性大改。
- 如果 `Solar_Model` 公开字段很多，不要为了省事把整个对象继续暴露给轨迹层。
- `Mass(r)` 与 `Total_Mass()` 的物理语义要分清：外部 Kepler 传播通常使用总质量，内部引力加速度可能需要包围质量。
- `TrajectoryBincount` 虽然主要用于已有数据生成路径，但它在 `Trajectory_Simulator` 中已经参与状态维护；不处理会让地球轨迹的径向 bin 仍按太阳半径解释。

## 10. 完成情况

完成日期：2026-05-31

执行分支：`earth-branch`

已完成内容：

- 新增 `vendor/damascus/include/Celestial_Model.hpp`，定义通用天体模型接口。
- `Solar_Model` 已继承 `Celestial_Model`，并实现 `Name()`、`Radius()`、`Total_Mass()`、`Target_Count()`、`Target_Isotope()` 等接口。
- `Simulation_Utilities` 的 `Event::Asymptotic_Speed_Sqr`、`Initial_Conditions`、`Hyperbolic_Kepler_Shift` 已改为接收 `Celestial_Model&`。
- `Initial_Conditions` 已新增 `asymptotic_distance` 参数；太阳路径暂时继续传入旧默认 `1000 AU`。
- `Hyperbolic_Kepler_Shift` 已改为使用 `body_model.Total_Mass()` 和 `body_model.Radius()`。
- `Simulation_Trajectory` 已改为由 `Trajectory_Simulator` 持有 `Celestial_Model&`，散射、逃逸、捕获、RK45 传播和 target 访问均走通用接口。
- `TrajectoryBincount` 的 bin 最大半径和 bin 宽度已由运行期 `maximum_distance` 决定，不再使用固定太阳半径常量。
- `src/main.cpp` 的 TXT 输出辅助路径已改为使用 `Celestial_Model&`；当前仍只构造 `Solar_Model`，符合本任务“不接入 Earth runtime factory”的边界。

验证结果：

- `cmake -S . -B build ...` 通过。
- `cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4` 通过。
- `mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg` 通过。
- 太阳 smoke summary 保持稳定：`Trajectory files = 1`，`Free = 1`，`Text rows written = 9`，`RK45 steps = 83`。
- 搜索确认 `Simulation_Utilities` 和 `Simulation_Trajectory` 的公开签名不再依赖 `Solar_Model&`。

遗留说明：

- 主程序仍只支持 `body = "Sun"`，`Earth` 的 runtime factory 和配置入口留给子任务 04。
- 构建中仍有第三方依赖和既有 `unique_ptr` 析构 warning，本任务未处理这些无关 warning。
