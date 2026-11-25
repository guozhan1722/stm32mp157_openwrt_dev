################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/jim/projects/stm32mp157_openwrt_dev/firmware/cube-m4/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c 

OBJS += \
./Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.o 

C_DEPS += \
./Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.o: /home/jim/projects/stm32mp157_openwrt_dev/firmware/cube-m4/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c Middlewares/FreeRTOS/CMSIS-RTOS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32MP157Cxx -DCORE_CM4 -DDEBUG -c -I../../../../../../../../Drivers/STM32MP1xx_HAL_Driver/Inc -I../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32MP1xx/Include -I../../../../../../../../Drivers/CMSIS/RTOS2/Include -I../../../../../../../../Utilities/ResourcesManager -I../../../../../../../../Drivers/STM32MP1xx_HAL_Driver/Inc/Legacy -I../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../../../../../../../Drivers/BSP/STM32MP15xx_DISCO -I../../../Inc -I../../../../../../../../Drivers/CMSIS/Include -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -ftls-model=local-exec -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-FreeRTOS-2f-CMSIS-2d-RTOS

clean-Middlewares-2f-FreeRTOS-2f-CMSIS-2d-RTOS:
	-$(RM) ./Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.cyclo ./Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.d ./Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.o ./Middlewares/FreeRTOS/CMSIS-RTOS/cmsis_os2.su

.PHONY: clean-Middlewares-2f-FreeRTOS-2f-CMSIS-2d-RTOS

