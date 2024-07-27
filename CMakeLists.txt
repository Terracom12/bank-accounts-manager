cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	message( FATAL_ERROR "Compiling with MSVC is unsupported!" )
endif()

project(BankAccounts VERSION 0.1 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(Catch2)

##### Build sources as library for easy linkage to both tests and runner application

add_library(
	bank_accounts_lib
	"src/bank_account/sc_checking_account.cpp"
	"src/bank_account/nosc_checking_account.cpp"
	"src/bank_account/interest_handler.cpp"
	"src/bank_account/hi_savings_account.cpp"
	"src/bank_account/hi_checking_account.cpp"
	"src/bank_account/savings_account.cpp"
	"src/bank_account/cd_account.cpp"
	"src/bank_account/account_info.cpp"
)

target_compile_options(
	bank_accounts_lib PUBLIC
	-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
	"$<$<CONFIG:Debug>:-g;-O0;-fno-omit-frame-pointer>"
	"$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
	"$<$<CONFIG:Release>:-O3;-Werror>"
)

target_link_options(
	bank_accounts_lib PUBLIC
	"$<$<CONFIG:Debug>:-g3;-O0;-fno-omit-frame-pointer>"
	"$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
)

target_include_directories(
	bank_accounts_lib
	PUBLIC
	"include"
)

target_link_libraries(
	bank_accounts_lib
	PUBLIC
	fmt::fmt
)

add_executable(
	bank_accounts_exe
	"src/console_interface.cpp"
	"src/main.cpp"
)

target_link_libraries(bank_accounts_exe PRIVATE bank_accounts_lib)


##### Unit Testing with Catch2

enable_testing()

add_executable(
	bank_accounts_test
	"test/test_money.cpp"
	"test/test_accounts.cpp"
)

target_link_libraries(
	bank_accounts_test
	PUBLIC bank_accounts_lib
	PRIVATE Catch2::Catch2WithMain
)

include(Catch)
include(CTest)
catch_discover_tests(bank_accounts_test)
