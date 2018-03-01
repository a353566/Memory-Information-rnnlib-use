#ifndef NUMERIC_HPP
#define NUMERIC_HPP

#include <string>

// 將 string 轉成數字的方法，不是的話回傳 false
bool StringToNumber(std::string numStr, int *num) {
  // 轉成數字，不是的話回傳 false
  int isNegative = 1;
  int capacity = 0;
  int i=0;
  // 看是不是負號
  if (numStr[0] == '-')  {
    isNegative = -1;
    i=1;
  }
  for (i; i < numStr.size(); i++) {
    int temp = numStr[i] - '0';
    if ( 0<=temp && temp<=9 ) {
      capacity = 10*capacity + temp;
    } else {
      return false;
    }
  }
  *num = isNegative * capacity;
  return true;
}

#endif /* NUMERIC_HPP */