################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/BloomFilter.cpp \
../src/CoinKey.cpp \
../src/IPv6.cpp \
../src/MerkleTree.cpp \
../src/StandardTransactions.cpp \
../src/TransactionSigner.cpp \
../src/hdkeys.cpp 

OBJS += \
./src/BloomFilter.o \
./src/CoinKey.o \
./src/IPv6.o \
./src/MerkleTree.o \
./src/StandardTransactions.o \
./src/TransactionSigner.o \
./src/hdkeys.o 

CPP_DEPS += \
./src/BloomFilter.d \
./src/CoinKey.d \
./src/IPv6.d \
./src/MerkleTree.d \
./src/StandardTransactions.d \
./src/TransactionSigner.d \
./src/hdkeys.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


