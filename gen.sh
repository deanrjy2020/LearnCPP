#!/bin/bash

OUTPUT_FILE="g_tests.h"

echo "// This file is auto-generated. DO NOT EDIT." > $OUTPUT_FILE
echo "" >> $OUTPUT_FILE
echo "// This file is part of the main.cpp" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE
echo "// 声明外部的测试入口" >> $OUTPUT_FILE

TESTS=$(cat "tests_list.src")
#TESTS="name1 name2"

STR1=""
STR2="// 注册所有测试函数"$'\n'
STR2+="// use map for alphabetical order when printing all."$'\n'
STR2+="map<string, function<void()>> testMap = {"$'\n'
STR2+="    // namespace string     namespace             entry"$'\n'
for T in ${TESTS}; do
    T_FIXED=$(printf "%-20s" "$T")  # 左对齐，宽度为10，用空格填充
    STR1+="namespace "$T_FIXED"{ void cppMain(); }"$'\n'
    STR2+="    { \""$(printf "%-20s" "$T\",")" "$T_FIXED"::cppMain },"$'\n'
done
STR2+="};"

echo "$STR1" >> $OUTPUT_FILE
echo "$STR2" >> $OUTPUT_FILE

# touch main.cpp to rebuild
touch main.cpp

echo "Done."