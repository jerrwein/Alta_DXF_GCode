################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CPP_FLAGS := -O0 -g3 -Wall
CC_FLAGS := -O0 -g3 -Wall

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/minIni.c \
../src/DXF_FileInput.cpp \
../src/DoubleVector.cpp \
../src/Main.cpp \
../src/STL_MapBase.cpp \
../src/STL_VectorBase.cpp \
../src/ToolPaths.cpp \
../src/GCode_Output.cpp \
../src/OpenGL_DisplayPaths.cpp 

OBJS += \
./src/minIni.o \
./src/DXF_FileInput.o \
./src/DoubleVector.o \
./src/Main.o \
./src/STL_MapBase.o \
./src/STL_VectorBase.o \
./src/ToolPaths.o \
./src/GCode_Output.o \
./src/OpenGL_DisplayPaths.o 

# Presently unused
CPP_DEPS += \
./src/minIni.d \
./src/DXF_FileInput.d \
./src/DoubleVector.d \
./src/Main.d \
./src/STL_MapBase.d \
./src/STL_VectorBase.d \
./src/ToolPaths.d \
./src/GCode_Output.d \
./src/OpenGL_DisplayPaths.d

H_DEPENDS += \
src/minIni.h \
src/minGlue.h \
src/DXF_FileInput.h \
src/DoubleVector.h \
src/STL_MapBase.h \
src/STL_VectorBase.h \
src/ToolPaths.h \
src/GCode_Output.h \
src/OpenGL_DisplayPaths.h

# Each subdirectory must supply rules for building sources it contributes
src/%.o: src/%.cpp $(H_DEPENDS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPP_FLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: src/%.c $(H_DEPENDS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	cc $(CC_FLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


