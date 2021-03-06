# Phenix Devkit Firmware v0.1
The PhenixPro DevKit is an intelligent mobile robot control, computing and networking platform. It's integrated in a Xilinx Zynq SoC, with dual core ARM Cortex-A9 CPU and FPGA fabric on a signal chip. It is also running PhenOS(based on freeRTOS) and Linux on each CPU core respectively. Attitude estimation, flight control, navigation are implemented by ArduPilot(http://ardupilot.org). So far, it only support quadcopter(we call it flying robot).

Here is a simple process for developers to compile the source code.

compile cpu1 BSP(Board Support Package):

```
make bsp-clean;make bsp -j8
```

compile pilot code:

```
make pilot-clean;make pilot -j8
```

compile all:

```
make clean;make -j8
```

**Note:**

​	Bcause the pilot code depends on cpu1 bsp, please make sure that you have compiled bsp before compiling pilot code.

For those who have further questions, please push your questions onto our developers' forum at http://dev.robsense.com, we have developers and our engineering team there for Q&A. And also have the Wechat group, since it's only for invited developers, please add our coordinator Jie(his wechat account is: zhoushanjinjie) first and he will invite you to the Wechat group. 
