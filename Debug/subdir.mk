################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../MotionDetect.cpp \
../Record.cpp \
../JExport.cpp \
../Capture.cpp \
../pogocam.cpp



OBJS += \
./MotionDetect.o \
./Record.o \
./JExport.o \
./Capture.o \
./pogocam.o

CPP_DEPS += \
./MotionDetect.d \
./Record.d \
./JExport.d \
./Capture.d \
./pogocam.d


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"../../revel-1.1.0/include/" -I"../../libxvid/include/" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


